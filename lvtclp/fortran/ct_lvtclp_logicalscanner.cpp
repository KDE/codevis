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

#include <ct_lvtmdb_fileobject.h>
#include <ct_lvtmdb_functionobject.h>
#include <ct_lvtmdb_objectstore.h>
#include <flang/Frontend/CompilerInstance.h>
#include <flang/Parser/parse-tree-visitor.h>
#include <flang/Parser/parse-tree.h>

#include <iostream>

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

            auto *file = memDb.getFile(currentFilePath);
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
