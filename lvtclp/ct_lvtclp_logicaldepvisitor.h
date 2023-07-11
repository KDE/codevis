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

#include <ct_lvtclp_clputil.h>
#include <ct_lvtclp_staticfnhandler.h>
#include <ct_lvtclp_visitlog.h>

#include <ct_lvtshr_graphenums.h>

#include <clang/AST/AST.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Frontend/ASTConsumers.h>

#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace Codethink {

namespace lvtmdb {
class FieldObject;
}
namespace lvtmdb {
class FileObject;
}
namespace lvtmdb {
class MethodObject;
}
namespace lvtmdb {
class NamespaceObject;
}
namespace lvtmdb {
class ObjectStore;
}
namespace lvtmdb {
class PackageObject;
}
namespace lvtmdb {
class TypeObject;
}

namespace lvtclp {

// =======================
// class LogicalDepVisitor
// =======================

class LogicalDepVisitor : public clang::RecursiveASTVisitor<LogicalDepVisitor> {
    // Implements RecursiveASTVisitor and specifies which AST nodes it is
    // interested in by overriding the relevant methods. These methods are
    // invoked as nodes of that type are encountered in the AST. For example,
    // VistNamespaceDecl is invoked for each namespace declaration. These methods
    // then add the relevant data to the database.

    // DATA
    clang::ASTContext *Context;
    // Holds long-lived AST nodes (such as types and decls) that can be
    // referred to throughout the semantic analysis of a file

    lvtmdb::FileObject *sourceFilePtr;
    // The database row representing the source file of the translation unit

    lvtmdb::ObjectStore& d_memDb;
    // The active database session to add objects to

    std::filesystem::path d_prefix;

    std::vector<std::filesystem::path> d_nonLakosianDirs;
    std::vector<std::pair<std::string, std::string>> d_thirdPartyDirs;

    std::shared_ptr<VisitLog> d_visitLog_p;

    std::shared_ptr<StaticFnHandler> d_staticFnHandler_p;

    std::optional<std::function<void(const std::string&, long)>> d_messageCallback;

    bool d_catchCodeAnalysisOutput;

    // MANIPULATORS
    static const clang::CXXRecordDecl *qualTypeToRecordDecl(clang::QualType qualType);
    // Converts a QualType to an underlying C++ class type. If the QualType
    // has no underlying C++ class, it returns nullptr.

    void processMethodArg(const clang::VarDecl *varDecl,
                          lvtmdb::TypeObject *containerDecl,
                          clang::AccessSpecifier access = clang::AS_none);
    // If the appropriate access specifier is not known, leave it as AS_none,
    // and it can be figured out for normal method arguments or for lambda
    // arguments for a lambda declared inside a method

    void writeMethodArgRelation(const clang::Decl *decl,
                                clang::QualType type,
                                clang::AccessSpecifier access,
                                lvtmdb::TypeObject *parentClassPtr,
                                lvtmdb::MethodObject *methodDeclarationPtr);
    // Writes relations for the class in 'declPtr' for
    // where it is used as an argument type in a method in the parent class

    void parseArgumentTemplate(const clang::Decl *decl,
                               const clang::TemplateSpecializationType *templateTypePtr,
                               clang::AccessSpecifier access,
                               lvtmdb::TypeObject *parentClassPtr,
                               lvtmdb::MethodObject *methodDeclarationPtr);
    // Decompose a template type into its components

    void writeFieldRelations(const clang::Decl *decl,
                             clang::QualType fieldClass,
                             lvtmdb::TypeObject *parentClassPtr,
                             lvtmdb::FieldObject *fieldDeclarationPtr);
    // Writes appropriate relations for the class in 'declPtr'
    // for where it is used as a type in a field in the parent class

    void parseFieldTemplate(const clang::Decl *decl,
                            const clang::TemplateSpecializationType *templateTypePtr,
                            lvtmdb::TypeObject *parentClassPtr,
                            lvtmdb::FieldObject *fieldDeclarationPtr);
    // Write UsesInTheImplementation relations for the component types of the
    // templateTypePtr

    void visitLocalVarDeclOrParam(clang::VarDecl *varDecl);
    // Visit a variable declaration inside a function/method

    std::string getSignature(const clang::FunctionDecl *functionDecl);
    // Obtains the type signature for a function as a string

    void processBaseClassTemplateArgs(const clang::Decl *decl,
                                      lvtmdb::TypeObject *parent,
                                      const clang::CXXRecordDecl::base_class_range& bases);
    // Add uses in the interface relations for any template arguments to
    // base classes

    void addField(lvtmdb::TypeObject *parent, const clang::ValueDecl *decl, bool isStatic);
    // Add a field (described by `decl`) inside `parent` to the database

    std::vector<lvtmdb::TypeObject *>
    getTemplateArguments(clang::QualType type, const clang::Decl *decl, const char *desc);
    // Gets template arguments of type (if any)

    void addRelation(lvtmdb::TypeObject *source,
                     clang::QualType targetType,
                     const clang::Decl *decl,
                     clang::AccessSpecifier access,
                     const char *desc);
    // (Preferred version)
    // Add a relationship between source and target, choosing usesInImpl or
    // usesInInterface depending on the access specifier. decl and desc are
    // used to print a message using debugRelationship() when debugging is
    // enabled
    // Adds appropriate relationships for any template args of targetType

    void addRelation(lvtmdb::TypeObject *source,
                     lvtmdb::TypeObject *target,
                     const clang::Decl *decl,
                     clang::AccessSpecifier access,
                     const char *desc);
    // (Overload only for when the QualType of target is unavailable)
    // Add a relationship between source and target, choosing usesInImpl or
    // usesInInterface depending on the access specifier. decl and desc are
    // used to print a message using debugRelationship() when debugging is
    // enabled

    static void debugRelationship(
        lvtmdb::TypeObject *source, lvtmdb::TypeObject *target, const clang::Decl *decl, bool impl, const char *desc);

    static bool funcIsTemplateSpecialization(const clang::FunctionDecl *decl);
    static bool classIsTemplateSpecialization(const clang::CXXRecordDecl *decl);
    static bool methodIsTemplateSpecialization(const clang::CXXMethodDecl *decl);

    static bool skipRecord(const clang::CXXRecordDecl *recordDecl);
    // Determine whether this is a record which should/might be added to the
    // database

    lvtmdb::TypeObject *lookupUDT(const clang::NamedDecl *decl, const char *warnMsg);
    // Look up a UDT possibly already in the database. If not found return
    // a nullptr.
    // If warnMsg is non-null, print a warning when the class lookup fails.

    lvtmdb::TypeObject *lookupParentRecord(const clang::CXXRecordDecl *record, const char *warnMsg);
    // Like lookupRecord() except this will also resolve lambdas to their
    // containing class (and lambdas-in-lambdas, etc). This is useful for
    // finding the "real" class which should be used in a relation

    lvtmdb::TypeObject *
    lookupType(const clang::Decl *decl, clang::QualType qualType, const char *warnMsg, bool resolveParent);

    std::string getReturnType(const clang::FunctionDecl *fnDecl);
    // Get a normalized return type for a function

    void processChildStatements(const clang::Decl *decl, const clang::Stmt *top, lvtmdb::TypeObject *parent);
    // Search statements recursively from top for any relations we need to
    // add for parentDecl.

    std::vector<lvtmdb::TypeObject *>
    listChildStatementRelations(const clang::Decl *decl, const clang::Stmt *top, lvtmdb::TypeObject *parent);
    // Like processChildStatements except the uses-in-the-implementation
    // relations are listed in a vector instead of added

    void addReturnRelation(const clang::FunctionDecl *func, lvtmdb::TypeObject *parent, clang::AccessSpecifier access);
    // Adds a relation from parent for func's return value, according to the
    // access specifier. If parent is a lambda, a relation will be added from
    // parent's parent, etc.

    void parseReturnTemplate(const clang::Decl *decl,
                             clang::QualType returnQType,
                             lvtmdb::TypeObject *parent,
                             clang::AccessSpecifier access);
    // Recursively parse and add relations for a templated return type.
    // If this is a non-templated type, the relation will be added.

    void addUDTSourceFile(lvtmdb::TypeObject *udt, const clang::Decl *decl);
    // Add the location of the decl to the source files for that UDT

    lvtmdb::TypeObject *addUDT(const clang::NamedDecl *nameDecl, lvtshr::UDTKind kind);
    // Adds a clang::NamedDecl to the database as a user defined type

    void visitTypeAlias(const clang::NamedDecl *nameDecl, clang::QualType referencedType);
    // Process some kind of type alias described by nameDecl. The underlying
    // type referred to by the new name is referencedType

    static std::string getTemplateParameters(const clang::FunctionDecl *functionDecl);
    // Format the template parameters (if present) is a function decl

  public:
    // CREATORS
    LogicalDepVisitor(clang::ASTContext *context,
                      clang::StringRef file,
                      lvtmdb::ObjectStore& memDb,
                      std::filesystem::path prefix,
                      std::vector<std::filesystem::path> nonLakosians,
                      std::vector<std::pair<std::string, std::string>> d_thirdPartyDirs,
                      std::shared_ptr<VisitLog> visitLog,
                      std::shared_ptr<StaticFnHandler> staticFnHandler,
                      std::optional<std::function<void(const std::string&, long)>> d_messageCallback,
                      bool catchCodeAnalysisOutput);
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

} // namespace lvtclp

} // namespace Codethink

#endif
