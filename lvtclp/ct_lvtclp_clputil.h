// ct_lvtclp_clputil.h                                                -*-C++-*-

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

#ifndef INCLUDED_CT_LVTCLP_CLPUTIL
#define INCLUDED_CT_LVTCLP_CLPUTIL

//@PURPOSE: Provide somewhere to contain things common to lvtclp
//
//@CLASSES:
//  lvtclp::ClpUtil: common definitions for lvtclp

#include <filesystem>
#include <lvtclp_export.h>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <result/result.hpp>

#include <clang/Basic/SourceManager.h>
#include <clang/Tooling/CompilationDatabase.h>
#include <llvm/Support/GlobPattern.h>

namespace Codethink {

// FORWARD DECLARATIONS

namespace lvtmdb {
class FileObject;
}
namespace lvtmdb {
class ObjectStore;
}
namespace lvtmdb {
class PackageObject;
}
namespace lvtmdb {
class TypeObject;
}
namespace lvtmdb {
class FunctionObject;
}

namespace lvtclp {

enum class FileType : short {
    e_Header,
    e_Source,
    e_KnownUnknown,
    // A file type we expect to see amongst source files but
    // which isn't a header or a source.
    e_UnknownUnknown,
    // A file type we don't recognise.
};

struct CompilationDatabaseError {
    enum class Kind { ErrorLoadingFromFile, CompileCommandsContainsNoCommands, CompileCommandsContainsNoFiles };

    Kind kind;
    std::string message = "";
};

// ==============
// struct ClpUtil
// ==============

struct LVTCLP_EXPORT ClpUtil {
    // This struct groups things common to lvtclp

  public:
    // CLASS METHODS
    static std::filesystem::path normalisePath(std::filesystem::path path, const std::filesystem::path& prefix);
    // Normalise a path for inclusion in the database

    static lvtmdb::FileObject *writeSourceFile(const std::string& filename,
                                               bool isHeader,
                                               lvtmdb::ObjectStore& memDb,
                                               const std::filesystem::path& prefix,
                                               const std::vector<std::filesystem::path>& nonLakosianDirs,
                                               const std::vector<std::pair<std::string, std::string>>& thirdPartyDirs);

    static std::string getRealPath(const clang::SourceLocation& loc, const clang::SourceManager& mgr);
    // Fetch ann absolute path to loc (or "")

    static void writeDependencyRelations(lvtmdb::PackageObject *source, lvtmdb::PackageObject *target);
    // Add a package dependency from source to target

    static void addUsesInInter(lvtmdb::TypeObject *source, lvtmdb::TypeObject *target);
    // Add a uses in the interface relationship between source and target

    static void addUsesInImpl(lvtmdb::TypeObject *source, lvtmdb::TypeObject *target);
    // Add a uses in the impl relationship between source and target

    static void addFnDependency(lvtmdb::FunctionObject *source, lvtmdb::FunctionObject *target);
    // Add a function-to-function dependency

    static FileType categorisePath(const std::string& file);
    // return the FileType for the path

    static const char *const NON_LAKOSIAN_GROUP_NAME;

    static long getThreadId();
    // returns the id of this thread as a long.

    static bool isComponentOnPackageGroup(const std::filesystem::path& componentPath);
    static bool isComponentOnStandalonePackage(const std::filesystem::path& componentPath);

    static bool isFileIgnored(const std::string& file, std::vector<llvm::GlobPattern> const& ignoreGlobs);

    using PkgMatcherAddPkgFunction = std::function<void(std::string const& qualifiedName,
                                                        std::optional<std::string> parentQualifiedName,
                                                        std::optional<std::string> repositoryName,
                                                        std::optional<std::string> path)>;
    class SemanticPackingRule {
      public:
        virtual ~SemanticPackingRule() = default;
        virtual bool accept(std::string const& filepath) const = 0;
        virtual std::string process(std::string const& filepath, PkgMatcherAddPkgFunction const& addPkg) const = 0;
    };

    class PySemanticPackingRule : public SemanticPackingRule {
      public:
        explicit PySemanticPackingRule(std::filesystem::path pythonFile);
        ~PySemanticPackingRule() = default;
        bool accept(std::string const& filepath) const override;
        std::string process(std::string const& filepath, PkgMatcherAddPkgFunction const& addPkg) const override;

      private:
        std::filesystem::path d_pythonFile;
    };
    static std::vector<std::unique_ptr<SemanticPackingRule>> getAllSemanticPackingRules();
};

enum class CheckingMode : char {
    // Controls how clp behaves when it encounters something non-lakosian

    e_error, // Non-lakosian is an error
    e_warn, // Non-lakosian is a warning
    e_permissive, // Non-lakosian is ignored silently
};

class LVTCLP_EXPORT CombinedCompilationDatabase : public clang::tooling::CompilationDatabase {
    // clang::ToolingCompilationDatabase formed from a combination of existing
    // compilation databases
  private:
    // TYPES
    struct Private;

    // DATA
    std::unique_ptr<Private> d;

  public:
    // CREATORS
    CombinedCompilationDatabase();
    ~CombinedCompilationDatabase() noexcept override;

    // MODIFIERS
    cpp::result<bool, CompilationDatabaseError> addCompilationDatabase(const std::filesystem::path& path);
    // Read in a compile_commands.json file from path

    void addCompilationDatabase(const clang::tooling::CompilationDatabase& db, const std::filesystem::path& buildDir);
    // Add the contents of the compilation database given in as an argument
    // to this one, using the given build directory (the directory from which
    // relative paths in the database should be considered resolved)

    // ACCESSORS
    [[nodiscard]] std::vector<clang::tooling::CompileCommand>
    getCompileCommands(llvm::StringRef FilePath) const override;

    [[nodiscard]] std::vector<std::string> getAllFiles() const override;

    [[nodiscard]] std::vector<clang::tooling::CompileCommand> getAllCompileCommands() const override;
};

class LvtCompilationDatabase : public clang::tooling::CompilationDatabase {
    // clang::Tooling::CompilationDatabase with some extra accessors to
    // efficiently figure out if files, packages or package groups are present
    // in the database

  public:
    // CREATORS
    ~LvtCompilationDatabase() override = default;

    // ACCESSORS
    [[nodiscard]] virtual bool containsFile(const std::string& path) const = 0;
    // Returns true if the file at path is in the compilation database

    [[nodiscard]] virtual bool containsPackage(const std::string& pkg) const = 0;
    // Returns true if there is a package or package group with this
    // qualified name in the compilation database
};

namespace nonLakosian {

struct LVTCLP_EXPORT ClpUtil {
    static lvtmdb::FileObject *writeSourceFile(lvtmdb::ObjectStore& memDb,
                                               const std::string& filepath,
                                               const std::filesystem::path& sourceDirectory,
                                               const std::filesystem::path& inclusionPrefixPath);

    static void addSourceFileRelationWithParentPropagation(lvtmdb::FileObject *fromFileObj,
                                                           lvtmdb::FileObject *toFileObj);
};

} // namespace nonLakosian

} // end namespace lvtclp

} // end namespace Codethink

#endif
