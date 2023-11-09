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

#include <ct_lvtmdb_objectstore.h>
#include <flang/Frontend/CompilerInstance.h>
#include <flang/Parser/parse-tree-visitor.h>
#include <flang/Parser/parse-tree.h>

#include <iostream>

namespace Codethink::lvtclp::fortran {

using namespace Fortran::parser;
using namespace Codethink::lvtmdb;

struct FindExecutionPartCallsTreeVisitor {
    FindExecutionPartCallsTreeVisitor(ObjectStore& memDb): memDb(memDb)
    {
    }
    ObjectStore& memDb;

    // clang-format off
    template<typename A> bool Pre(const A&) { return true; }
    template<typename A> void Post(const A&) {}
    // clang-format on

    void Post(const CallStmt& f)
    {
        // TODO: newer (llvm-17) flang only - need to fix (or drop support) for llvm-15 or llvm-16 builds
        // std::cout << "++ CALLS " << std::get<Name>(std::get<ProcedureDesignator>(f.call.t).u).ToString() << "\n";
    }
};

struct ParseTreeVisitor {
    ParseTreeVisitor(ObjectStore& memDb): memDb(memDb)
    {
    }
    ObjectStore& memDb;

    // TODO: Perhaps we could return false and pick only the useful paths?
    //       This same note should also be considered for other visitors.
    // clang-format off
    template<typename A> bool Pre(const A&) { return true; }
    template<typename A> void Post(const A&) {}
    // clang-format on

    void Post(const SubroutineSubprogram& f)
    {
        auto& sr = std::get<Statement<SubroutineStmt>>(f.t);

        // TODO: QualifiedName is not unique if we use name directly
        auto functionName = std::get<Name>(sr.statement.t).ToString();
        memDb.withRWLock([&]() {
            auto *function = memDb.getOrAddFunction(
                /*qualifiedName=*/functionName,
                /*name=*/functionName,
                /*signature=*/"",
                /*returnType=*/"",
                /*templateParameters=*/"",
                /*parent=*/nullptr);
            (void) function;
        });

        auto& execPart = std::get<ExecutionPart>(f.t);
        auto visitor = FindExecutionPartCallsTreeVisitor{memDb};
        Fortran::parser::Walk(execPart, visitor);
    }
};

LogicalParseAction::LogicalParseAction(ObjectStore& memDb): memDb(memDb)
{
}

void LogicalParseAction::executeAction()
{
    auto visitor = ParseTreeVisitor{memDb};
    Fortran::parser::Walk(getInstance().getParsing().parseTree(), visitor);
}

} // namespace Codethink::lvtclp::fortran
