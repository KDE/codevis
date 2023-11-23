// ct_lvtclp_tool.cpp                                                 -*-C++-*-

/*
// Copyright 2023 Codethink Ltd <codethink@codethink.co.uk>
// SPDX-License-Identifier: Apache-2.0
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#include <ct_lvtclp_tool.h>

#include <ct_lvtclp_clputil.h>
#include <ct_lvtclp_compilerutil.h>
#include <ct_lvtclp_fileutil.h>
#include <ct_lvtclp_logicaldepscanner.h>
#include <ct_lvtclp_logicalpostprocessutil.h>
#include <ct_lvtclp_physicaldepscanner.h>
#include <ct_lvtclp_toolexecutor.h>

#include <ct_lvtmdb_objectstore.h>
#include <ct_lvtshr_functional.h>
#include <ct_lvtshr_stringhelpers.h>

#include <clang/Tooling/CommonOptionsParser.h>

#include <llvm/Support/GlobPattern.h>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include <QDebug>

Q_LOGGING_CATEGORY(LogTool, "log.tool")

namespace {

using Codethink::lvtclp::ClpUtil;
// using Codethink::lvtclp::FileUtil;
using Codethink::lvtclp::LvtCompilationDatabase;

// =================================
// class PartialCompilationDatabase
// =================================
class LvtCompilationDatabaseImpl : public LvtCompilationDatabase {
    // Partial implementation of LvtCompilationDatabase to share code between
    // PartialCompilationDatabase and SubsetCompilationDatabase.
    // Implements looking up files and packages in the compilation database

  private:
    // DATA
    std::vector<std::string> d_files;
    std::vector<std::filesystem::path> d_paths;
    std::unordered_set<std::string> d_components;
    std::unordered_set<std::string> d_pkgs;
    std::optional<std::filesystem::path> d_prefix;

  protected:
    const std::vector<std::string>& files() const
    {
        return d_files;
    }

    const std::vector<std::filesystem::path>& paths() const
    {
        return d_paths;
    }

    void reserve(std::size_t size)
    {
        d_files.reserve(size);
        d_paths.reserve(size);
        d_components.reserve(size);
        // d_pkgs will need a size much less than size. Don't try to estimate.
    }

    void shrinkToFit()
    {
        d_files.shrink_to_fit();
        d_paths.shrink_to_fit();
        // d_components: no shrink_to_fit for std::unordered_set
        // d_pkgs: no shrink_to_fit for std::unordered_set
    }

    void addPath(const std::filesystem::path& path)
    // path should be weakly_canonical
    {
        using Codethink::lvtclp::FileType;

        const auto it = std::find(d_paths.begin(), d_paths.end(), path);
        if (it != d_paths.end()) {
            return;
        }

        // we store multiple copies of the path in different formats so we only
        // have to pay for the conversions once
        d_paths.push_back(path);
        d_files.push_back(path.string());

        std::filesystem::path component(path);
        component.replace_extension();
        d_components.insert(component.string());

        // these two work because the directory structure is fixed:
        // .../groups/grp/grppkg/grppkg_component.cpp etc
        // package
        d_pkgs.insert(path.parent_path().string());
        // package group
        d_pkgs.insert(path.parent_path().parent_path().string());
    }

    void setPrefix(std::filesystem::path prefix)
    {
        d_prefix.emplace(std::move(prefix));
    }

  public:
    bool containsFile(const std::string& path) const override
    {
        using Codethink::lvtclp::FileType;
        const FileType type = Codethink::lvtclp::ClpUtil::categorisePath(path);
        if (type != FileType::e_Source && type != FileType::e_Header) {
            return false;
        }

        assert(d_prefix);
        std::filesystem::path filePath = std::filesystem::weakly_canonical(*d_prefix / path);

        // the compilation DB only knows about source files, so look for any
        // file which has is the same name, ignoring the extension.
        // Lakosian rules say we can't have headers without source (and vice-versa)
        filePath.replace_extension();

        return d_components.find(filePath.string()) != d_components.end();
    }

    bool containsPackage(const std::string& pkg) const override
    {
        assert(d_prefix);
        std::filesystem::path pkgPath = std::filesystem::weakly_canonical(*d_prefix / pkg);
        return d_pkgs.find(pkgPath.string()) != d_pkgs.end();
    }
};

// =================================
// class PartialCompilationDatabase
// =================================

class PartialCompilationDatabase : public LvtCompilationDatabaseImpl {
    // Produce a subset of another compilation database using ignore globs
    // Also applies modifications to compiler include directories according to
    // lvtclp::CompilerUtil

  private:
    // DATA
    std::vector<std::string> d_ignoreGlobs;
    std::function<void(const std::string&, long)> d_messageCallback;
    std::vector<clang::tooling::CompileCommand> d_compileCommands;
    std::filesystem::path d_commonParent;
    std::vector<llvm::GlobPattern> d_ignorePatterns;

  public:
    // CREATORS
    explicit PartialCompilationDatabase(std::filesystem::path sourcePath,
                                        std::vector<clang::tooling::CompileCommand> compileCommands,
                                        std::function<void(const std::string&, long)> messageCallback):
        d_messageCallback(std::move(messageCallback)),
        d_compileCommands(std::move(compileCommands)),
        d_commonParent(std::move(sourcePath))
    {
    }

    void setIgnoreGlobs(const std::vector<llvm::GlobPattern>& ignoreGlobs)
    {
        d_ignorePatterns = ignoreGlobs;
    }

    // MANIPULATORS
    void setup(Codethink::lvtclp::Tool::UseSystemHeaders useSystemHeaders, bool printToConsole)
    {
        using Codethink::lvtclp::CompilerUtil;
        std::vector<std::string> sysIncludes;

        std::optional<std::string> bundledHeaders = CompilerUtil::findBundledHeaders(printToConsole);
        if (!bundledHeaders) {
            const bool searchForHeaders = useSystemHeaders == Codethink::lvtclp::Tool::UseSystemHeaders::e_Yes
                || ((useSystemHeaders == Codethink::lvtclp::Tool::UseSystemHeaders::e_Query)
                    && CompilerUtil::weNeedSystemHeaders());

            if (searchForHeaders) {
                sysIncludes = CompilerUtil::findSystemIncludes();
                for (std::string& include : sysIncludes) {
                    std::string arg;
                    arg.append("-isystem").append(include);
                    include = std::move(arg);
                }
            }
        } else {
            sysIncludes.push_back("-isystem" + *bundledHeaders);
        }

        // Command lines that doesn't exist on clang, and could be potentially used
        // from compile_commands.json - if we allow them in, we will have problems.
        const std::vector<std::string> missingCommandLineOnClang = {"-mno-direct-extern-access"};

        auto removeIgnoredFilesLambda = [this](clang::tooling::CompileCommand& cmd) -> bool {
            return ClpUtil::isFileIgnored(cmd.Filename, d_ignorePatterns);
        };

        d_compileCommands.erase(
            std::remove_if(d_compileCommands.begin(), d_compileCommands.end(), removeIgnoredFilesLambda),

            std::end(d_compileCommands));

        for (auto& cmd : d_compileCommands) {
            std::filesystem::path path = std::filesystem::weakly_canonical(cmd.Filename);
            addPath(path);
            cmd.Filename = path.string();

            // Disable all warnings to defeat any -Werror arguments
            // the source is configured to use and to make the output
            // easier to read
            cmd.CommandLine.emplace_back("-Wno-everything");

            auto new_end = std::remove_if(cmd.CommandLine.begin(),
                                          cmd.CommandLine.end(),
                                          [&missingCommandLineOnClang](const std::string& command) -> bool {
                                              for (const auto& missingCmdLine : missingCommandLineOnClang) {
                                                  if (command.find(missingCmdLine) != command.npos) {
                                                      return true;
                                                  }
                                              }
                                              return false;
                                          });

            cmd.CommandLine.erase(new_end, cmd.CommandLine.end());

            // add system includes
            std::copy(sysIncludes.begin(), sysIncludes.end(), std::back_inserter(cmd.CommandLine));
        }

        shrinkToFit();
        setPrefix(d_commonParent);
    }

    // ACCESSORS
    const std::filesystem::path& commonParent() const
    {
        return d_commonParent;
    }

    std::vector<clang::tooling::CompileCommand> getCompileCommands(llvm::StringRef FilePath) const override
    {
        for (auto const& cmd : d_compileCommands) {
            if (cmd.Filename == FilePath) {
                return {cmd};
            }
        }
        return {};
    }

    std::vector<std::string> getAllFiles() const override
    {
        return files();
    }

    std::vector<clang::tooling::CompileCommand> getAllCompileCommands() const override
    {
        return d_compileCommands;
    }
};

class SubsetCompilationDatabase : public LvtCompilationDatabaseImpl {
    // A subset compilation database using an allow-list of paths
  private:
    // DATA
    const clang::tooling::CompilationDatabase& d_parent;

  public:
    // CREATORS
    explicit SubsetCompilationDatabase(const clang::tooling::CompilationDatabase& parent,
                                       const std::vector<std::string>& paths,
                                       const std::filesystem::path& commonParent):
        d_parent(parent)
    {
        reserve(paths.size());

        for (const std::string& path : paths) {
            std::filesystem::path fullPath = commonParent / path;
            addPath(std::filesystem::weakly_canonical(fullPath));
        }

        shrinkToFit();
        setPrefix(commonParent);
    }

    // ACCESSORS
    std::vector<clang::tooling::CompileCommand> getCompileCommands(llvm::StringRef FilePath) const override
    {
        const std::vector<std::string>& files = SubsetCompilationDatabase::files();
        const auto it = std::find(files.begin(), files.end(), FilePath);
        if (it != files.end()) {
            return d_parent.getCompileCommands(FilePath);
        }

        return {};
    }

    std::vector<std::string> getAllFiles() const override
    {
        std::vector<std::string> out;
        out.reserve(paths().size()); // estimate

        std::vector<std::string> parentFiles = d_parent.getAllFiles();
        for (const std::string& path : files()) {
            const auto it = std::find(parentFiles.begin(), parentFiles.end(), path);
            if (it != parentFiles.end()) {
                out.push_back(path);
            }
        }

        return out;
    }

    std::vector<clang::tooling::CompileCommand> getAllCompileCommands() const override
    {
        std::vector<clang::tooling::CompileCommand> out;
        out.reserve(paths().size()); // estimate

        for (const std::string& path : files()) {
            std::vector<clang::tooling::CompileCommand> pathCmds = d_parent.getCompileCommands(path);
            std::move(pathCmds.begin(), pathCmds.end(), std::back_inserter(out));
        }

        return out;
    }

    int numCompileCommands() const
    // using int because qt signals don't like std::size_t and ultimately
    // we are passing this to QProgressBar::setMaximum anyway
    {
        const std::size_t size = getAllCompileCommands().size();

        // we would likely have run out of memory long before we could have
        // INT_MAX compile commands
        if (size > INT_MAX) {
            return INT_MAX;
        }
        return static_cast<int>(size);
    }
};

} // unnamed namespace

namespace Codethink::lvtclp {

struct Tool::Private {
    std::filesystem::path sourcePath;
    std::optional<clang::tooling::CompileCommand> compileCommand;
    std::function<void(const std::string&, long)> messageCallback;
    std::vector<std::filesystem::path> compileCommandsJsons;
    std::filesystem::path databasePath;
    unsigned numThreads = 1;
    std::vector<llvm::GlobPattern> ignoreList;
    std::vector<std::filesystem::path> nonLakosianDirs;
    std::vector<std::pair<std::string, std::string>> thirdPartyDirs;
    bool printToConsole = false;
    std::optional<HeaderCallbacks::HeaderLocationCallback_f> headerLocationCallback = std::nullopt;
    std::optional<HandleCppCommentsCallback_f> handleCppCommentsCallback = std::nullopt;

    UseSystemHeaders useSystemHeaders = UseSystemHeaders::e_Query;

    // The tool can be used either with a local memory database or with a
    // shared one. Only one can be used at a time. The default is to use
    // localMemDb. If setMemDb(other) is called, will ignore the local one.
    lvtmdb::ObjectStore localMemDb;
    std::shared_ptr<lvtmdb::ObjectStore> sharedMemDb = nullptr;

    std::optional<PartialCompilationDatabase> compilationDb;
    // full compilation db (compileCommandJson minus ignoreList)

    std::optional<SubsetCompilationDatabase> incrementalCdb;
    // partial compilation db containing only the paths which need to be
    // re-parsed for an incremental update

    // Normally the ToolExecutor is managed from runPhysical() or runFull()
    // but it can be cancelled from another thread. The mutex should be held
    // when cancelling the executor or deleting it. The mutex should not be
    // held to execute the tool executor because that would prevent the
    // cancellation. It is safe to execute without the mutex because the
    // toolExecutor is not invalidated from the cancellation thread.
    std::mutex executorMutex;
    ToolExecutor *toolExecutor = nullptr;
    // only non-null when there is an executor running
    bool executorCancelled = false;

    bool lastRunMadeChanges = false;

    bool showDatabaseErrors = false;
    // Flag that indicates that database errors should be
    // reported to the UI.

    [[nodiscard]] lvtmdb::ObjectStore& memDb()
    {
        return sharedMemDb ? *sharedMemDb : localMemDb;
    }

    void setNumThreads(unsigned numThreadsIn)
    {
        if (numThreadsIn < 1) {
            numThreads = 1;
        } else {
            numThreads = numThreadsIn;
        }
    }

    void setIgnoreList(const std::vector<std::string>& userIgnoreList)
    {
        ignoreList.clear();

        // add non-duplicates
        std::set<std::string> nonDuplicatedItems(std::begin(userIgnoreList), std::end(userIgnoreList));
        for (const auto& ignoreFile : nonDuplicatedItems) {
            llvm::Expected<llvm::GlobPattern> pat = llvm::GlobPattern::create(ignoreFile);
            if (pat) {
                ignoreList.push_back(pat.get());
            }
        }

        if (compilationDb) {
            compilationDb->setIgnoreGlobs(ignoreList);
        }
    }

    void setNonLakosianDirs(const std::vector<std::filesystem::path>& nonLakosians)
    {
        nonLakosianDirs.reserve(nonLakosians.size());

        std::transform(nonLakosians.begin(),
                       nonLakosians.end(),
                       std::back_inserter(nonLakosianDirs),
                       [](const std::filesystem::path& dir) {
                           return std::filesystem::weakly_canonical(dir);
                       });
    }

    Private(Tool *tool,
            std::filesystem::path inSourcePath,
            std::vector<std::filesystem::path> inCompileCommandsJsons,
            std::filesystem::path inDatabasePath,
            unsigned numThreadsIn,
            const std::vector<std::string>& ignoreList,
            const std::vector<std::filesystem::path>& nonLakosians,
            std::vector<std::pair<std::string, std::string>> thirdPartyDirs,
            bool inPrintToConsole = true):
        sourcePath(std::move(inSourcePath)),
        messageCallback([tool](const std::string& msg, long tid) {
            tool->messageCallback(msg, tid);
        }),
        compileCommandsJsons(std::move(inCompileCommandsJsons)),
        databasePath(std::move(inDatabasePath)),
        thirdPartyDirs(std::move(thirdPartyDirs)),
        printToConsole(inPrintToConsole)
    {
        setNumThreads(numThreadsIn);
        setIgnoreList(ignoreList);
        setNonLakosianDirs(nonLakosians);
    }

    Private(Tool *tool,
            std::filesystem::path inSourcePath,
            clang::tooling::CompileCommand compileCommand,
            std::filesystem::path inDatabasePath,
            const std::vector<std::string>& ignoreList,
            const std::vector<std::filesystem::path>& nonLakosians,
            std::vector<std::pair<std::string, std::string>> thirdPartyDirs,
            bool inPrintToConsole = true):
        sourcePath(std::move(inSourcePath)),
        compileCommand(std::in_place, compileCommand),
        messageCallback([tool](const std::string& msg, long tid) {
            tool->messageCallback(msg, tid);
        }),
        databasePath(std::move(inDatabasePath)),
        thirdPartyDirs(std::move(thirdPartyDirs)),
        printToConsole(inPrintToConsole)
    {
        setNumThreads(1);
        setIgnoreList(ignoreList);
        setNonLakosianDirs(nonLakosians);
    }

    Private(Tool *tool,
            std::filesystem::path inSourcePath,
            const std::vector<clang::tooling::CompileCommand>& compileCommands,
            std::filesystem::path inDatabasePath,
            unsigned numThreadsIn,
            const std::vector<std::string>& ignoreList,
            std::vector<std::filesystem::path> nonLakosianDirs,
            std::vector<std::pair<std::string, std::string>> thirdPartyDirs,
            bool inPrintToConsole):
        sourcePath(std::move(inSourcePath)),
        messageCallback([tool](const std::string& msg, long tid) {
            tool->messageCallback(msg, tid);
        }),
        databasePath(std::move(inDatabasePath)),
        nonLakosianDirs(std::move(nonLakosianDirs)),
        thirdPartyDirs(std::move(thirdPartyDirs)),
        printToConsole(inPrintToConsole),
        compilationDb(std::in_place, sourcePath, compileCommands, messageCallback)
    {
        setNumThreads(numThreadsIn);
        setIgnoreList(ignoreList);
    }
};

Tool::Tool(std::filesystem::path sourcePath,
           const std::vector<std::filesystem::path>& compileCommandsJsons,
           const std::filesystem::path& databasePath,
           unsigned numThreads,
           const std::vector<std::string>& ignoreList,
           const std::vector<std::filesystem::path>& nonLakosianDirs,
           std::vector<std::pair<std::string, std::string>> thirdPartyDirs,
           bool printToConsole):
    d(std::make_unique<Tool::Private>(this,
                                      std::move(sourcePath),
                                      compileCommandsJsons,
                                      databasePath,
                                      numThreads,
                                      ignoreList,
                                      nonLakosianDirs,
                                      std::move(thirdPartyDirs),
                                      printToConsole))
{
}

Tool::Tool(std::filesystem::path sourcePath,
           const clang::tooling::CompileCommand& compileCommand,
           const std::filesystem::path& databasePath,
           const std::vector<std::string>& ignoreList,
           const std::vector<std::filesystem::path>& nonLakosianDirs,
           std::vector<std::pair<std::string, std::string>> thirdPartyDirs,
           bool printToConsole):
    d(std::make_unique<Tool::Private>(this,
                                      std::move(sourcePath),
                                      compileCommand,
                                      databasePath,
                                      ignoreList,
                                      nonLakosianDirs,
                                      std::move(thirdPartyDirs),
                                      printToConsole))
{
}

Tool::Tool(std::filesystem::path sourcePath,
           const clang::tooling::CompilationDatabase& db,
           const std::filesystem::path& databasePath,
           unsigned numThreads,
           const std::vector<std::string>& ignoreList,
           const std::vector<std::filesystem::path>& nonLakosianDirs,
           std::vector<std::pair<std::string, std::string>> thirdPartyDirs,
           bool printToConsole):
    d(std::make_unique<Tool::Private>(this,
                                      std::move(sourcePath),
                                      db.getAllCompileCommands(),
                                      databasePath,
                                      numThreads,
                                      ignoreList,
                                      nonLakosianDirs,
                                      std::move(thirdPartyDirs),
                                      printToConsole))
{
    d->compilationDb->setup(UseSystemHeaders::e_Query, !printToConsole);
}

Tool::~Tool() noexcept = default;

void Tool::setSharedMemDb(std::shared_ptr<lvtmdb::ObjectStore> const& sharedMemDb)
{
    d->sharedMemDb = sharedMemDb;
}

lvtmdb::ObjectStore& Tool::getObjectStore()
{
    return d->memDb();
}

void Tool::setUseSystemHeaders(UseSystemHeaders value)
{
    d->useSystemHeaders = value;
}

bool Tool::ensureSetup()
{
    if (!processCompilationDatabase()) {
        return false;
    }
    assert(d->compilationDb);

    return true;
}

bool Tool::processCompilationDatabase()
{
    if (d->compilationDb) {
        return true;
    }

    if (!d->compileCommandsJsons.empty()) {
        CombinedCompilationDatabase compDb;
        for (const std::filesystem::path& path : d->compileCommandsJsons) {
            auto result = compDb.addCompilationDatabase(path);
            if (result.has_error()) {
                switch (result.error().kind) {
                case CompilationDatabaseError::Kind::ErrorLoadingFromFile: {
                    Q_EMIT messageFromThread(
                        tr(("Error loading file " + path.string() + " with " + result.error().message).c_str()),
                        0);
                    break;
                }
                case CompilationDatabaseError::Kind::CompileCommandsContainsNoCommands: {
                    Q_EMIT messageFromThread(
                        tr(("Error processing " + path.string() + " contains no commands").c_str()),
                        0);
                    break;
                }
                case CompilationDatabaseError::Kind::CompileCommandsContainsNoFiles: {
                    Q_EMIT messageFromThread(tr(("Error processing " + path.string() + " contains no files").c_str()),
                                             0);
                    break;
                }
                }
                return false;
            }
        }
        d->compilationDb.emplace(d->sourcePath, compDb.getAllCompileCommands(), d->messageCallback);
    } else if (d->compileCommand.has_value()) {
        d->compilationDb.emplace(d->sourcePath, std::vector({d->compileCommand.value()}), d->messageCallback);
    } else {
        std::cerr << "No compilation database nor compilation command on the tool execution";
        return false;
    }

    d->compilationDb->setIgnoreGlobs(d->ignoreList);
    d->compilationDb->setup(d->useSystemHeaders, !d->printToConsole);
    return true;
}

void Tool::setupIncrementalUpdate(FilesystemScanner::IncrementalResult& res, bool doIncremental)
// Processes what changes the filesystem scanner found, populating
// d->incrementalCdb with a code database containing only the files we need
// to re-parse later
{
    d->lastRunMadeChanges = !res.newFiles.empty() || !res.modifiedFiles.empty() || !res.deletedFiles.empty();
    if (!d->lastRunMadeChanges) {
        // no changes to report

        std::vector<std::string> files;
        if (!doIncremental) {
            // operate on every file anyway because we are in non-incremental mode
            files = d->compilationDb->getAllFiles();
        } // else files is empty because there are no changes

        d->incrementalCdb.emplace(*d->compilationDb, files, d->compilationDb->commonParent());
        return;
    }

    // delete files which were deleted or modified
    QList<std::string> removedFiles;
    std::set<intptr_t> removedPtrs;
    auto deleteFile = [this, &removedFiles, &removedPtrs](const std::string& qualifiedName) {
        if (d->printToConsole) {
            qDebug() << "Going to remove file: " << qualifiedName.c_str();
        }
        d->memDb().withRWLock([&] {
            const bool alreadyRemoved =
                std::find(std::begin(removedFiles), std::end(removedFiles), qualifiedName) != std::end(removedFiles);
            if (!alreadyRemoved) {
                removedFiles += d->memDb().removeFile(d->memDb().getFile(qualifiedName), removedPtrs);
            }
        });
    };

    std::for_each(res.modifiedFiles.begin(), res.modifiedFiles.end(), deleteFile);
    std::for_each(res.deletedFiles.begin(), res.deletedFiles.end(), deleteFile);

    for (const std::string& pkg : res.deletedPkgs) {
        lvtmdb::PackageObject *memPkg = nullptr;

        d->memDb().withROLock([&] {
            memPkg = d->memDb().getPackage(pkg);
        });
        std::set<intptr_t> removed;
        d->memDb().removePackage(memPkg, removed);
    }

    if (d->printToConsole) {
        qDebug() << "All files that are removed from our call:";
        for (const std::string& file : qAsConst(removedFiles)) {
            qDebug() << "\t " << file.c_str();
        }
    }

    // set up compilation database for incremental update
    std::vector<std::string> newPaths;
    if (doIncremental) {
        for (const std::string& removedFile : qAsConst(removedFiles)) {
            // we only want to add files removed because of dependency propagation,
            // not because the real files are gone
            const auto it = std::find(res.deletedFiles.begin(), res.deletedFiles.end(), removedFile);
            if (it == res.deletedFiles.end()) {
                newPaths.push_back(removedFile);
            }
        }

        std::move(res.newFiles.begin(), res.newFiles.end(), std::back_inserter(newPaths));
        std::move(res.modifiedFiles.begin(), res.modifiedFiles.end(), std::back_inserter(newPaths));
    } else {
        // We were asked not to do an incremental update so add all existing
        // files anyway
        newPaths = d->compilationDb->getAllFiles();
    }

    if (d->printToConsole) {
        qDebug() << "Files for the incremental db";
        for (const std::string& file : newPaths) {
            qDebug() << "\t " << file.c_str();
        }
    }

    d->incrementalCdb.emplace(*d->compilationDb, newPaths, d->compilationDb->commonParent());
}

bool Tool::findPhysicalEntities(bool doIncremental)
{
    if (!ensureSetup()) {
        return false;
    }

    bool dbErrorState = false;

    const lvtmdb::ObjectStore::State oldState = d->memDb().state();
    if (oldState == lvtmdb::ObjectStore::State::Error || oldState == lvtmdb::ObjectStore::State::PhysicalError) {
        // we can't trust anything in this database
        doIncremental = false;
        dbErrorState = true;

        // Since the Wt-to-Soci refactoring, we don't store previous acquired data on the database anymore, so if we
        // clear the database for any error, we'll lose the conectivity between the components. But there are some minor
        // errors that may be "ok" to continue, such as partial inclusion errors (e.g.: ONE file in a given component
        // couldn't be found) - In such cases it is not worth to wipe out the entire codebase just because of one
        // inclusion error.
        // TODO: Verify if there's a better handling for error databases and/or warn the user about it.
        //        // Don't trust anything in an error database
        //        d->memDb().withRWLock([&]() {
        //            d->memDb().clear();
        //        });
    }

    // We can do a physical update if we completed the last physical parse.
    if (oldState != lvtmdb::ObjectStore::State::PhysicalReady && oldState != lvtmdb::ObjectStore::State::AllReady
        && oldState != lvtmdb::ObjectStore::State::LogicalError) {
        // the database never contained physical information so we can't do an
        // incremental update
        doIncremental = false;
    }

    FilesystemScanner scanner(d->memDb(),
                              d->compilationDb->commonParent(),
                              *d->compilationDb,
                              d->messageCallback,
                              d->printToConsole,
                              d->nonLakosianDirs,
                              d->ignoreList);

    {
        std::unique_lock<std::mutex> lock(d->executorMutex);
        if (d->executorCancelled) {
            d->executorCancelled = false;
            return false;
        }
    }

    FilesystemScanner::IncrementalResult res = scanner.scanCompilationDb();
    {
        std::unique_lock<std::mutex> lock(d->executorMutex);
        if (d->executorCancelled) {
            d->executorCancelled = false;
            return false;
        }
    }

    setupIncrementalUpdate(res, doIncremental);

    {
        std::unique_lock<std::mutex> lock(d->executorMutex);
        if (d->executorCancelled) {
            d->executorCancelled = false;
            return false;
        }
    }

    if (d->lastRunMadeChanges || dbErrorState) {
        // we only have physical entities: no dependencies
        d->memDb().setState(lvtmdb::ObjectStore::State::NoneReady);
    }

    return true;
}

void Tool::setPrintToConsole(bool b)
{
    d->printToConsole = b;
}

bool Tool::runPhysical(bool skipScan)
{
    if (!ensureSetup()) {
        return false;
    }
    {
        std::unique_lock<std::mutex> lock(d->executorMutex);
        if (d->executorCancelled) {
            d->executorCancelled = false;
            return false;
        }
    }

    if (!skipScan && !findPhysicalEntities(true)) {
        return false;
    }

    assert(d->incrementalCdb);

    auto filenameCallback = [this](const std::string& path) {
        if (d->printToConsole) {
            qDebug() << "Processing file " << path;
        }
        Q_EMIT processingFileNotification(QString::fromStdString(path));
    };

    auto messageCallback = [this](const std::string& message, long threadId) {
        if (d->printToConsole) {
            qDebug() << message;
        }
        Q_EMIT messageFromThread(QString::fromStdString(message), threadId);
    };

    std::unique_ptr<clang::tooling::FrontendActionFactory> dependencyScanner =
        std::make_unique<DepScanActionFactory>(d->memDb(),
                                               d->compilationDb->commonParent(),
                                               d->nonLakosianDirs,
                                               d->thirdPartyDirs,
                                               filenameCallback,
                                               d->ignoreList,
                                               d->headerLocationCallback);

    Q_EMIT aboutToCallClangNotification(d->incrementalCdb->numCompileCommands());

    d->toolExecutor = new ToolExecutor(*d->incrementalCdb, d->numThreads, messageCallback, d->memDb());

    llvm::Error err = d->toolExecutor->execute(std::move(dependencyScanner));

    bool cancelled = false;
    {
        std::unique_lock<std::mutex> lock(d->executorMutex);
        delete d->toolExecutor;
        d->toolExecutor = nullptr;
        cancelled = d->executorCancelled;
        d->executorCancelled = false;
    }

    if (err) {
        d->memDb().setState(lvtmdb::ObjectStore::State::PhysicalError);

        err = llvm::handleErrors(std::move(err), [&](llvm::StringError const& err) -> llvm::Error {
            Q_EMIT messageFromThread(QString::fromStdString(err.getMessage()), 0);
            return llvm::Error::success();
        });
        return !err;
    }

    if (cancelled) {
        d->memDb().setState(lvtmdb::ObjectStore::State::NoneReady);
    } else {
        if (d->memDb().state() != lvtmdb::ObjectStore::State::AllReady
            || d->memDb().state() != lvtmdb::ObjectStore::State::LogicalError) {
            d->memDb().setState(lvtmdb::ObjectStore::State::PhysicalReady);
        }
    }

    return !err;
}

bool Tool::runFull(bool skipPhysical)
{
    if (!ensureSetup()) {
        return false;
    }
    {
        std::unique_lock<std::mutex> lock(d->executorMutex);
        if (d->executorCancelled) {
            d->executorCancelled = false;
            return false;
        }
    }

    bool doIncremental = true;
    const lvtmdb::ObjectStore::State oldState = d->memDb().state();
    if (oldState != lvtmdb::ObjectStore::State ::AllReady) {
        // the database never contained logical information so we can't do an
        // incremental update
        doIncremental = false;
    }

    if (!findPhysicalEntities(doIncremental)) {
        return false;
    }

    if (!skipPhysical && !runPhysical(true)) {
        return false;
    }

    assert(d->incrementalCdb);

    if (oldState != lvtmdb::ObjectStore::State::AllReady) {
        // if we aren't starting from a database with logical data, we need to
        // re-run on every file, even if incremental updates only found changes
        // in a few files
        d->incrementalCdb.emplace(*d->compilationDb, d->compilationDb->getAllFiles(), d->compilationDb->commonParent());
    }

    auto filenameCallback = [this](const std::string& path) {
        if (d->printToConsole) {
            qDebug() << "Processing file " << path;
        }
        Q_EMIT processingFileNotification(QString::fromStdString(path));
    };

    std::optional<std::function<void(const std::string&, long)>> messageOpt;
    if (d->showDatabaseErrors) {
        messageOpt = d->messageCallback;
    }

    std::unique_ptr<clang::tooling::FrontendActionFactory> actionFactory =
        std::make_unique<LogicalDepActionFactory>(d->memDb(),
                                                  d->compilationDb->commonParent(),
                                                  d->nonLakosianDirs,
                                                  d->thirdPartyDirs,
                                                  filenameCallback,
                                                  messageOpt,
                                                  d->printToConsole,
                                                  d->handleCppCommentsCallback);

    Q_EMIT aboutToCallClangNotification(d->incrementalCdb->numCompileCommands());

    d->toolExecutor = new ToolExecutor(*d->incrementalCdb, d->numThreads, d->messageCallback, d->memDb());

    llvm::Error err = d->toolExecutor->execute(std::move(actionFactory));

    bool cancelled = false;
    {
        std::unique_lock<std::mutex> lock(d->executorMutex);
        delete d->toolExecutor;
        d->toolExecutor = nullptr;
        cancelled = d->executorCancelled;
        d->executorCancelled = false;
    }

    if (err) {
        d->memDb().setState(lvtmdb::ObjectStore::State::LogicalError);

        err = llvm::handleErrors(std::move(err), [&](llvm::StringError const& err) -> llvm::Error {
            Q_EMIT messageFromThread(QString::fromStdString(err.getMessage()), 0);
            return llvm::Error::success();
        });
        return !err;
    }

    if (!cancelled) {
        d->memDb().setState(lvtmdb::ObjectStore::State::AllReady);
    }
    // else keep it the same because physical info is probably okay

    if (!cancelled) {
        if (d->printToConsole) {
            qDebug() << "Collating output database";
        }
        Q_EMIT processingFileNotification(tr("Collating output database"));
        // d->memDb().withROLock([&] {
        //    d->memDb().writeToDb(d->outputDb.session());
        //    d->outputDb.setState(static_cast<lvtmdb::ObjectStore::State>(d->memDb().state()));
        // });
    }

    if (!LogicalPostProcessUtil::postprocess(d->memDb(), d->printToConsole)) {
        return false;
    }

    return !err;
}

bool Tool::lastRunMadeChanges() const
{
    return d->lastRunMadeChanges;
}

void Tool::setHeaderLocationCallback(HeaderCallbacks::HeaderLocationCallback_f const& headerLocationCallback)
{
    d->headerLocationCallback = headerLocationCallback;
}

void Tool::setHandleCppCommentsCallback(HandleCppCommentsCallback_f const& handleCppCommentsCallback)
{
    d->handleCppCommentsCallback = handleCppCommentsCallback;
}

void Tool::cancelRun()
{
    std::unique_lock<std::mutex> lock(d->executorMutex);
    d->executorCancelled = true;

    if (!d->toolExecutor) {
        return;
    }

    d->toolExecutor->cancelRun();
}

void Tool::setShowDatabaseErrors(bool value)
{
    d->showDatabaseErrors = value;
}

bool Tool::finalizingThreads() const
{
    return d->executorCancelled;
}

void Tool::messageCallback(const std::string& message, long threadId)
{
    if (d->printToConsole) {
        qDebug() << message;
    }

    Q_EMIT messageFromThread(QString::fromStdString(message), threadId);
}

} // namespace Codethink::lvtclp
