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
#include <fortran/ct_lvtclp_physicalscanner.h>
#include <fortran/ct_lvtclp_tool.h>

#include <flang/Frontend/CompilerInstance.h>
#include <flang/Frontend/FrontendActions.h>

#include <clang/Driver/DriverDiagnostic.h>
#include <flang/Frontend/CompilerInstance.h>
#include <flang/Frontend/CompilerInvocation.h>
#include <flang/Frontend/TextDiagnosticBuffer.h>
#include <flang/FrontendTool/Utils.h>
#include <flang/Parser/parse-tree-visitor.h>
#include <llvm/Option/Arg.h>
#include <llvm/Option/ArgList.h>
#include <llvm/Option/OptTable.h>
#include <llvm/Support/TargetSelect.h>

#include <iostream>
#include <memory>

namespace Codethink::lvtclp::fortran {

using namespace Fortran::frontend;

void run(FrontendAction& act)
{
    std::unique_ptr<CompilerInstance> flang(new CompilerInstance());
    flang->createDiagnostics();
    if (!flang->hasDiagnostics()) {
        // TODO: Proper error management
        std::cout << "Could not create flang diagnostics.\n";
        return;
    }

    auto *diagsBuffer = new TextDiagnosticBuffer;
    llvm::IntrusiveRefCntPtr<clang::DiagnosticIDs> diagID(new clang::DiagnosticIDs());
    llvm::IntrusiveRefCntPtr<clang::DiagnosticOptions> diagOpts = new clang::DiagnosticOptions();
    clang::DiagnosticsEngine diags(diagID, &*diagOpts, diagsBuffer);

    // TODO: Check how to pass files or maybe cmakelists information?
    auto args = std::array<const char *, 1>{"helloworld.f"};
    auto commandLineArgs = llvm::ArrayRef<const char *>(args);
    bool success = CompilerInvocation::createFromArgs(flang->getInvocation(), commandLineArgs, diags);

    // Initialize targets first, so that --version shows registered targets.
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmPrinters();

    diagsBuffer->flushDiagnostics(flang->getDiagnostics());

    if (!success) {
        // TODO: Proper error management
        std::cout << "Could not prepare the flang object.\n";
        return;
    }

    success = flang->executeAction(act);
    if (!success) {
        // TODO: Proper error management
        std::cout << "Action failed.\n";
        return;
    }

    // Delete output files to free Compiler Instance
    flang->clearOutputFiles(/*EraseFiles=*/false);
}

bool Tool::runPhysical(bool skipScan)
{
    auto action = PhysicalParseAction{};
    run(action);

    return true;
}

bool Tool::runFull(bool skipPhysical)
{
    if (!skipPhysical) {
        runPhysical(/*skipScan=*/false);
    }
    auto action = LogicalParseAction{};
    run(action);

    return true;
}

} // namespace Codethink::lvtclp::fortran
