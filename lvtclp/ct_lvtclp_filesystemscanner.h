// ct_lvtclp_filesystemscanner.h                                     -*-C++-*-

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

#ifndef INCLUDED_CT_LVTCLP_FILESYSTEMSCANNER
#define INCLUDED_CT_LVTCLP_FILESYSTEMSCANNER

//@PURPOSE: Scan the filesystem for package groups, packages, and componenets
//
//@CLASSES:
//  lvtclp::FilesystemScanner

#include <lvtclp_export.h>

#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include <llvm/Support/GlobPattern.h>

namespace Codethink::lvtmdb {
class ObjectStore;
}
namespace Codethink::lvtmdb {
class PackageObject;
}

namespace Codethink::lvtclp {

class LvtCompilationDatabase;

struct PackageHelper {
    std::string parentRepositoryName;
    std::string qualifiedName;
    std::string filePath;
};

struct RepositoryHelper {
    std::string qualifiedName;
    std::string path;
};

class LVTCLP_EXPORT FilesystemScanner {
  private:
    // PRIVATE TYPES
    struct Private;

    // DATA
    std::unique_ptr<Private> d;

  public:
    // CREATORS
    explicit FilesystemScanner(lvtmdb::ObjectStore& memDb,
                               const std::filesystem::path& prefix,
                               const LvtCompilationDatabase& cdb,
                               std::function<void(const std::string&, long)> messageCallback,
                               bool catchCodeAnalysisOutput,
                               std::vector<std::filesystem::path> nonLakosianDirs,
                               std::vector<llvm::GlobPattern> ignoreGlobs,
                               bool enableLakosianRules);
    // cdb and memDb must live at least as long as this FilesystemScanner

    ~FilesystemScanner() noexcept;

    // TYPES
    struct IncrementalResult {
        std::vector<std::string> newFiles;
        std::vector<std::string> modifiedFiles;
        std::vector<std::string> deletedFiles;

        // In the main code we only care about files, but these are useful for
        // testing
        std::vector<std::string> newPkgs;
        std::vector<std::string> deletedPkgs;
    };

    // MANIPULATORS
    IncrementalResult scanCompilationDb();
    // Scan compilation database: adding source files, components,
    // packages, and package groups to the database.

  private:
    // MANIPULATORS
    void scanPath(const std::filesystem::path& path);
    void scanHeader(const std::filesystem::path& path);

    void addSourceFile(const std::filesystem::path& path, const std::string& package);

    std::string
    addLakosianSourcePackage(const std::filesystem::path& path, const std::string& parent, bool isStandalone);
    // returns the qualified name of the package

    IncrementalResult addToDatabase();

    lvtmdb::PackageObject *addPackage(IncrementalResult& out,
                                      std::unordered_set<lvtmdb::PackageObject *>& existingPkgs,
                                      const std::string& qualifiedName,
                                      const std::string& parentName,
                                      const std::string& filePath,
                                      const std::string& repositoryName);

    bool tryProcessFileUsingSemanticRules(const std::filesystem::path& path);
    void processFileUsingLakosianRules(const std::filesystem::path& path);
};

} // namespace Codethink::lvtclp

#endif // INCLUDED_CT_LVTCLP_FILESYSTEMSCANNER
