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

#include <ct_lvtclp_logicaldepscanner.h>
#include <ct_lvtclp_logicaldepvisitor.h>

#include <ct_lvtclp_staticfnhandler.h>
#include <ct_lvtclp_visitlog.h>

#include <clang/AST/ASTConsumer.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendActions.h>

#include <ct_lvtclp_diagnostic_consumer.h>

#include <filesystem>

namespace {

using namespace Codethink;
using namespace lvtclp;

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
    clang::ASTContext *d_context;

  public:
    // CREATORS
    LogicalDepConsumer(clang::ASTContext *context,
                       clang::StringRef file,
                       lvtmdb::ObjectStore& memDb,
                       const std::filesystem::path& prefix,
                       const std::vector<std::filesystem::path>& nonLakosians,
                       std::vector<std::pair<std::string, std::string>> d_thirdPartyDirs,
                       const std::shared_ptr<VisitLog>& visitLog,
                       const std::shared_ptr<StaticFnHandler>& staticFnHandler,
                       std::optional<std::function<void(const std::string&, long)>> messageCallback,
                       bool catchCodeAnalysisOutput,
                       std::optional<HandleCppCommentsCallback_f> handleCppCommentsCallback = std::nullopt):

        // Instantiates a new LogicalDepConsumer for the given file as a
        // translation unit
        d_visitor(context,
                  file,
                  memDb,
                  prefix,
                  nonLakosians,
                  std::move(d_thirdPartyDirs),
                  visitLog,
                  staticFnHandler,
                  std::move(messageCallback),
                  catchCodeAnalysisOutput),
        d_handleCppCommentsCallback(std::move(handleCppCommentsCallback)),
        d_filename(file.str()),
        d_context(context)
    {
    }

    // MANIPULATORS
    bool HandleTopLevelDecl(clang::DeclGroupRef declGroupRef) override
    // Override the method that gets called for each parsed top-level
    // declaration
    {
        if (d_handleCppCommentsCallback) {
            auto& ctx = *d_context;
            auto& sm = ctx.getSourceManager();
            auto *commentsInFile = ctx.Comments.getCommentsInFile(sm.getMainFileID());
            if (commentsInFile) {
                for (auto&& [_, rawComment] : *commentsInFile) {
                    auto filename = std::filesystem::path{d_filename}.filename().string();
                    auto briefText = rawComment->getBriefText(ctx);
                    auto startLine = sm.getExpansionLineNumber(rawComment->getBeginLoc());
                    auto endLine = sm.getExpansionLineNumber(rawComment->getEndLoc());
                    (*d_handleCppCommentsCallback)(filename, briefText, startLine, endLine);
                }
            }
        }

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
    lvtmdb::ObjectStore& d_memDb;

    std::filesystem::path d_prefix;

    std::vector<std::filesystem::path> d_nonLakosianDirs;
    std::vector<std::pair<std::string, std::string>> d_thirdPartyDirs;

    std::shared_ptr<VisitLog> d_visitLog_p;
    std::shared_ptr<StaticFnHandler> d_staticFnHandler_p;

    std::function<void(const std::string&)> d_filenameCallback;
    // callback when we process a new file

    std::optional<std::function<void(const std::string&, long)>> d_messageCallback;
    // sends a message to the UI.

    bool d_catchCodeAnalysisOutput;

    std::optional<HandleCppCommentsCallback_f> d_handleCppCommentsCallback;

  public:
    // CREATORS
    LogicalDepFrontendAction(lvtmdb::ObjectStore& memDb,
                             std::filesystem::path prefix,
                             std::vector<std::filesystem::path> nonLakosians,
                             std::vector<std::pair<std::string, std::string>> thirdPartyDirs,
                             std::function<void(const std::string&)> filenameCallback,
                             std::optional<std::function<void(const std::string&, long)>> messageCallback,
                             bool catchCodeAnalysisOutput,
                             std::optional<HandleCppCommentsCallback_f> handleCppCommentsCallback = std::nullopt):
        d_memDb(memDb),
        d_prefix(std::move(prefix)),
        d_nonLakosianDirs(std::move(nonLakosians)),
        d_thirdPartyDirs(std::move(thirdPartyDirs)),
        d_visitLog_p(std::make_shared<VisitLog>()),
        d_staticFnHandler_p(std::make_shared<StaticFnHandler>(memDb)),
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
                                                    d_memDb,
                                                    d_prefix,
                                                    d_nonLakosianDirs,
                                                    d_thirdPartyDirs,
                                                    d_visitLog_p,
                                                    d_staticFnHandler_p,
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
        d_staticFnHandler_p->writeOutToDb();
        d_staticFnHandler_p->reset();
    }
};

} // namespace

namespace Codethink::lvtclp {

LogicalDepActionFactory::LogicalDepActionFactory(
    lvtmdb::ObjectStore& memDb,
    std::filesystem::path prefix,
    std::vector<std::filesystem::path> nonLakosians,
    std::vector<std::pair<std::string, std::string>> thirdPartyDirs,
    std::function<void(const std::string&)> filenameCallback,
    std::optional<std::function<void(const std::string&, long)>> messageCallback,
    bool catchCodeAnalysisOutput,
    std::optional<HandleCppCommentsCallback_f> handleCppCommentsCallback):
    d_memDb(memDb),
    d_prefix(std::move(prefix)),
    d_nonLakosianDirs(std::move(nonLakosians)),
    d_thirdPartyDirs(std::move(thirdPartyDirs)),
    d_filenameCallback(std::move(filenameCallback)),
    d_messageCallback(std::move(messageCallback)),
    d_catchCodeAnalysisOutput(catchCodeAnalysisOutput),
    d_handleCppCommentsCallback(std::move(handleCppCommentsCallback))
{
}

std::unique_ptr<clang::FrontendAction> LogicalDepActionFactory::create()
{
    return std::make_unique<LogicalDepFrontendAction>(d_memDb,
                                                      d_prefix,
                                                      d_nonLakosianDirs,
                                                      d_thirdPartyDirs,
                                                      d_filenameCallback,
                                                      d_messageCallback,
                                                      d_catchCodeAnalysisOutput,
                                                      d_handleCppCommentsCallback);
}

} // namespace Codethink::lvtclp
