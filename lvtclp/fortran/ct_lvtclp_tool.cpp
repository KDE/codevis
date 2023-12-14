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

namespace {

std::vector<std::filesystem::path> filterFortranFiles(std::vector<std::filesystem::path> const& files)
{
    auto fortranFiles = std::vector<std::filesystem::path>{};
    for (auto const& file : files) {
        auto ext = file.extension().string();
        if (ext != ".f" && ext != ".for" && ext != ".f90" && ext != ".inc") {
            continue;
        }
        fortranFiles.emplace_back(file);
    }
    return fortranFiles;
}

std::vector<std::filesystem::path> extractFilesFromCompileCommands(std::filesystem::path const& compileCommandsJson)
{
    auto errorMessage = std::string{};
    auto jsonDb =
        clang::tooling::JSONCompilationDatabase::loadFromFile(compileCommandsJson.string(),
                                                              errorMessage,
                                                              clang::tooling::JSONCommandLineSyntax::AutoDetect);

    auto fortranFiles = std::vector<std::filesystem::path>{};
    for (auto const& cmd : jsonDb->getAllCompileCommands()) {
        auto filename = std::filesystem::path{cmd.Filename};
        if (filename.is_relative()) {
            std::cout << "Warning: Relative path is not currently handled. Skipping " << filename << "\n";
            continue;
        }
        fortranFiles.emplace_back(filename);
    }
    return fortranFiles;
}

} // namespace

namespace Codethink::lvtclp::fortran {

using namespace Fortran::frontend;

Tool::Tool(std::filesystem::path const& compileCommandsJson):
    files(filterFortranFiles(extractFilesFromCompileCommands(compileCommandsJson)))
{
}

Tool::Tool(std::vector<std::filesystem::path> const& files): files(filterFortranFiles(files))
{
}

void run(FrontendAction& act, std::string const& filename)
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

    auto args = std::array<const char *, 1>{filename.c_str()};
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
    auto action = PhysicalParseAction{this->memDb()};
    for (auto const& f : files) {
        run(action, f.string());
    }

    return true;
}

bool Tool::runFull(bool skipPhysical)
{
    if (!skipPhysical) {
        runPhysical(/*skipScan=*/false);
    }
    auto action = LogicalParseAction{this->memDb()};
    for (auto const& f : files) {
        run(action, f);
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