// ct_lvtclp_logicaldepvisitor.h                                      -*-C++-*-

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

#ifndef INCLUDED_CT_LVTCLP_LOGICALDEPVISITOR
#define INCLUDED_CT_LVTCLP_LOGICALDEPVISITOR

//@PURPOSE: Provides callbacks invoked by clang when traversing its AST
//
//@CLASSES:
//  lvtclp::LogicalDepVistor Implements clang::RecursiveASTVisitor
//                           Not for use outside lvtclp
//
//@SEE_ALSO:
//
//@DESCRIPTION:
// See https://clang.llvm.org/docs/RAVFrontendAction.html
//
// clang::RecursiveASTVisitor provides callbacks for types of nodes in clang's
// abstract syntax tree representation of a C++ source file. Our callbacks
// add these nodes to the code database

#include <clang/AST/AST.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Frontend/ASTConsumers.h>

// =======================
// class LogicalDepVisitor
// =======================

class LogicalDepVisitor : public clang::RecursiveASTVisitor<LogicalDepVisitor> {
    // Implements RecursiveASTVisitor and specifies which AST nodes it is
    // interested in by overriding the relevant methods. These methods are
    // invoked as nodes of that type are encountered in the AST. For example,
    // VistNamespaceDecl is invoked for each namespace declaration. These methods
    // then add the relevant data to the database.

  public:
    // CREATORS
    LogicalDepVisitor();
    // Instantiate a new LogicalDepVisitor for the given file as a
    // translation unit

    bool VisitNamespaceDecl(clang::NamespaceDecl *namespaceDecl);
    // Extract data about the namespace hierarchy

    bool VisitCXXRecordDecl(clang::CXXRecordDecl *recordDecl);
    // Extract data about the C++ classes

    bool VisitFunctionDecl(clang::FunctionDecl *functionDecl);
    // Extract data about C++ static and member functions and stand along
    // functions within namespaces

    bool VisitFieldDecl(clang::FieldDecl *fieldDecl);
    // Extract data about the fields within a class

    bool VisitVarDecl(clang::VarDecl *varDecl);
    // Extract data about the variable declarations

    bool VisitCXXMethodDecl(clang::CXXMethodDecl *methodDecl);
    // Scan through each method for temporary objects (which imply usesInImpl)

    bool VisitUsingDecl(clang::UsingDecl *usingDecl);
    // Kind of a type alias e.g.
    // namespace foo { class C; }
    // namespace bar { using foo::C; }
    // bar::C === foo::C

    bool VisitTypedefNameDecl(clang::TypedefNameDecl *nameDecl);
    // Visit type aliases (e.g. using foo = bar;)
    // and typedefs

    bool VisitEnumDecl(clang::EnumDecl *enumDecl);
    // Extract data about enums (scoped or not)
};

#endif
