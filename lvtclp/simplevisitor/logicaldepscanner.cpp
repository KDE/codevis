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
#include <clang/Basic/LangOptions.h>
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
    LogicalDepConsumer()

    // Instantiates a new LogicalDepConsumer for the given file as a
    // translation unit
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

  public:
    // CREATORS
    LogicalDepFrontendAction()
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

        clang::LangOptions& lOpts = compiler.getLangOpts();
        lOpts.DelayedTemplateParsing = false;

        // Attempt to obliterate any -Werror options
        clang::DiagnosticOptions& dOpts = compiler.getDiagnosticOpts();
        dOpts.Warnings = {"-Wno-everything"};

        return std::make_unique<LogicalDepConsumer>();
    }

    bool BeginSourceFileAction(clang::CompilerInstance& ci) override
    {
        return SyntaxOnlyAction::BeginSourceFileAction(ci);
    }

    void EndSourceFileAction() override
    // Callback at the end of processing a single input
    {
    }
};

LogicalDepActionFactory::LogicalDepActionFactory()
{
}

std::unique_ptr<clang::FrontendAction> LogicalDepActionFactory::create()
{
    return std::make_unique<LogicalDepFrontendAction>();
}
