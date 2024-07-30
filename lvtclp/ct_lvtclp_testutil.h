// ct_lvtclp_testutil.h                                                -*-C++-*-

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

#ifndef INCLUDED_CT_LVTCLP_TESTUTIL
#define INCLUDED_CT_LVTCLP_TESTUTIL

#include <ct_lvtclp_clputil.h>

#include <ct_lvtmdb_objectstore.h>

#include <filesystem>
#include <initializer_list>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <clang/Tooling/CompilationDatabase.h>

namespace Codethink::lvtclp {

struct Test_Util {
    // Not for use outside lvtclp tests

    static bool runOnCode(lvtmdb::ObjectStore& mdb, const std::string& source, const std::string& fileName);
    // Run the AST parse on the c++ source in `source`, saving to database
    // `session` and acting as though `source` has the file name `fileName`

    static bool isAExists(const std::string& derivedClassQualifiedName,
                          const std::string& baseClassQualifiedName,
                          lvtmdb::ObjectStore& session);
    static bool usesInTheImplementationExists(const std::string& sourceQualifiedName,
                                              const std::string& targetQualifiedName,
                                              lvtmdb::ObjectStore& session);
    static bool usesInTheInterfaceExists(const std::string& sourceQualifiedName,
                                         const std::string& targetQualifiedName,
                                         lvtmdb::ObjectStore& session);
    // Test if a relationship exists in the database. Expects an active
    // database transaction

    static bool classDefinedInFile(const std::string& classQualifiedName,
                                   const std::string& fileQualifiedName,
                                   lvtmdb::ObjectStore& session);
    // Test if a class is defined in a particular file. Expects an active
    // database transaction

    static bool createFile(const std::filesystem::path& path, const std::string& contents = "");
    // create a file at path, deleting anything already there with that name
    // contents is written to the file
    // returns true on success, false otherwise
};

class StaticCompilationDatabase : public LvtCompilationDatabase {
    // A compilation database constructable from compile commands

  private:
    // DATA
    std::vector<clang::tooling::CompileCommand> d_compileCommands;
    const std::filesystem::path d_directory;

  public:
    // CREATORS
    explicit StaticCompilationDatabase(std::initializer_list<clang::tooling::CompileCommand> commands,
                                       std::filesystem::path directory);
    // Construct directly from commands

    explicit StaticCompilationDatabase(std::initializer_list<std::pair<std::string, std::string>> paths,
                                       const std::string& command,
                                       const std::vector<std::string>& arguments,
                                       std::filesystem::path directory);
    // This constructor constructs the clang::tooling::CompileCommands for you
    // paths is a series of pairs of <input path, output path>
    // command is the compiler path
    // arguments are common arguments to be added to every command line
    // directory is the directory to run from

    [[nodiscard]] std::vector<clang::tooling::CompileCommand>
    getCompileCommands(llvm::StringRef FilePath) const override;

    [[nodiscard]] std::vector<std::string> getAllFiles() const override;

    [[nodiscard]] std::vector<clang::tooling::CompileCommand> getAllCompileCommands() const override;

    [[nodiscard]] bool containsFile(const std::string& file) const override;

    [[nodiscard]] bool containsPackage(const std::string& str) const override;
};

struct ModelUtil {
    // Easy language for specifying and checking the contents of a database:

    // PUBLIC TYPES
    struct SourceFileModel {
        std::string path;
        bool isHeader;
        std::string pkg;
        std::string component;
        std::vector<std::string> namespaces;
        std::vector<std::string> classes;
        std::vector<std::string> includes;
    };

    struct ComponentModel {
        std::string qualifiedName;
        std::string name;
        std::vector<std::string> files;
        std::vector<std::string> deps;
    };

    struct PackageModel {
        std::string name;
        std::string parent;
        std::vector<std::string> udts;
        std::vector<std::string> components;
        std::vector<std::string> dependencies;
    };

    struct UDTModel {
        std::string qualifiedName;
        std::string nmspc;
        std::string pkg;
        std::vector<std::string> usesInImpl;
        std::vector<std::string> usesInInter;
        std::vector<std::string> isA;
        std::vector<std::string> methods;
        std::vector<std::string> fields;
    };

    // CLASS METHODS
    [[nodiscard]] static bool checkSourceFiles(lvtmdb::ObjectStore& store,
                                               const std::initializer_list<SourceFileModel>& files,
                                               bool printToConsole = false);
    // Check that the source files described by models are all in the database
    // session with the described properties

    [[nodiscard]] static bool checkComponents(lvtmdb::ObjectStore& store,
                                              const std::initializer_list<ComponentModel>& checkComponents);
    // Check that the components described by models are all in the database
    // session with the described properties

    [[nodiscard]] static bool checkPackages(lvtmdb::ObjectStore& store,
                                            const std::initializer_list<PackageModel>& pkgs);
    // Check that the packages and package groups described by models are
    // all in the database session with the described properties

    [[nodiscard]] static bool checkUDTs(lvtmdb::ObjectStore& store, const std::initializer_list<UDTModel>& udts);
    // Check that the types described by models are all in the database
    // session with the described properties
};

} // namespace Codethink::lvtclp

#pragma push_macro("slots")
#undef slots
#include <pybind11/embed.h>
#include <pybind11/pybind11.h>
#pragma pop_macro("slots")

namespace py = pybind11;
struct PyDefaultGilReleasedContext {
    py::scoped_interpreter pyInterp;
    py::gil_scoped_release pyGilDefaultReleased;
};

#endif // INCLUDED_CT_LVTCLP_TESTUTIL
