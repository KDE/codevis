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

#include <fortran/ct_lvtclp_logicalscanner.h>

#include <ct_lvtmdb_componentobject.h>
#include <ct_lvtmdb_fileobject.h>
#include <ct_lvtmdb_functionobject.h>
#include <ct_lvtmdb_objectstore.h>
#include <flang/Frontend/CompilerInstance.h>
#include <flang/Parser/parse-tree-visitor.h>
#include <flang/Parser/parse-tree.h>

#include <fstream>
#include <iostream>
#include <type_traits>

namespace {
bool srcFileContainsSrcCode(std::string const& guessedSrcFile, std::string const& targetSrcCode)
{
    auto ifs = std::ifstream(guessedSrcFile);
    auto contents = std::string(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());

    // Transforms the file contents, as if it was processed by the lexer.
    contents.erase(std::remove_if(contents.begin(),
                                  contents.end(),
                                  [](unsigned char c) {
                                      return std::isspace(c);
                                  }),
                   contents.end());
    std::transform(contents.begin(), contents.end(), contents.begin(), [](unsigned char c) {
        return std::tolower(c);
    });

    return (contents.find(targetSrcCode) != std::string::npos);
};
} // namespace

namespace Codethink::lvtclp::fortran {

using namespace Fortran::parser;
using namespace Codethink::lvtmdb;

struct FindExecutionPartCallsTreeVisitor {
    FindExecutionPartCallsTreeVisitor(ObjectStore& memDb, FunctionObject *caller): memDb(memDb), caller(caller)
    {
    }
    ObjectStore& memDb;
    FunctionObject *caller;

    // clang-format off
    template<typename A> bool Pre(const A&) { return true; }
    template<typename A> void Post(const A&) {}
    // clang-format on

    void Post(const Call& f)
    {
        auto functionName = std::get<Name>(std::get<ProcedureDesignator>(f.t).u).ToString();
        memDb.withRWLock([&]() {
            auto *callee = memDb.getOrAddFunction(
                /*qualifiedName=*/functionName,
                /*name=*/functionName,
                /*signature=*/"",
                /*returnType=*/"",
                /*templateParameters=*/"",
                /*parent=*/nullptr);
            FunctionObject::addDependency(caller, callee);
        });
    }
};

struct ParseTreeVisitor {
    ParseTreeVisitor(ObjectStore& memDb, std::filesystem::path const& currentFilePath):
        memDb(memDb), currentFilePath(currentFilePath)
    {
    }
    ObjectStore& memDb;
    std::filesystem::path currentFilePath;

    // TODO: Perhaps we could return false and pick only the useful paths?
    //       This same note should also be considered for other visitors.
    // clang-format off
    template<typename A> bool Pre(const A&) { return true; }
    template<typename A> void Post(const A&) {}
    // clang-format on

    void Post(const SubroutineSubprogram& f)
    {
        prepareAndAddFuncToDB(f);
    }

    void Post(const FunctionSubprogram& f)
    {
        prepareAndAddFuncToDB(f);
    }

  private:
    // clang-format off
    template<typename T>
    requires(std::is_same_v<T, FunctionSubprogram> || std::is_same_v<T, SubroutineSubprogram>)
    void prepareAndAddFuncToDB(T const& f)
    // clang-format on
    {
        auto const& ss = std::get<0>(f.t);
        {
            // Sanity check
            using ss_T = std::remove_cvref_t<decltype(ss)>;
            // clang-format off
            static_assert(
                std::is_same_v<ss_T, Statement<SubroutineStmt>> ||
                std::is_same_v<ss_T, Statement<FunctionStmt>>
            );
            // clang-format on
        }

        // TODO: QualifiedName is not unique if we use name directly
        auto functionName = std::get<Name>(ss.statement.t).ToString();
        FunctionObject *function = nullptr;
        memDb.withRWLock([&]() {
            function = memDb.getOrAddFunction(
                /*qualifiedName=*/functionName,
                /*name=*/functionName,
                /*signature=*/"",
                /*returnType=*/"",
                /*templateParameters=*/"",
                /*parent=*/nullptr);
            assert(function);

            // Find where the function was really defined.
            //
            // TODO: This approach is extremely inefficient, and must be reworked when we have
            //       provenience mapping in the AST or another way of finding out where the INCLUDEs
            //       come from.
            auto guessedDefinitionFilePath = [&]() {
                auto srcCode = ss.source.ToString();

                if (srcFileContainsSrcCode(currentFilePath.string(), srcCode)) {
                    return currentFilePath.string();
                }

                auto *file = memDb.getFile(currentFilePath.string());
                auto fileLock = file->readOnlyLock();
                auto *component = file->component();
                auto componentLock = component->readOnlyLock();
                for (auto *depComponent : component->forwardDependencies()) {
                    auto depComponentLock = depComponent->readOnlyLock();
                    for (auto *depFile : depComponent->files()) {
                        auto depFileLock = depFile->readOnlyLock();
                        auto depFilePath = depFile->qualifiedName();
                        if (srcFileContainsSrcCode(depFilePath, srcCode)) {
                            return depFilePath;
                        }
                    }
                }

                // If the definition file is not found, returns the "current" (in process) file, to avoid
                // missing source.
                // TODO: Proper warning message propagation.
                std::cout << "WARNING: Could not find proper source code for function " << functionName
                          << " - Will fallback to " << currentFilePath.string() << "\n";
                return currentFilePath.string();
            }();

            auto *file = memDb.getFile(guessedDefinitionFilePath);
            file->withRWLock([&] {
                file->addGlobalFunction(function);
            });
        });

        auto& execPart = std::get<ExecutionPart>(f.t);
        auto visitor = FindExecutionPartCallsTreeVisitor{memDb, function};
        Fortran::parser::Walk(execPart, visitor);
    }
};

LogicalParseAction::LogicalParseAction(ObjectStore& memDb): memDb(memDb)
{
}

void LogicalParseAction::executeAction()
{
    auto currentInputPath = std::filesystem::path{getCurrentFileOrBufferName().str()};
    auto visitor = ParseTreeVisitor{memDb, currentInputPath};
    Fortran::parser::Walk(getInstance().getParsing().parseTree(), visitor);
}

} // namespace Codethink::lvtclp::fortran
