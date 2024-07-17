// lvtclp_codebasedbvisitor.cpp                                       -*-C++-*-

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

#include "logicaldepvisitor.h"

#include <clang/AST/ASTContext.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/Version.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Lex/PPCallbacks.h>
#include <clang/Rewrite/Core/Rewriter.h>

#include <iostream>

#include <functional>
#include <string>

LogicalDepVisitor::LogicalDepVisitor(clang::ASTContext *Context,
                                     clang::StringRef file,
                                     std::filesystem::path prefix,
                                     std::filesystem::path buildFolder,
                                     std::vector<std::pair<std::string, std::string>> thirdPartyDirs,
                                     std::optional<std::function<void(const std::string&, long)>> messageCallback,
                                     bool catchCodeAnalysisOutput):
    Context(Context),
    d_prefix(std::filesystem::weakly_canonical(prefix)),
    d_buildFolder(buildFolder),
    d_thirdPartyDirs(std::move(thirdPartyDirs)),
    d_messageCallback(std::move(messageCallback)),
    d_catchCodeAnalysisOutput(catchCodeAnalysisOutput)
{
}

bool LogicalDepVisitor::VisitNamespaceDecl(clang::NamespaceDecl *namespaceDecl)
{
    return true;
}

bool LogicalDepVisitor::VisitCXXRecordDecl(clang::CXXRecordDecl *recordDecl)
{
    return true;
}

bool LogicalDepVisitor::VisitFunctionDecl(clang::FunctionDecl *functionDecl)
{
    return true;
}

bool LogicalDepVisitor::VisitFieldDecl(clang::FieldDecl *fieldDecl)
{
    return true; // RETURN
}

bool LogicalDepVisitor::VisitVarDecl(clang::VarDecl *varDecl)
{
    std::cout << "Visiting variable " << varDecl->getDeclName().getAsString() << std::endl;
    return true;
}

bool LogicalDepVisitor::VisitCXXMethodDecl(clang::CXXMethodDecl *methodDecl)
{
    return true;
}

bool LogicalDepVisitor::VisitUsingDecl(clang::UsingDecl *usingDecl)
{
    return true;
}

bool LogicalDepVisitor::VisitTypedefNameDecl(clang::TypedefNameDecl *nameDecl)
{
    return true;
}

bool LogicalDepVisitor::VisitEnumDecl(clang::EnumDecl *enumDecl)
{
    return true;
}
