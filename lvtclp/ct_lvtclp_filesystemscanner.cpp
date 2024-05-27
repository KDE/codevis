// ct_lvtclp_filesystemscanner.cpp                                   -*-C++-*-

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

#include <ct_lvtclp_filesystemscanner.h>

#include <ct_lvtmdb_componentobject.h>
#include <ct_lvtmdb_errorobject.h>
#include <ct_lvtmdb_fileobject.h>
#include <ct_lvtmdb_objectstore.h>
#include <ct_lvtmdb_packageobject.h>
#include <ct_lvtmdb_repositoryobject.h>
#include <ct_lvtmdb_util.h>

#include <ct_lvtshr_stringhelpers.h>

#include <ct_lvtclp_clputil.h>
#include <ct_lvtclp_componentutil.h>
#include <ct_lvtclp_fileutil.h>

#include <QDebug>
#include <QDir>

#include <llvm/Support/FileSystem.h>

#include <cassert>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace {

struct FoundPackage {
    std::string parent;
    std::string qualifiedName;
    std::string filePath;
    std::string repositoryName;
};

struct FoundThing {
    // something with a qualified name and a parent (referred to by qualified name)
    std::string parent;
    std::string qualifiedName;
    std::string filePath;
};

struct FoundThingHash {
    std::size_t operator()(FoundThing const& thing) const
    {
        return std::hash<std::string>{}(thing.parent + thing.qualifiedName);
    }
};

} // namespace

namespace Codethink::lvtclp {

struct FilesystemScanner::Private {
    lvtmdb::ObjectStore& memDb;
    // Memory database
    std::filesystem::path prefix;
    // common prefix of paths

    std::filesystem::path buildPath;
    // where our project is being build
    // needed in case of sources generated as build artifacts

    const LvtCompilationDatabase& cdb;
    // compilation database used so that we don't add things deliberately
    // configured out of the build
    std::vector<std::filesystem::path> nonLakosianDirs;

    std::vector<FoundThing> foundFiles;

    // from comparing the files we found to the files in the database we can
    // figure out which files are new and which are deleted
    std::vector<FoundPackage> foundPkgs;
    std::unordered_set<std::string> foundPkgNames;
    std::unordered_map<std::string, PackageHelper> foundPkgGrps;
    std::vector<RepositoryHelper> foundRepositories;

    // compare with the set of packages in the database to see which packages
    // were added and removed
    // Package groups and packages are separated so we can add all of the
    // package groups first and then be sure we have all the parents in place
    // when adding packages

    std::function<void(const std::string&, long)> messageCallback;
    bool catchCodeAnalysisOutput;

    std::vector<llvm::GlobPattern> ignoreGlobs;

    bool enableLakosianRules;

    explicit Private(lvtmdb::ObjectStore& memDb,
                     std::filesystem::path prefix,
                     std::filesystem::path buildPath,
                     const LvtCompilationDatabase& cdb,
                     std::function<void(const std::string, long)> messageCallback,
                     std::vector<std::filesystem::path> nonLakosianDirs,
                     bool catchCodeAnalysisOutput,
                     std::vector<llvm::GlobPattern> ignoreGlobs,
                     bool enableLakosianRules):
        memDb(memDb),
        prefix(std::move(prefix)),
        buildPath(buildPath),
        cdb(cdb),
        nonLakosianDirs(std::move(nonLakosianDirs)),
        messageCallback(std::move(messageCallback)),
        catchCodeAnalysisOutput(catchCodeAnalysisOutput),
        ignoreGlobs(std::move(ignoreGlobs)),
        enableLakosianRules(enableLakosianRules)
    {
    }
};

FilesystemScanner::FilesystemScanner(lvtmdb::ObjectStore& memDb,
                                     const std::filesystem::path& prefix,
                                     const std::filesystem::path& buildPath,
                                     const LvtCompilationDatabase& cdb,
                                     std::function<void(const std::string&, long)> messageCallback,
                                     bool catchCodeAnalysisOutput,
                                     std::vector<std::filesystem::path> nonLakosianDirs,
                                     std::vector<llvm::GlobPattern> ignoreGlobs,
                                     bool enableLakosianRules):
    d(std::make_unique<FilesystemScanner::Private>(memDb,
                                                   prefix,
                                                   buildPath,
                                                   cdb,
                                                   std::move(messageCallback),
                                                   std::move(nonLakosianDirs),
                                                   catchCodeAnalysisOutput,
                                                   std::move(ignoreGlobs),
                                                   enableLakosianRules))
{
}

FilesystemScanner::~FilesystemScanner() noexcept = default;

FilesystemScanner::IncrementalResult FilesystemScanner::scanCompilationDb()
{
    for (const std::string& string : d->cdb.getAllFiles()) {
        const std::filesystem::path path(string);
        scanPath(path);
        scanHeader(path);
    }

    return addToDatabase();
}

void FilesystemScanner::scanHeader(const std::filesystem::path& path)
{
    // the compilation database only contains .cpp files
    // but we need to discover changes to headers as well so we can calculate,
    // the incrmental update to-do list before doing a real physical scan (which
    // actually processes the #include directives in .cpp files). To always be
    // correct for non-lakosian code we would have to do a full (non-incremental)
    // physical scan to find all headers, then use all of these to calculate the
    // incremental update todo list. That would dramatically slow down tool usage
    // on lakosian code. Instead we will try to guess header paths here without
    // looking at #include directives.
    //
    // For lakosian style code, headers will be in the same directory as the
    // .cpp file, but lots of other code will put headers in another include
    // directory. We can discover include directories from the compiler arguments
    // in the compilation database.
    //
    // We will assume that the header file has the same name as the .cpp file
    // but with a header extension. Most non-lakosian code will follow this
    // convention anyway, and using components is the bare minimum of lakosian
    // design.
    static const std::vector<std::string> headerExtensions({".h", ".hh", ".h++", ".hpp"});

    const std::filesystem::path parent = std::filesystem::weakly_canonical(path.parent_path());
    const std::filesystem::path stem = path.stem();

    const std::vector<clang::tooling::CompileCommand> compileCommands = d->cdb.getCompileCommands(path.string());

    std::vector<std::filesystem::path> includeDirectories;
    includeDirectories.emplace_back(parent); // for lakosian code

    // collect include directories for this file
    for (const clang::tooling::CompileCommand& cmd : compileCommands) {
        for (const std::string& arg : cmd.CommandLine) {
            // check for arguments like -Isome/path
            if (arg.size() > 2 && arg[0] == '-' && arg[1] == 'I') {
                // remove the -I
                const std::string includeDirStr = arg.substr(2);

                std::filesystem::path includeDir(includeDirStr);
                if (includeDir.is_relative()) {
                    // the include path is relative to the source file e.g.
                    // -I../../groups/foo/foobar
                    includeDir = parent / includeDir;
                }

                if (std::filesystem::is_directory(includeDir)) {
                    includeDir = std::filesystem::canonical(includeDir);

                    auto it = std::find(includeDirectories.begin(), includeDirectories.end(), includeDir);
                    if (it == includeDirectories.end()) {
                        includeDirectories.emplace_back(std::move(includeDir));
                    }
                }
            }
        }
    }

    // look in the include directories for a matching header
    for (const std::filesystem::path& includeDir : includeDirectories) {
        for (const std::string& ext : headerExtensions) {
            std::filesystem::path headerPath = (includeDir / stem).concat(ext);
            if (std::filesystem::exists(headerPath)) {
                // we found the header!
                scanPath(headerPath);
                return;
            }
        }
    }
}

bool FilesystemScanner::tryProcessFileUsingSemanticRules(const std::filesystem::path& path)
{
    auto addPkgFromSemanticRulesProcessing = [this](std::string const& qualifiedName,
                                                    std::optional<std::string> parentQualifiedName = std::nullopt,
                                                    std::optional<std::string> repositoryName = std::nullopt,
                                                    std::optional<std::string> path = std::nullopt) {
        if (d->foundPkgNames.count(qualifiedName) > 0) {
            return;
        }

        if (repositoryName) {
            d->foundRepositories.emplace_back(RepositoryHelper{*repositoryName, ""});
        }

        if (parentQualifiedName) {
            if (d->foundPkgNames.count(*parentQualifiedName) == 0) {
                d->foundPkgs.emplace_back(
                    FoundPackage{"", *parentQualifiedName, "", repositoryName ? *repositoryName : ""});
                d->foundPkgNames.insert(*parentQualifiedName);
            }
        }

        d->foundPkgs.emplace_back(FoundPackage{parentQualifiedName ? *parentQualifiedName : "",
                                               qualifiedName,
                                               path ? *path : "",
                                               repositoryName ? *repositoryName : ""});
        d->foundPkgNames.insert(qualifiedName);
    };

    auto filePathQString = QString::fromStdString(path.string());
    auto fullFilePath = QDir::fromNativeSeparators(filePathQString).toStdString();
    for (auto const& semanticPackingRule : ClpUtil::getAllSemanticPackingRules()) {
        if (semanticPackingRule->accept(fullFilePath)) {
            auto pkg = semanticPackingRule->process(fullFilePath, addPkgFromSemanticRulesProcessing);
            addSourceFile(path, pkg);
            return true;
        }
    }
    return false;
}

void FilesystemScanner::processFileUsingLakosianRules(const std::filesystem::path& path)
{
    if (ClpUtil::isComponentOnStandalonePackage(path)) {
        const auto pkgPath = path.parent_path();
        const auto pkg = addLakosianSourcePackage(pkgPath, "", true);
        addSourceFile((pkgPath / path.filename()).string(), pkg);
    } else if (ClpUtil::isComponentOnPackageGroup(path)) {
        const auto pkgPath = path.parent_path();
        const auto pkgGrpPath = pkgPath.parent_path();
        const auto pkgGrp = addLakosianSourcePackage(pkgGrpPath, "", false);
        const auto pkg = addLakosianSourcePackage(pkgPath, pkgGrp, false);
        addSourceFile((pkgPath / path.filename()).string(), pkg);
    } else {
        const static std::string nonLakosianGroup(ClpUtil::NON_LAKOSIAN_GROUP_NAME);
        if (!d->foundPkgGrps.count(nonLakosianGroup)) {
            d->foundPkgGrps[nonLakosianGroup] = PackageHelper{"", nonLakosianGroup, std::string{}};
        }
        const auto pkgPath = path.parent_path();
        const auto pkg = addLakosianSourcePackage(pkgPath, ClpUtil::NON_LAKOSIAN_GROUP_NAME, false);
        if (!pkg.empty()) {
            addSourceFile(path, pkg);
        } else {
            addSourceFile(path, nonLakosianGroup);
        }
    }
}

void FilesystemScanner::scanPath(const std::filesystem::path& path)
{
    if (!std::filesystem::is_regular_file(path)) {
        return;
    }

    if (ClpUtil::isFileIgnored(path.filename().string(), d->ignoreGlobs)) {
        return;
    }

    if (tryProcessFileUsingSemanticRules(path)) {
        return;
    }

    if (d->enableLakosianRules) {
        processFileUsingLakosianRules(path);
    } else {
        nonLakosian::ClpUtil::writeSourceFile(d->memDb,
                                              path.string(),
                                              d->prefix.string(),
                                              d->buildPath.string(),
                                              d->prefix.string());
    }
}

void FilesystemScanner::addSourceFile(const std::filesystem::path& path, const std::string& package)
{
    d->foundFiles.push_back({package, path.string(), std::string{}});
}

std::string FilesystemScanner::addLakosianSourcePackage(const std::filesystem::path& path,
                                                        const std::string& inParent,
                                                        bool isStandalone)
{
    std::string parent = inParent;

    if (!std::filesystem::is_directory(path)) {
        return {};
    }

    const std::filesystem::path normalisedPath = ClpUtil::normalisePath(path, d->prefix);
    if (normalisedPath.empty()) {
        return {};
    }

    std::filesystem::path fullPath;
    if (normalisedPath.is_relative()) {
        fullPath = d->prefix / normalisedPath;
    } else {
        fullPath = normalisedPath;
    }

    // check if this path is explicitly flagged as non-lakosian
    for (const std::filesystem::path& nonLakosianDir : d->nonLakosianDirs) {
        if (FileUtil::pathStartsWith(nonLakosianDir, fullPath)) {
            const static std::string nonLakosianGroup(ClpUtil::NON_LAKOSIAN_GROUP_NAME);
            d->foundPkgGrps[nonLakosianGroup] = PackageHelper{"", nonLakosianGroup, std::string{}};
            parent = nonLakosianGroup;
            break;
        }
    }

    std::string qualifiedName = normalisedPath.string();
    if (!isStandalone && parent.empty()) {
        if (!d->foundPkgNames.count(qualifiedName)) {
            auto filePath = QString::fromStdString(path.string());
            auto projectSource = QString::fromStdString(d->prefix.string());
            if (filePath.startsWith(projectSource)) {
                filePath.replace(projectSource, "${SOURCE_DIR}/");
            }
            d->foundPkgGrps[qualifiedName] = PackageHelper{"", qualifiedName, filePath.toStdString()};
        }
    } else {
        if (d->foundPkgGrps.count(qualifiedName)) {
            // we already added this without a parent. Get rid of that because
            // we now have a parent
            d->foundPkgGrps.erase(qualifiedName);
        }
        auto filePath = QString::fromStdString(path.string());
        auto projectSource = QString::fromStdString(d->prefix.string());
        if (filePath.startsWith(projectSource)) {
            filePath.replace(projectSource, "${SOURCE_DIR}/");
        }
        d->foundPkgs.emplace_back(FoundPackage{parent, qualifiedName, filePath.toStdString(), ""});
        d->foundPkgNames.insert(std::move(qualifiedName));
    }

    return normalisedPath.string();
}

lvtmdb::PackageObject *FilesystemScanner::addPackage(IncrementalResult& out,
                                                     std::unordered_set<lvtmdb::PackageObject *>& existingPkgs,
                                                     const std::string& qualifiedName,
                                                     const std::string& parentName,
                                                     const std::string& filePath,
                                                     const std::string& repositoryName)
{
    lvtmdb::PackageObject *pkg = d->memDb.getPackage(qualifiedName);
    if (pkg) {
        existingPkgs.insert(pkg);
        return pkg;
    }

    const std::filesystem::path path(qualifiedName);
    std::string name = path.filename().string();

    lvtmdb::PackageObject *parent = nullptr;
    if (!parentName.empty()) {
        // we can't recurse in the common case, otherwise existingPkgs
        // would get false positives
        parent = d->memDb.getPackage(parentName);
        if (!parent) {
            // assumes parent has no parent (it is a package group)
            parent = addPackage(out, existingPkgs, parentName, std::string{}, filePath, repositoryName);
        }
        assert(parent);
    }

    lvtmdb::RepositoryObject *repo = d->memDb.getOrAddRepository(repositoryName, "");

    out.newPkgs.push_back(qualifiedName);
    lvtmdb::PackageObject *thisPkg = d->memDb.getOrAddPackage(qualifiedName, std::move(name), filePath, parent, repo);

    if (repo) {
        repo->withRWLock([&] {
            repo->addChild(thisPkg);
        });
    }

    if (parent) {
        parent->withRWLock([&] {
            parent->addChild(thisPkg);
        });
    }

    return thisPkg;
}

FilesystemScanner::IncrementalResult FilesystemScanner::addToDatabase()
{
    auto lock = d->memDb.rwLock();

    IncrementalResult out;

    // track what already existed so we can find deleted things
    const std::vector<lvtmdb::PackageObject *> allDbPkgs = d->memDb.getAllPackages();
    const std::vector<lvtmdb::FileObject *> allDbFiles = d->memDb.getAllFiles();
    std::unordered_set<lvtmdb::PackageObject *> existingPkgs;
    std::unordered_set<lvtmdb::FileObject *> existingFiles;

    // add repositories
    for (const auto& helper : d->foundRepositories) {
        (void) d->memDb.getOrAddRepository(helper.qualifiedName, helper.path);
    }

    // add package groups
    for (const auto& [name, helper] : d->foundPkgGrps) {
        addPackage(out, existingPkgs, name, std::string{}, helper.filePath, helper.parentRepositoryName);
    }

    // add packages
    for (auto const& pkg : d->foundPkgs) {
        addPackage(out, existingPkgs, pkg.qualifiedName, pkg.parent, pkg.filePath, pkg.repositoryName);
    }

    // add files
    for (const FoundThing& file : d->foundFiles) {
        lvtmdb::PackageObject *parent = d->memDb.getPackage(file.parent);
        assert(parent || file.parent.empty());

        const std::filesystem::path path = ClpUtil::normalisePath(file.qualifiedName, d->prefix).string();

        std::filesystem::path fullPath = d->prefix / path;
        auto hash = [&fullPath]() -> std::string {
            auto result = llvm::sys::fs::md5_contents(fullPath.string());
            if (result) {
                return result.get().digest().str().str();
            }

            // allow failure to hash file contents because we use memory mapped files in tests
            return "";
        }();

        lvtmdb::FileObject *filePtr = d->memDb.getFile(path.string());
        if (!filePtr) {
            const FileType type = ClpUtil::categorisePath(path.string());
            bool isHeader;
            if (type == FileType::e_Header) {
                isHeader = true;
            } else if (type == FileType::e_Source) {
                isHeader = false;
            } else if (type == FileType::e_KnownUnknown) {
                continue;
            } else { // type == FileType::e_UnknownUnknown
                d->memDb.getOrAddError(lvtmdb::MdbUtil::ErrorKind::ParserError,
                                       "",
                                       "Unknown file extension",
                                       path.string());
                continue;
            }

            // create or fetch the component for this file
            lvtmdb::ComponentObject *comp = ComponentUtil::addComponent(path, parent, d->memDb);

            filePtr =
                d->memDb.getOrAddFile(path.string(), path.filename().string(), isHeader, std::move(hash), parent, comp);
            out.newFiles.push_back(path.string());
            comp->withRWLock([&] {
                comp->addFile(filePtr);
            });
            parent->withRWLock([&] {
                parent->addComponent(comp);
            });

        } else {
            existingFiles.insert(filePtr);

            // allowing !inCdb when it ended up in the database anyway
            // (for example via an #include)

            filePtr->withRWLock([&] {
                if (hash != filePtr->hash()) {
                    if (d->catchCodeAnalysisOutput) {
                        qDebug() << "Found modified file " << path.string();
                    }
                    out.modifiedFiles.push_back(filePtr->qualifiedName());
                    filePtr->setHash(std::move(hash));
                }
            });
        }
    }

    // which packages and package groups were deleted?
    for (lvtmdb::PackageObject *pkg : allDbPkgs) {
        std::string qualifiedName;
        pkg->withROLock([&] {
            qualifiedName = pkg->qualifiedName();
        });
        if (!d->cdb.containsPackage(qualifiedName)) {
            // that package isn't in the code database so we will never find it
            // in the scan anyway
            continue;
        }
        const auto it = existingPkgs.find(pkg);
        if (it == existingPkgs.end()) {
            if (d->catchCodeAnalysisOutput) {
                qDebug() << "Package deleted: " << qualifiedName;
            }
            out.deletedPkgs.push_back(qualifiedName);
        }
    }

    // which files were deleted?
    for (lvtmdb::FileObject *file : allDbFiles) {
        std::string qualifiedName;
        file->withROLock([&] {
            qualifiedName = file->qualifiedName();
        });
        if (!d->cdb.containsFile(qualifiedName)) {
            // we never find system paths in existing files because this
            // filesystem visitor does not traverse them (causing us to always
            // think system files have been deleted)
            continue;
        }

        const auto it = existingFiles.find(file);
        if (it == existingFiles.end()) {
            file->withROLock([&] {
                if (d->catchCodeAnalysisOutput) {
                    qDebug() << "File deleted: " << file->qualifiedName();
                }
                out.deletedFiles.push_back(file->qualifiedName());
            });
        }
    }

    // we've processed everything we added. Clear everything so we are ready for
    // the next scan
    d->foundFiles.clear();
    d->foundPkgs.clear();
    d->foundPkgGrps.clear();

    return out;
}

} // namespace Codethink::lvtclp
