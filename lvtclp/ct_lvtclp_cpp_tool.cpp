// ct_lvtclp_cpp_tool.cpp                                                 -*-C++-*-

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

#include "ct_lvtclp_cpp_tool_constants.h"
#include <ct_lvtclp_cpp_tool.h>

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
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include <QDebug>
#include <QElapsedTimer>

Q_LOGGING_CATEGORY(LogTool, "log.cpp_tool")

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
    std::unordered_set<std::filesystem::path> d_paths;
    std::unordered_set<std::string> d_components;
    std::unordered_set<std::string> d_pkgs;
    std::optional<std::filesystem::path> d_prefix;

  protected:
    const std::vector<std::string>& files() const
    {
        return d_files;
    }

    const std::unordered_set<std::filesystem::path>& paths() const
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
        // d_paths.shrink_to_fit();
        // d_components: no shrink_to_fit for std::unordered_set
        // d_pkgs: no shrink_to_fit for std::unordered_set
    }

    void addPath(const std::filesystem::path& path)
    // path should be weakly_canonical
    {
        using Codethink::lvtclp::FileType;

        const auto [it, inserted] = d_paths.insert(path);
        if (!inserted) {
            return;
        }

        // we store multiple copies of the path in different formats so we only
        // have to pay for the conversions once
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
    std::unordered_map<std::string, int> d_filenameToCompileCommandIdx;
    std::filesystem::path d_commonParent;
    std::filesystem::path d_buildPath;
    std::vector<llvm::GlobPattern> d_ignorePatterns;

  public:
    // CREATORS
    explicit PartialCompilationDatabase(std::filesystem::path sourcePath,
                                        std::filesystem::path buildPath,
                                        std::vector<clang::tooling::CompileCommand> compileCommands,
                                        std::function<void(const std::string&, long)> messageCallback):
        d_messageCallback(std::move(messageCallback)),
        d_compileCommands(std::move(compileCommands)),
        d_commonParent(std::move(sourcePath)),
        d_buildPath(buildPath)
    {
    }

    void setIgnoreGlobs(const std::vector<llvm::GlobPattern>& ignoreGlobs)
    {
        d_ignorePatterns = ignoreGlobs;
    }

    // MANIPULATORS
    void setup(Codethink::lvtclp::CppTool::UseSystemHeaders useSystemHeaders,
               const std::vector<std::string>& userProvidedExtraArgs,
               bool printToConsole)
    {
        std::cout << "Partial Compilation Database Setup Started" << std::endl;
        QElapsedTimer timer;
        timer.start();
        using Codethink::lvtclp::CompilerUtil;
        std::vector<std::string> sysIncludes;

        std::optional<std::string> bundledHeaders = CompilerUtil::findBundledHeaders(printToConsole);
        if (!bundledHeaders) {
            const bool searchForHeaders = useSystemHeaders == Codethink::lvtclp::CppTool::UseSystemHeaders::e_Yes
                || ((useSystemHeaders == Codethink::lvtclp::CppTool::UseSystemHeaders::e_Query)
                    && CompilerUtil::weNeedSystemHeaders());

            if (searchForHeaders) {
                // Check if we will need to run this for every compile commands.
                sysIncludes = CompilerUtil::findSystemIncludes(d_compileCommands[0].CommandLine[0]);
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

        int idx = 0;
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

            // add extra (user provided) includes
            std::copy(userProvidedExtraArgs.begin(), userProvidedExtraArgs.end(), std::back_inserter(cmd.CommandLine));

            d_filenameToCompileCommandIdx[cmd.Filename] = idx;
            idx += 1;
        }

        shrinkToFit();
        setPrefix(d_commonParent);
        std::cout << "Partial Database Setup Finished" << timer.elapsed() << std::endl;
    }

    // ACCESSORS
    const std::filesystem::path& commonParent() const
    {
        return d_commonParent;
    }

    std::vector<clang::tooling::CompileCommand> getCompileCommands(llvm::StringRef FilePath) const override
    {
        try {
            int idx = d_filenameToCompileCommandIdx.at(FilePath.str());
            return {d_compileCommands[idx]};
        } catch (...) {
            return {};
        }
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

struct CppTool::Private {
    std::optional<clang::tooling::CompileCommand> compileCommand;
    std::vector<std::filesystem::path> compileCommandsJsons;
    std::function<void(const std::string&, long)> messageCallback;
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

    ThreadStringMap pathToCanonical;

    CppToolConstants constants;

    [[nodiscard]] lvtmdb::ObjectStore& memDb()
    {
        return sharedMemDb ? *sharedMemDb : localMemDb;
    }

    Private(CppTool *tool, CppToolConstants constants, std::vector<std::filesystem::path> inCompileCommandsJsons):
        compileCommandsJsons(inCompileCommandsJsons),
        messageCallback([tool](const std::string& msg, long tid) {
            tool->messageCallback(msg, tid);
        }),
        constants(constants)
    {
        if (compilationDb) {
            compilationDb->setIgnoreGlobs(constants.ignoreGlobs);
        }
    }

    Private(CppTool *tool, CppToolConstants constants, clang::tooling::CompileCommand compileCommand):
        compileCommand(std::in_place, compileCommand),
        messageCallback([tool](const std::string& msg, long tid) {
            tool->messageCallback(msg, tid);
        }),
        constants(constants)
    {
        if (compilationDb) {
            compilationDb->setIgnoreGlobs(constants.ignoreGlobs);
        }
    }

    Private(CppTool *tool,
            CppToolConstants constants,
            const std::vector<clang::tooling::CompileCommand>& compileCommands):
        messageCallback([tool](const std::string& msg, long tid) {
            tool->messageCallback(msg, tid);
        }),
        compilationDb(std::in_place, constants.prefix, constants.buildPath, compileCommands, messageCallback),
        constants(constants)
    {
        if (compilationDb) {
            compilationDb->setIgnoreGlobs(constants.ignoreGlobs);
        }
    }
};

CppTool::CppTool(CppToolConstants constants, const std::vector<std::filesystem::path>& compileCommandsJsons):
    d(std::make_unique<CppTool::Private>(this, constants, compileCommandsJsons))
{
}

CppTool::CppTool(CppToolConstants constants, const clang::tooling::CompileCommand& compileCommand):
    d(std::make_unique<CppTool::Private>(this, constants, compileCommand))
{
}

CppTool::CppTool(CppToolConstants constants, const clang::tooling::CompilationDatabase& db):
    d(std::make_unique<CppTool::Private>(this, constants, db.getAllCompileCommands()))
{
    d->compilationDb->setup(UseSystemHeaders::e_Query,
                            constants.userProvidedExtraCompileCommandsArgs,
                            !constants.printToConsole);
}

CppTool::~CppTool() noexcept = default;

void CppTool::setSharedMemDb(std::shared_ptr<lvtmdb::ObjectStore> const& sharedMemDb)
{
    d->sharedMemDb = sharedMemDb;
}

lvtmdb::ObjectStore& CppTool::getObjectStore()
{
    return d->memDb();
}

void CppTool::setUseSystemHeaders(UseSystemHeaders value)
{
    d->useSystemHeaders = value;
}

bool CppTool::ensureSetup()
{
    if (!processCompilationDatabase()) {
        return false;
    }
    assert(d->compilationDb);

    return true;
}

bool CppTool::processCompilationDatabase()
{
    if (d->compilationDb) {
        return true;
    }
    QElapsedTimer timer;
    std::cout << "Starting Process Compilation Database" << std::endl;
    timer.start();

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
        d->compilationDb.emplace(d->constants.prefix,
                                 d->constants.buildPath,
                                 compDb.getAllCompileCommands(),
                                 d->messageCallback);
    } else if (d->compileCommand.has_value()) {
        d->compilationDb.emplace(d->constants.prefix,
                                 d->constants.buildPath,
                                 std::vector({d->compileCommand.value()}),
                                 d->messageCallback);
    } else {
        std::cerr << "No compilation database nor compilation command on the tool execution";
        return false;
    }

    d->compilationDb->setIgnoreGlobs(d->constants.ignoreGlobs);
    d->compilationDb->setup(d->useSystemHeaders,
                            d->constants.userProvidedExtraCompileCommandsArgs,
                            !d->constants.printToConsole);

    std::cout << "Process compilation database finished" << timer.elapsed() << std::endl;
    return true;
}

void CppTool::setupIncrementalUpdate(FilesystemScanner::IncrementalResult& res, bool doIncremental)
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
        if (d->constants.printToConsole) {
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

    if (d->constants.printToConsole) {
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

    if (d->constants.printToConsole) {
        qDebug() << "Files for the incremental db";
        for (const std::string& file : newPaths) {
            qDebug() << "\t " << file.c_str();
        }
    }

    d->incrementalCdb.emplace(*d->compilationDb, newPaths, d->compilationDb->commonParent());
}

bool CppTool::findPhysicalEntities(bool doIncremental)
{
    if (!ensureSetup()) {
        return false;
    }
    QElapsedTimer timer;
    std::cout << "Find Physical Entities Started" << std::endl;
    timer.start();
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

    FilesystemScanner scanner(d->memDb(), d->constants, *d->compilationDb, d->messageCallback);

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

    std::cout << "Find Physical Entities Finished" << timer.elapsed() << std::endl;
    return true;
}

bool CppTool::runPhysical(bool skipScan)
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
        if (d->constants.printToConsole) {
            qDebug() << "Processing file " << path;
        }
        Q_EMIT processingFileNotification(QString::fromStdString(path));
    };

    auto messageCallback = [this](const std::string& message, long threadId) {
        if (d->constants.printToConsole) {
            qDebug() << message;
        }
        Q_EMIT messageFromThread(QString::fromStdString(message), threadId);
    };

    std::unique_ptr<clang::tooling::FrontendActionFactory> dependencyScanner =
        std::make_unique<DepScanActionFactory>(d->memDb(),
                                               d->constants,
                                               filenameCallback,
                                               d->pathToCanonical,
                                               d->headerLocationCallback);

    Q_EMIT aboutToCallClangNotification(tr("Physical Parse"), d->incrementalCdb->numCompileCommands());

    d->toolExecutor = new ToolExecutor(*d->incrementalCdb, d->constants.numThreads, messageCallback, d->memDb());

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

bool CppTool::runFull(bool skipPhysical)
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
        if (d->constants.printToConsole) {
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
                                                  d->constants,
                                                  filenameCallback,
                                                  messageOpt,
                                                  d->handleCppCommentsCallback);

    Q_EMIT aboutToCallClangNotification(tr("Logical Parse"), d->incrementalCdb->numCompileCommands());

    d->toolExecutor = new ToolExecutor(*d->incrementalCdb, d->constants.numThreads, d->messageCallback, d->memDb());

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

        std::ignore = llvm::handleErrors(std::move(err), [&](llvm::StringError const& err) -> llvm::Error {
            Q_EMIT messageFromThread(QString::fromStdString(err.getMessage()), 0);
            return llvm::Error::success();
        });
        if (d->constants.printToConsole) {
            qDebug() << "We got a logical error, aborting";
        }
        return false;
    }

    if (!cancelled) {
        d->memDb().setState(lvtmdb::ObjectStore::State::AllReady);
    }
    // else keep it the same because physical info is probably okay

    if (!cancelled) {
        if (d->constants.printToConsole) {
            qDebug() << "Collating output database";
        }
    }

    if (!LogicalPostProcessUtil::postprocess(d->memDb(), d->constants.printToConsole)) {
        return false;
    }

    return !err;
}

bool CppTool::lastRunMadeChanges() const
{
    return d->lastRunMadeChanges;
}

void CppTool::setHeaderLocationCallback(HeaderCallbacks::HeaderLocationCallback_f const& headerLocationCallback)
{
    d->headerLocationCallback = headerLocationCallback;
}

void CppTool::setHandleCppCommentsCallback(HandleCppCommentsCallback_f const& handleCppCommentsCallback)
{
    d->handleCppCommentsCallback = handleCppCommentsCallback;
}

void CppTool::cancelRun()
{
    std::unique_lock<std::mutex> lock(d->executorMutex);
    d->executorCancelled = true;

    if (!d->toolExecutor) {
        return;
    }

    d->toolExecutor->cancelRun();
}

void CppTool::setShowDatabaseErrors(bool value)
{
    d->showDatabaseErrors = value;
}

bool CppTool::finalizingThreads() const
{
    return d->executorCancelled;
}

void CppTool::messageCallback(const std::string& message, long threadId)
{
    if (d->constants.printToConsole) {
        qDebug() << message;
    }

    Q_EMIT messageFromThread(QString::fromStdString(message), threadId);
}

} // namespace Codethink::lvtclp

#include "moc_ct_lvtclp_cpp_tool.cpp"
