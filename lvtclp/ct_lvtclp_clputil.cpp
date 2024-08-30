// ct_lvtclp_clputil.cpp                                              -*-C++-*-

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

#include <ct_lvtclp_clputil.h>

#include <ct_lvtclp_componentutil.h>

#include <ct_lvtmdb_componentobject.h>
#include <ct_lvtmdb_fileobject.h>
#include <ct_lvtmdb_functionobject.h>
#include <ct_lvtmdb_objectstore.h>
#include <ct_lvtmdb_packageobject.h>
#include <ct_lvtmdb_typeobject.h>

#include <ct_lvtclp_fileutil.h>
#include <ct_lvtshr_stringhelpers.h>

#include <clang/Tooling/JSONCompilationDatabase.h>
#include <filesystem>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/GlobPattern.h>
#include <pybind11/embed.h>
#include <pybind11/functional.h>
#include <pybind11/stl.h>

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QtGlobal>

#include <algorithm>
#include <memory>
#include <regex>
#include <set>
#include <thread>

namespace {

using namespace Codethink;

template<typename T>
std::set<T> getUnion(const std::set<T>& a, const std::set<T>& b)
{
    std::set<T> result = a;
    result.insert(b.begin(), b.end());
    return result;
}

const std::set<std::string> headerExtensions({".h", ".hh", ".h++", ".hpp", ".H"});
const std::set<std::string> sourceExtensions({".cpp", ".c", ".C", ".c++", ".cc", ".cxx", ".t.cpp", ".moc"});
const std::set<std::string> otherExtensions({".dep", ".mem", ".o", ".swp", ".md", ".txt", ""});
const std::set<std::string> allCppExtensions = getUnion(headerExtensions, sourceExtensions);

lvtmdb::PackageObject *getSourcePackage(const std::string& qualifiedName,
                                        std::string name,
                                        std::string diskPath,
                                        lvtmdb::ObjectStore& memDb,
                                        lvtmdb::PackageObject *parent,
                                        lvtmdb::RepositoryObject *repository)
// assumes memDb is already locked for writing
{
    if (qualifiedName.empty()) {
        return nullptr;
    }

    return memDb.getOrAddPackage(qualifiedName, std::move(name), std::move(diskPath), parent, repository);
}

/**
 * Visual studio was giving compiler error with local lambda function. In order to solve the issue, the lambda has been
 * extracted to this struct - It is meant to be used in `getPackageForPath` only.
 */
struct addPkgForSemPackRuleHelper {
    explicit addPkgForSemPackRuleHelper(lvtmdb::ObjectStore& memDb): memDb(memDb)
    {
    }

    void operator()(std::string const& qualifiedName,
                    std::optional<std::string> parentQualifiedName = std::nullopt,
                    std::optional<std::string> repositoryName = std::nullopt,
                    std::optional<std::string> path = std::nullopt)
    {
        auto *repository = repositoryName ? memDb.getOrAddRepository(*repositoryName, "") : nullptr;
        auto *parent = parentQualifiedName
            ? memDb.getOrAddPackage(*parentQualifiedName, *parentQualifiedName, "", nullptr, repository)
            : nullptr;
        memDb.getOrAddPackage(qualifiedName, qualifiedName, path ? *path : "", parent, repository);
    }

  private:
    lvtmdb::ObjectStore& memDb;
};

lvtmdb::PackageObject *getPackageForPath(const std::filesystem::path& path,
                                         lvtmdb::ObjectStore& memDb,
                                         const std::filesystem::path& prefix,
                                         const std::vector<std::filesystem::path>& nonLakosianDirs,
                                         const std::vector<std::pair<std::string, std::string>>& thirdPartyDirs)
// assumes path is already normalised using ClpUtil::normalisePath
// assumes memDb is already locked for writing
{
    using namespace Codethink::lvtclp;

    // synthesise package from the directory containing the source file
    const std::filesystem::path pkgPath = path.parent_path();

    lvtmdb::PackageObject *pkg = memDb.getPackage(pkgPath.string());
    if (pkg) {
        return pkg;
    }

    auto addPkg = addPkgForSemPackRuleHelper{memDb};
    auto fullFilePathQString = QString::fromStdString((prefix / path).string());
    auto fullFilePath = QDir::fromNativeSeparators(fullFilePathQString).toStdString();
    for (auto&& semanticPackingRule : ClpUtil::getAllSemanticPackingRules()) {
        if (semanticPackingRule->accept(fullFilePath)) {
            auto pkgQName = semanticPackingRule->process(fullFilePath, addPkg);
            return memDb.getPackage(pkgQName);
        }
    }

    const std::filesystem::path fullPkgPath = prefix / pkgPath;
    bool nonLakosian = false;
    for (const std::filesystem::path& nonLakosianDir : nonLakosianDirs) {
        if (FileUtil::pathStartsWith(nonLakosianDir, fullPkgPath)) {
            nonLakosian = true;
            break;
        }
    }

    std::string topLevelPkgQualifiedName;
    std::string topLevelPkgName;
    auto filePath = QString::fromStdString(fullPkgPath.string());

    bool isMapped = false;
    for (auto const& [mappedPathRegex, mappedGroupName] : thirdPartyDirs) {
        if (std::regex_search(path.generic_string(), std::regex{mappedPathRegex})) {
            topLevelPkgQualifiedName = mappedGroupName;
            topLevelPkgName = mappedGroupName;
            isMapped = true;
            break;
        }
    }

    auto isStandalonePkg = false;
    if (!isMapped) {
        if (nonLakosian) {
            topLevelPkgQualifiedName = ClpUtil::NON_LAKOSIAN_GROUP_NAME;
            topLevelPkgName = ClpUtil::NON_LAKOSIAN_GROUP_NAME;
        } else {
            auto projectSource = QString::fromStdString(prefix.string());
            isStandalonePkg = ClpUtil::isComponentOnStandalonePackage(path);
            if (isStandalonePkg) {
                topLevelPkgQualifiedName = ("standalones" / pkgPath.filename()).string();
                topLevelPkgName = pkgPath.filename().string();

                if (filePath.startsWith(projectSource)) {
                    filePath.replace(projectSource, "${SOURCE_DIR}/");
                }
            } else if (ClpUtil::isComponentOnPackageGroup(path)) {
                topLevelPkgQualifiedName = ("groups" / pkgPath.parent_path().filename()).generic_string();
                topLevelPkgName = pkgPath.parent_path().filename().generic_string();

                if (filePath.startsWith(projectSource)) {
                    filePath.replace(projectSource, "${SOURCE_DIR}/");
                }
            } else {
                topLevelPkgQualifiedName = lvtclp::ClpUtil::NON_LAKOSIAN_GROUP_NAME;
                topLevelPkgName = lvtclp::ClpUtil::NON_LAKOSIAN_GROUP_NAME;
            }
        }
    }

    if (isStandalonePkg) {
        return getSourcePackage(topLevelPkgQualifiedName,
                                std::move(topLevelPkgName),
                                filePath.toStdString(),
                                memDb,
                                nullptr,
                                nullptr);
    }

    // Either package inside a group or non-lakosian package
    auto *grp = getSourcePackage(topLevelPkgQualifiedName,
                                 std::move(topLevelPkgName),
                                 filePath.toStdString(),
                                 memDb,
                                 nullptr,
                                 nullptr);
    if (pkgPath.filename().string().empty()) {
        return grp;
    }

    return getSourcePackage(topLevelPkgQualifiedName + "/" + pkgPath.filename().string(),
                            pkgPath.filename().string(),
                            filePath.toStdString(),
                            memDb,
                            grp,
                            nullptr);
}

} // namespace

namespace Codethink::lvtclp {

std::filesystem::path ClpUtil::normalisePath(std::filesystem::path path, std::filesystem::path prefix)
{
    if (FileUtil::pathStartsWith(prefix, path)) {
        path = FileUtil::nonPrefixPart(prefix, path);
    }

    return path;
}

lvtmdb::FileObject *ClpUtil::writeSourceFile(const std::string& inFilename,
                                             bool isHeader,
                                             lvtmdb::ObjectStore& memDb,
                                             const std::filesystem::path& prefix,
                                             const std::vector<std::filesystem::path>& nonLakosianDirs,
                                             const std::vector<std::pair<std::string, std::string>>& thirdPartyDirs)
{
    if (inFilename.empty()) {
        return nullptr;
    }
    const std::filesystem::path path = normalisePath(inFilename, prefix);
    const std::string filename = path.string();

    lvtmdb::FileObject *ret = nullptr;
    memDb.withROLock([&] {
        ret = memDb.getFile(filename);
    });
    if (ret) {
        return ret;
    }

    const std::filesystem::path fullPath = prefix / path;
    auto hash = [&fullPath]() -> std::string {
        auto result = llvm::sys::fs::md5_contents(fullPath.string());
        if (result) {
            return result.get().digest().str().str();
        }

        // allow failure to hash file contents because we use memory mapped files in tests
        return {};
    }();

    auto file = memDb.withRWLock([&]() {
        auto *package = getPackageForPath(path, memDb, prefix, nonLakosianDirs, thirdPartyDirs);
        auto *component = ComponentUtil::addComponent(path, package, memDb);
        auto *file = memDb.getOrAddFile(filename, path.filename().string(), isHeader, hash, package, component);

        component->withRWLock([&] {
            component->addFile(file);
        });
        package->withRWLock([&] {
            package->addComponent(component);
        });

        return file;
    });

    return file;
}

std::string ClpUtil::getRealPath(const clang::SourceLocation& loc, const clang::SourceManager& mgr)
{
    auto pathFromLocation = [](const clang::SourceLocation& loc,
                               const clang::SourceManager& mgr) -> std::filesystem::path {
        const clang::FileID id = mgr.getFileID(loc);
        const clang::FileEntry *entry = mgr.getFileEntryForID(id);

        std::filesystem::path filePath;
        if (entry) {
            filePath = entry->tryGetRealPathName().str();
        }

        return filePath;
    };

    std::filesystem::path res = pathFromLocation(mgr.getSpellingLoc(loc), mgr);
    if (res.empty()) {
        res = pathFromLocation(mgr.getExpansionLoc(loc), mgr);
    }

    return std::filesystem::weakly_canonical(res).generic_string();
}

void ClpUtil::writeDependencyRelations(lvtmdb::PackageObject *source, lvtmdb::PackageObject *target)
{
    if (!source || !target || source == target) {
        return; // RETURN
    }
    lvtmdb::PackageObject::addDependency(source, target);
}

void ClpUtil::addUsesInInter(lvtmdb::TypeObject *source, lvtmdb::TypeObject *target)
{
    if (!source || !target || source == target) {
        return;
    }
    lvtmdb::TypeObject::addUsesInTheInterface(source, target);
}

void ClpUtil::addUsesInImpl(lvtmdb::TypeObject *source, lvtmdb::TypeObject *target)
{
    if (!source || !target || source == target) {
        return;
    }
    lvtmdb::TypeObject::addUsesInTheImplementation(source, target);
}

void ClpUtil::addFnDependency(lvtmdb::FunctionObject *source, lvtmdb::FunctionObject *target)
{
    if (!source || !target || source == target) {
        return;
    }
    lvtmdb::FunctionObject::addDependency(source, target);
}

FileType ClpUtil::categorisePath(const std::string& file)
{
    const std::filesystem::path path(file);
    const std::string ext = path.extension().string();

    if (headerExtensions.count(ext)) {
        return FileType::e_Header;
    }
    if (sourceExtensions.count(ext)) {
        return FileType::e_Source;
    }
    if (otherExtensions.count(ext)) {
        return FileType::e_UnknownUnknown;
    }
    return FileType::e_KnownUnknown;
}

const char *const ClpUtil::NON_LAKOSIAN_GROUP_NAME = "non-lakosian group";

struct CombinedCompilationDatabase::Private {
    std::vector<clang::tooling::CompileCommand> compileCommands;
    std::vector<std::string> files;
};

CombinedCompilationDatabase::CombinedCompilationDatabase(): d(std::make_unique<Private>())
{
}

CombinedCompilationDatabase::~CombinedCompilationDatabase() noexcept = default;

cpp::result<bool, CompilationDatabaseError>
CombinedCompilationDatabase::addCompilationDatabase(const std::filesystem::path& path)
{
    QElapsedTimer timer;
    std::cout << "Add Compilation Database Started" << std::endl;
    timer.start();
    std::string errorMessage;
    std::unique_ptr<clang::tooling::JSONCompilationDatabase> jsonDb =
        clang::tooling::JSONCompilationDatabase::loadFromFile(path.string(),
                                                              errorMessage,
                                                              clang::tooling::JSONCommandLineSyntax::AutoDetect);
    if (!jsonDb) {
        return cpp::fail(CompilationDatabaseError{CompilationDatabaseError::Kind::ErrorLoadingFromFile, errorMessage});
    }

    if (jsonDb->getAllFiles().size() == 0) {
        return cpp::fail(CompilationDatabaseError{CompilationDatabaseError::Kind::CompileCommandsContainsNoFiles});
    }

    std::vector<clang::tooling::CompileCommand> compileCommands = jsonDb->getAllCompileCommands();
    if (compileCommands.size() == 0) {
        return cpp::fail(CompilationDatabaseError{CompilationDatabaseError::Kind::CompileCommandsContainsNoCommands});
    }

    const std::filesystem::path buildDir = path.parent_path();
    addCompilationDatabase(compileCommands, buildDir);
    std::cout << "Add Compilation Database finished" << timer.elapsed() << std::endl;
    return {};
}

void CombinedCompilationDatabase::addCompilationDatabase(std::vector<clang::tooling::CompileCommand>& compileCommands,
                                                         const std::filesystem::path& buildDir)
{
    for (clang::tooling::CompileCommand& cmd : compileCommands) {
        // resolve any relative paths
        std::filesystem::path filename(cmd.Filename);
        if (filename.is_relative()) {
            filename = buildDir / filename;
        }
        cmd.Filename = filename.string();

        auto ext = filename.extension().string();
        if (!allCppExtensions.contains(ext)) {
            continue;
        }

        d->files.push_back(cmd.Filename);
        d->compileCommands.push_back(std::move(cmd));
    }
}

std::vector<clang::tooling::CompileCommand>
CombinedCompilationDatabase::getCompileCommands(llvm::StringRef FilePath) const
{
    for (auto const& cmd : d->compileCommands) {
        if (cmd.Filename == FilePath) {
            return {cmd};
        }
    }
    return {};
}

std::vector<std::string> CombinedCompilationDatabase::getAllFiles() const
{
    return d->files;
}

std::vector<clang::tooling::CompileCommand> CombinedCompilationDatabase::getAllCompileCommands() const
{
    return d->compileCommands;
}

long ClpUtil::getThreadId()
{
    return static_cast<long>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
}

bool ClpUtil::isComponentOnPackageGroup(const std::filesystem::path& componentPath)
{
    // Check if the component name starts with the package name, and if the package name starts with the package
    // group name. Those are the basic rules for a component be within a package that's inside a package group.
    auto componentName = QString::fromStdString(componentPath.filename().string());
    auto pkgName = QString::fromStdString(componentPath.parent_path().filename().string());
    auto pkgGroupName = QString::fromStdString(componentPath.parent_path().parent_path().filename().string());
    if (pkgGroupName.size() != 3) {
        return false;
    }

    if (componentName.startsWith(pkgName + "_") && pkgName.startsWith(pkgGroupName)) {
        return true;
    }

    // Except if the package name contains a '+' sign.
    auto trySpecialPkgName = pkgName.split("+");
    if (trySpecialPkgName.size() == 2) {
        auto specialPkgName = trySpecialPkgName[0];
        if (componentName.startsWith(specialPkgName + "_") && specialPkgName.startsWith(pkgGroupName)) {
            return true;
        }
    }

    return false;
}

bool ClpUtil::isComponentOnStandalonePackage(const std::filesystem::path& componentPath)
{
    // Capture special component naming inside standalone package in the form
    // <prefix>_<pkgname>_<component_name>. e.g.: ct_lvtclp_filesystemscanner.cpp
    auto hasStandaloneNameWithPkgPrefix =
        std::regex_search(componentPath.generic_string(), std::regex{"/([a-zA-Z]{1,2})_([a-zA-Z0-9_]+)\\."});
    if (!hasStandaloneNameWithPkgPrefix) {
        return false;
    }

    // In order to be a valid standalone package, the component must be inside a package containing it's prefix
    auto componentName = QString::fromStdString(componentPath.filename().generic_string());
    auto parentPkgName = QString::fromStdString(componentPath.parent_path().filename().generic_string());
    if (componentName.startsWith(parentPkgName)) {
        return true;
    }

    // The containing package may be acceptable if it is named without the '<prefix>_' prefix.
    // In other words, a component named '<prefix>_<pkgname>_<component_name>' can be inside a package named
    // <pkgname>.
    auto splitComponentName = componentName.split("_");
    if (splitComponentName.size() >= 3) {
        if (splitComponentName[1] == parentPkgName) {
            return true;
        }

        // The containing package may be acceptable if it is named without the '<prefix>_' prefix.
        // In other words, a component named '<prefix>_<pkgname>_<component_name>' can be inside a package named
        // <pkgname>.
        auto splitComponentName = componentName.split("_");
        if (splitComponentName.size() >= 3) {
            if (splitComponentName[1] == parentPkgName) {
                return true;
            }
        }
    }

    return false;
}

bool ClpUtil::isFileIgnored(const std::string& file, std::vector<llvm::GlobPattern> const& ignoreGlobs)
{
    for (auto&& ignoreGlob : ignoreGlobs) {
        if (ignoreGlob.match(file)) {
            return true;
        }
    }
    return false;
}

ClpUtil::PySemanticPackingRule::PySemanticPackingRule(std::filesystem::path pythonFile):
    d_pythonFile(std::move(pythonFile))
{
}

std::vector<llvm::GlobPattern> ClpUtil::stringListToGlobPattern(const std::vector<std::string>& userIgnoreList)
{
    std::vector<llvm::GlobPattern> res;
    std::set<std::string> nonDuplicatedItems(std::begin(userIgnoreList), std::end(userIgnoreList));
    for (const auto& ignoreFile : nonDuplicatedItems) {
        llvm::Expected<llvm::GlobPattern> pat = llvm::GlobPattern::create(ignoreFile);
        if (pat) {
            res.push_back(pat.get());
        }
    }
    return res;
}

std::vector<std::filesystem::path> ClpUtil::ensureCanonical(const std::vector<std::filesystem::path>& maybeCanonical)
{
    std::vector<std::filesystem::path> canonicalPaths;
    canonicalPaths.reserve(maybeCanonical.size());

    std::transform(maybeCanonical.begin(),
                   maybeCanonical.end(),
                   std::back_inserter(canonicalPaths),
                   [](const std::filesystem::path& dir) {
                       return std::filesystem::weakly_canonical(dir);
                   });

    return canonicalPaths;
}

namespace detail {
auto getPyFunFrom(std::filesystem::path const& pythonFile, std::string const& functionName)
{
    namespace py = pybind11;

    auto modulePath = pythonFile.parent_path().string();
    auto pySys = py::module_::import("sys");
    pySys.attr("path").attr("append")(modulePath);

    auto pyUserModule = py::module_::import(pythonFile.stem().string().c_str());
    return py::function(pyUserModule.attr(functionName.c_str()));
}
} // namespace detail

bool ClpUtil::PySemanticPackingRule::accept(std::string const& filepath) const
{
    namespace py = pybind11;
    py::gil_scoped_acquire gil{};
    return detail::getPyFunFrom(d_pythonFile, "accept")(filepath).cast<bool>();
}

std::string ClpUtil::PySemanticPackingRule::process(std::string const& filepath,
                                                    PkgMatcherAddPkgFunction const& addPkg) const
{
    namespace py = pybind11;
    py::gil_scoped_acquire gil{};
    return detail::getPyFunFrom(d_pythonFile, "process")(filepath, addPkg).cast<std::string>();
}

std::vector<std::unique_ptr<ClpUtil::SemanticPackingRule>> ClpUtil::getAllSemanticPackingRules()
{
    auto searchPaths = std::vector<std::string>{};
    if (const char *env_p = std::getenv("SEMRULES_PATH")) {
        searchPaths.emplace_back(env_p);
    }
    if (QCoreApplication::instance()) {
        auto appPath = QDir(QCoreApplication::applicationDirPath() + "/semrules").path().toStdString();
        searchPaths.emplace_back(appPath);
    }
    auto homePath = QDir(QDir::homePath() + "/semrules").path().toStdString();
    searchPaths.emplace_back(homePath);

    std::vector<std::filesystem::path> sortedPaths;
    for (auto&& path : searchPaths) {
        if (!std::filesystem::exists(path)) {
            continue;
        }

        for (auto&& entry : std::filesystem::directory_iterator(path)) {
            if (entry.path().extension().string() == ".py") {
                sortedPaths.push_back(entry.path());
            }
        }
    }
    std::sort(sortedPaths.begin(), sortedPaths.end());

    auto rules = std::vector<std::unique_ptr<ClpUtil::SemanticPackingRule>>{};
    for (auto&& path : sortedPaths) {
        rules.emplace_back(std::make_unique<PySemanticPackingRule>(path));
    }
    return rules;
}

namespace nonLakosian {

inline QString asLinuxPath(QString path)
{
    return path.replace("\\", "/");
}

lvtmdb::FileObject *ClpUtil::writeSourceFile(lvtmdb::ObjectStore& memDb,
                                             const std::string& fString,
                                             const std::filesystem::path& sourceDir,
                                             const std::filesystem::path& buildDir,
                                             const std::filesystem::path& inclusionPrefixPath)
{
    std::string filepath = std::filesystem::path{fString}.generic_string();
    std::filesystem::path sourceDirectory = sourceDir.generic_string();
    std::filesystem::path buildDirectory = buildDir.generic_string();

    auto memDbLock = memDb.rwLock();

    auto const LINUX_SEP = QString{"/"}; // Uses linux separator even on Windows. Paths must be converted on Windows.

    auto mainFolderName = QString{};
    auto currentVirtualWorkPath = QString{};
    auto relativePath = QString{};
    const bool inSource = QString::fromStdString(filepath).contains(QString::fromStdString(sourceDirectory.string()));
    const bool inBuild = QString::fromStdString(filepath).contains(QString::fromStdString(buildDirectory.string()));

    if (inSource || inBuild) {
        // If it is a file in the source directory, it takes precedence for qualified name deduction
        auto prefixAsString = asLinuxPath(QString::fromStdString(sourceDirectory.string()));
        if (prefixAsString.endsWith('/')) {
            prefixAsString.chop(1);
        }

        mainFolderName = prefixAsString.split(LINUX_SEP).last();
        if (mainFolderName.isEmpty()) {
            mainFolderName = "Unnamed Project";
        }
        currentVirtualWorkPath = "${SOURCE_DIR}/";
        relativePath = QString::fromStdString(filepath).replace(QString::fromStdString(sourceDirectory.string()), "");
    } else {
        // Anything else will be moved to a global "external" pseudo-folder
        // TODO: Let the user provide more information regarding what is not "external".
        mainFolderName = "External Libraries";
        currentVirtualWorkPath = "${EXTERNAL_LIBS_DIR}/";
        relativePath = asLinuxPath(QString::fromStdString(filepath));
    }

    auto *parentPkg = memDb.getOrAddPackage(
        /*qualifiedName=*/mainFolderName.toStdString(),
        /*name=*/mainFolderName.toStdString(),
        /*diskPath=*/currentVirtualWorkPath.toStdString(),
        /*parent=*/nullptr,
        /*repository=*/nullptr);

    auto relativePathAsVec = relativePath.split(LINUX_SEP);
    auto filename = relativePathAsVec.takeLast();
    for (auto const& folderName : relativePathAsVec) {
        if (folderName.isEmpty()) {
            continue;
        }

        currentVirtualWorkPath = currentVirtualWorkPath + folderName + LINUX_SEP;
        auto *newPkg = memDb.getOrAddPackage(
            /*qualifiedName=*/folderName.toStdString(),
            /*name=*/folderName.toStdString(),
            /*diskPath=*/currentVirtualWorkPath.toStdString(),
            /*parent=*/parentPkg,
            /*repository=*/nullptr);

        parentPkg->withRWLock([&]() {
            parentPkg->addChild(newPkg);
        });
        parentPkg = newPkg;
    }
    auto componentName = std::filesystem::path{filename.toStdString()}.stem().string();
    auto *component = memDb.getOrAddComponent(componentName, componentName, parentPkg);
    parentPkg->withRWLock([&]() {
        parentPkg->addComponent(component);
    });

    auto *file = memDb.getOrAddFile(
        /*qualifiedName=*/(currentVirtualWorkPath + filename).toStdString(),
        /*name=*/(currentVirtualWorkPath + filename).toStdString(),
        /*isHeader=*/false,
        /*hash=*/"",
        /*package=*/parentPkg,
        /*component=*/component);

    component->withRWLock([&]() {
        component->addFile(file);
    });
    return file;
}

void ClpUtil::addSourceFileRelationWithParentPropagation(lvtmdb::FileObject *fromFileObj, lvtmdb::FileObject *toFileObj)
{
    if (!fromFileObj || !toFileObj) {
        return;
    }
    lvtmdb::FileObject::addIncludeRelation(fromFileObj, toFileObj);

    auto *fromComponent = fromFileObj->withROLock([&fromFileObj]() {
        return fromFileObj->component();
    });
    auto *toComponent = toFileObj->withROLock([&toFileObj]() {
        return toFileObj->component();
    });
    if (!fromComponent || !toComponent) {
        return;
    }
    lvtmdb::ComponentObject::addDependency(fromComponent, toComponent);

    // Important note: Package dependencies are propagated by "same level", but maybe they should be
    // rethink to support multi-level dependency, since the folder structure between the dependencies
    // may be different. Example:
    // '/a/b/c/xx.c' -[includes]-> '/d/yy.h'
    // We'll only persist the "package-dependency" 'c'->'d'. The discussion is not if the dependencies
    // 'b'->'d' and 'a'->'d' should exist or not - The discussion is weather this information isn't
    // already implicitly defined when we say that 'xx.c' depends on 'yy.h', so possibly all package-level
    // dependencies on the database are irrelevant.
    // (Extra note: Even component-to/from-package dependencies should appear on GUI, IMO).
    auto *fromPackage = fromComponent->withROLock([&fromComponent]() {
        return fromComponent->package();
    });
    auto *toPackage = toComponent->withROLock([&toComponent]() {
        return toComponent->package();
    });
    while (fromPackage && toPackage) {
        lvtmdb::PackageObject::addDependency(fromPackage, toPackage);

        fromPackage = fromPackage->withROLock([&fromPackage]() {
            return fromPackage->parent();
        });
        toPackage = toPackage->withROLock([&toPackage]() {
            return toPackage->parent();
        });
    }
}

} // namespace nonLakosian

} // end namespace Codethink::lvtclp
