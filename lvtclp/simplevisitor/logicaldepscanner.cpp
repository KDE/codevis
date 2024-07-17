// ct_lvtclp_logicaldepscanner.cpp                                    -*-C++-*-

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

#include "logicaldepscanner.h"
#include "logicaldepvisitor.h"

#include <clang/AST/ASTConsumer.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendActions.h>

#include <filesystem>

// ========================
// class LogicalDepConsumer
// ========================

class LogicalDepConsumer : public clang::ASTConsumer {
    // Implementation of the ASTConsumer interface for reading an AST produced
    // by the Clang parser. Each instance is specific to a file.

    // DATA
    LogicalDepVisitor d_visitor;
    // LogicalDepVisitor which inspects AST nodes

    std::optional<std::function<void(const std::string&, long)>> d_messageCallback;
    // sends a message to the UI.

    std::optional<HandleCppCommentsCallback_f> d_handleCppCommentsCallback;

    std::string d_filename;

  public:
    // CREATORS
    LogicalDepConsumer(clang::ASTContext *context,
                       clang::StringRef file,
                       const std::filesystem::path& prefix,
                       const std::filesystem::path& buildFolder,
                       std::vector<std::pair<std::string, std::string>> d_thirdPartyDirs,
                       std::optional<std::function<void(const std::string&, long)>> messageCallback,
                       bool catchCodeAnalysisOutput,
                       std::optional<HandleCppCommentsCallback_f> handleCppCommentsCallback = std::nullopt):

        // Instantiates a new LogicalDepConsumer for the given file as a
        // translation unit
        d_visitor(context,
                  file,
                  prefix,
                  buildFolder,
                  std::move(d_thirdPartyDirs),
                  std::move(messageCallback),
                  catchCodeAnalysisOutput),
        d_filename(file.str())
    {
    }

    // MANIPULATORS
    bool HandleTopLevelDecl(clang::DeclGroupRef declGroupRef) override
    // Override the method that gets called for each parsed top-level
    // declaration
    {
        for (auto *b : declGroupRef) {
            // Traverse the declaration using our AST visitor.
            d_visitor.TraverseDecl(b);
        }

        return true;
    }
};

// ==============================
// class LogicalDepFrontendAction
// ==============================

class LogicalDepFrontendAction : public clang::SyntaxOnlyAction {
    // Per-thread clang tooling stuff

    // DATA
    std::filesystem::path d_prefix;
    std::filesystem::path d_buildFolder;
    std::vector<std::pair<std::string, std::string>> d_thirdPartyDirs;

    std::function<void(const std::string&)> d_filenameCallback;
    // callback when we process a new file

    std::optional<std::function<void(const std::string&, long)>> d_messageCallback;
    // sends a message to the UI.

    bool d_catchCodeAnalysisOutput;

    std::optional<HandleCppCommentsCallback_f> d_handleCppCommentsCallback;

  public:
    // CREATORS
    LogicalDepFrontendAction(std::filesystem::path prefix,
                             std::filesystem::path buildFolder,
                             std::vector<std::pair<std::string, std::string>> thirdPartyDirs,
                             std::function<void(const std::string&)> filenameCallback,
                             std::optional<std::function<void(const std::string&, long)>> messageCallback,
                             bool catchCodeAnalysisOutput,
                             std::optional<HandleCppCommentsCallback_f> handleCppCommentsCallback = std::nullopt):
        d_prefix(std::move(prefix)),
        d_buildFolder(buildFolder),
        d_thirdPartyDirs(std::move(thirdPartyDirs)),
        d_filenameCallback(std::move(filenameCallback)),
        d_messageCallback(std::move(messageCallback)),
        d_catchCodeAnalysisOutput(catchCodeAnalysisOutput),
        d_handleCppCommentsCallback(std::move(handleCppCommentsCallback))
    {
    }

    // MANIPULATORS
    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance& compiler,
                                                          clang::StringRef file) override
    {
        // try our best not to bail on compilation errors
        clang::FrontendOptions& fOpts = compiler.getFrontendOpts();
        fOpts.FixWhatYouCan = true;
        fOpts.FixAndRecompile = true;
        fOpts.FixToTemporaries = true;

        // Attempt to obliterate any -Werror options
        clang::DiagnosticOptions& dOpts = compiler.getDiagnosticOpts();
        dOpts.Warnings = {"-Wno-everything"};

        return std::make_unique<LogicalDepConsumer>(&compiler.getASTContext(),
                                                    file,
                                                    d_prefix,
                                                    d_buildFolder,
                                                    d_thirdPartyDirs,
                                                    d_messageCallback,
                                                    d_catchCodeAnalysisOutput,
                                                    d_handleCppCommentsCallback);
    }

    bool BeginSourceFileAction(clang::CompilerInstance& ci) override
    {
        d_filenameCallback(getCurrentFile().str());

        return SyntaxOnlyAction::BeginSourceFileAction(ci);
    }

    void EndSourceFileAction() override
    // Callback at the end of processing a single input
    {
    }
};

LogicalDepActionFactory::LogicalDepActionFactory(
    std::filesystem::path prefix,
    std::filesystem::path buildFolder,
    std::vector<std::pair<std::string, std::string>> thirdPartyDirs,
    std::function<void(const std::string&)> filenameCallback,
    std::optional<std::function<void(const std::string&, long)>> messageCallback,
    bool catchCodeAnalysisOutput,
    std::optional<HandleCppCommentsCallback_f> handleCppCommentsCallback):
    d_prefix(std::move(prefix)),
    d_buildFolder(buildFolder),
    d_thirdPartyDirs(std::move(thirdPartyDirs)),
    d_filenameCallback(std::move(filenameCallback)),
    d_messageCallback(std::move(messageCallback)),
    d_catchCodeAnalysisOutput(catchCodeAnalysisOutput),
    d_handleCppCommentsCallback(std::move(handleCppCommentsCallback))
{
}

std::unique_ptr<clang::FrontendAction> LogicalDepActionFactory::create()
{
    return std::make_unique<LogicalDepFrontendAction>(d_prefix,
                                                      d_buildFolder,
                                                      d_thirdPartyDirs,
                                                      d_filenameCallback,
                                                      d_messageCallback,
                                                      d_catchCodeAnalysisOutput,
                                                      d_handleCppCommentsCallback);
}
