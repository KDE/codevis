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
#include <clang/Tooling/JSONCompilationDatabase.h>
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

using namespace Fortran::frontend;
using namespace clang::tooling;

namespace {

void run(FrontendAction& act, CompileCommand const& cmd)
{
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

namespace Codethink::lvtclp::fortran {

Tool::Tool(std::unique_ptr<CompilationDatabase> compilationDatabase):
    compilationDatabase(std::move(compilationDatabase))
{
}

std::unique_ptr<Tool> Tool::fromCompileCommands(std::filesystem::path const& compileCommandsJson)
{
    auto errorMessage = std::string{};
    auto jsonDb = JSONCompilationDatabase::loadFromFile(compileCommandsJson.string(),
                                                        errorMessage,
                                                        clang::tooling::JSONCommandLineSyntax::AutoDetect);
    // TODO: Proper error management
    if (!errorMessage.empty()) {
        std::cout << "Tool::fromCompileCommands error: " << errorMessage;
        return nullptr;
    }

    return std::make_unique<Tool>(std::move(jsonDb));
}

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

lvtmdb::ObjectStore& Tool::getObjectStore()
{
    return this->memDb();
}

void Tool::setSharedMemDb(std::shared_ptr<lvtmdb::ObjectStore> const& sharedMemDb)
{
    this->sharedMemDb = sharedMemDb;
}

} // namespace Codethink::lvtclp::fortran
