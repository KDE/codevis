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

#include <fortran/backends/flang/ct_lvtclp_logicalscanner.h>
#include <fortran/backends/flang/ct_lvtclp_physicalscanner.h>
#include <fortran/ct_lvtclp_tool.h>

#include <flang/Frontend/CompilerInstance.h>
#include <flang/Frontend/CompilerInvocation.h>
#include <flang/Frontend/TextDiagnosticBuffer.h>
#include <flang/FrontendTool/Utils.h>
#include <flang/Parser/parse-tree-visitor.h>
#include <llvm/Support/TargetSelect.h>

#include <clang/Driver/DriverDiagnostic.h>
#include <clang/Tooling/JSONCompilationDatabase.h>

#include <iostream>
#include <memory>

namespace {
void run(Fortran::frontend::FrontendAction& act, clang::tooling::CompileCommand const& cmd)
{
    using namespace Fortran::frontend;

    auto ext = std::filesystem::path{cmd.Filename}.extension().string();
    if (ext != ".f" && ext != ".for" && ext != ".f90" && ext != ".inc") {
        return;
    }

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

    auto args = std::vector<const char *>{};
    args.push_back(cmd.Filename.c_str());
    for (auto const& cmdLine : cmd.CommandLine) {
        if (cmdLine.starts_with("-I")) {
            args.push_back(cmdLine.c_str());
        }
    }
    bool success =
        CompilerInvocation::createFromArgs(flang->getInvocation(), llvm::ArrayRef<const char *>(args), diags);

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
} // namespace

using namespace clang::tooling;

namespace Codethink::lvtclp::fortran {

bool Tool::runPhysical(bool skipScan)
{
    auto action = PhysicalParseAction{this->memDb()};
    for (auto const& cmd : this->compilationDatabase->getAllCompileCommands()) {
        run(action, cmd);
    }

    return true;
}

bool Tool::runFull(bool skipPhysical)
{
    if (!skipPhysical) {
        runPhysical(/*skipScan=*/false);
    }
    auto action = LogicalParseAction{this->memDb()};
    for (auto const& cmd : this->compilationDatabase->getAllCompileCommands()) {
        run(action, cmd);
    }

    return true;
}

} // namespace Codethink::lvtclp::fortran
