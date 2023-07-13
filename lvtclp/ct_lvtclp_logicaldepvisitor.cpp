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

#include <ct_lvtclp_logicaldepvisitor.h>

#include <ct_lvtmdb_componentobject.h>
#include <ct_lvtmdb_errorobject.h>
#include <ct_lvtmdb_fieldobject.h>
#include <ct_lvtmdb_fileobject.h>
#include <ct_lvtmdb_functionobject.h>
#include <ct_lvtmdb_methodobject.h>
#include <ct_lvtmdb_namespaceobject.h>
#include <ct_lvtmdb_objectstore.h>
#include <ct_lvtmdb_packageobject.h>
#include <ct_lvtmdb_typeobject.h>
#include <ct_lvtmdb_variableobject.h>

#include <ct_lvtshr_graphenums.h>

#include <ct_lvtshr_stringhelpers.h>

#include <ct_lvtclp_clputil.h>

#include <QDebug>

#include <boost/algorithm/string.hpp>

#include <clang/AST/ASTContext.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/Version.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Lex/PPCallbacks.h>
#include <clang/Rewrite/Core/Rewriter.h>

#include <llvm/Support/raw_ostream.h>

#include <functional>
#include <string>

#include <thread>

namespace {

// clang::Type::isTypedefNameType (not in older clang)
// Determines whether this type is written as a typedef-name.
bool typeIsTypedefNameType(const clang::Type *type)
{
    if (type->getAs<clang::TypedefType>()) {
        return true;
    }
    if (const auto *TST = type->getAs<clang::TemplateSpecializationType>()) {
        return TST->isTypeAlias();
    }
    return false;
}

// remove a prefix from the start of a string
std::string removeStrPrefix(const std::string& str, const char *prefix)
{
    const std::size_t loc = str.find(prefix);
    if (loc == 0) {
        return str.substr(strlen(prefix));
    }
    return str;
}

// Remove everyting in a string after a particular char
std::string truncStrAfterChar(const std::string& str, const char c)
{
    const std::size_t loc = str.find(c);
    if (loc != std::string::npos) {
        return str.substr(0, loc);
    }
    return str;
}

// Remove trailing spaces from string
std::string trimTrailingSpaces(const std::string& str)
{
    std::size_t loc = str.find_last_not_of(' ');
    return str.substr(0, loc + 1);
}

template<typename TARGET>
void searchClangStmtAST(const clang::Stmt *top, std::function<void(const TARGET *)> visitor)
// Recursively search AST starting at `top`, calling `visitor` on any
// TARGETs we find
{
    if (!top) {
        return;
    }

    auto *target = clang::dyn_cast<TARGET>(top);
    if (target) {
        visitor(target);
    }

    // recurse into statement children
    std::for_each(top->child_begin(), top->child_end(), [&visitor](const clang::Stmt *top) {
        searchClangStmtAST(top, visitor);
    });
}

} // unnamed namespace

namespace Codethink::lvtclp {

LogicalDepVisitor::LogicalDepVisitor(clang::ASTContext *Context,
                                     clang::StringRef file,
                                     lvtmdb::ObjectStore& memDb,
                                     std::filesystem::path prefix,
                                     std::vector<std::filesystem::path> nonLakosians,
                                     std::vector<std::pair<std::string, std::string>> thirdPartyDirs,
                                     std::shared_ptr<VisitLog> visitLog,
                                     std::shared_ptr<StaticFnHandler> staticFnHandler,
                                     std::optional<std::function<void(const std::string&, long)>> messageCallback,
                                     bool catchCodeAnalysisOutput):
    Context(Context),
    d_memDb(memDb),
    d_prefix(std::move(prefix)),
    d_nonLakosianDirs(std::move(nonLakosians)),
    d_thirdPartyDirs(std::move(thirdPartyDirs)),
    d_visitLog_p(std::move(visitLog)),
    d_staticFnHandler_p(std::move(staticFnHandler)),
    d_messageCallback(std::move(messageCallback)),
    d_catchCodeAnalysisOutput(catchCodeAnalysisOutput)
{
    sourceFilePtr = ClpUtil::writeSourceFile(file.str(), false, d_memDb, d_prefix, d_nonLakosianDirs, d_thirdPartyDirs);
}

bool LogicalDepVisitor::VisitNamespaceDecl(clang::NamespaceDecl *namespaceDecl)
{
    if (d_visitLog_p->alreadyVisited(namespaceDecl, clang::Decl::Kind::Namespace)) {
        return true;
    }

    std::string name = namespaceDecl->getNameAsString();

    if (!namespaceDecl->hasExternalFormalLinkage() || name.empty()) {
        // anon namespace
        return true; // RETURN
    }

    std::string qualifiedName = namespaceDecl->getQualifiedNameAsString();
    std::string parentName = qualifiedName.substr(0, qualifiedName.length() - name.length() - 2);

    lvtmdb::NamespaceObject *namespacePtr = nullptr;
    d_memDb.withROLock([&] {
        namespacePtr = d_memDb.getNamespace(qualifiedName);
    });
    if (!namespacePtr) {
        lvtmdb::NamespaceObject *parentNamespace = nullptr;
        d_memDb.withRWLock([&] {
            parentNamespace = d_memDb.getNamespace(parentName);
            namespacePtr = d_memDb.getOrAddNamespace(qualifiedName, std::move(name), parentNamespace);
        });
        if (parentNamespace) {
            parentNamespace->withRWLock([&] {
                parentNamespace->addChild(namespacePtr);
            });
        }
    }

    const clang::SourceManager& srcMgr = Context->getSourceManager();
    std::string sourceFile = ClpUtil::getRealPath(namespaceDecl->getLocation(), srcMgr);
    lvtmdb::FileObject *filePtr =
        ClpUtil::writeSourceFile(sourceFile, true, d_memDb, d_prefix, d_nonLakosianDirs, d_thirdPartyDirs);
    if (namespacePtr) {
        namespacePtr->withRWLock([&] {
            namespacePtr->addFile(filePtr);
        });
    }
    filePtr->withRWLock([&] {
        filePtr->addNamespace(namespacePtr);
    });

    return true;
}

bool LogicalDepVisitor::skipRecord(const clang::CXXRecordDecl *recordDecl)
{
    // local class isn't architecturally significant
    // empty name => anon class
    // not external linkage => non-unique fully qualified name
    return recordDecl->isLocalClass() || recordDecl->getName().empty() || !recordDecl->hasExternalFormalLinkage();
}

lvtmdb::TypeObject *LogicalDepVisitor::lookupUDT(const clang::NamedDecl *decl, const char *warnMsg)
{
    if (!decl) {
        return nullptr;
    }
    if (decl->getKind() == clang::Decl::Kind::CXXRecord) {
        const auto *recordDecl = clang::cast<clang::CXXRecordDecl>(decl);
        if (skipRecord(recordDecl)) {
            return nullptr;
        }
        // this is an internal clang type but clang doesn't recognise it as a
        // built-in type
        if (recordDecl->getQualifiedNameAsString() == "__va_list_tag") {
            return nullptr;
        }
    }

    const std::string qualifiedName = decl->getQualifiedNameAsString();

    lvtmdb::TypeObject *mdbUDT = nullptr;
    d_memDb.withROLock([&] {
        mdbUDT = d_memDb.getType(qualifiedName);
    });

    if (warnMsg && !mdbUDT) {
        std::string message = "WARN: lookupUDT for " + qualifiedName + " failed for " + warnMsg + "\n"
            + "      while processing decl at " + decl->getLocation().printToString(Context->getSourceManager()) + "\n";

        if (d_messageCallback) {
            auto& fnc = *d_messageCallback;
            fnc(message, ClpUtil::getThreadId());
        }

        d_memDb.withRWLock([&] {
            d_memDb.getOrAddError(lvtmdb::MdbUtil::ErrorKind::ParserError,
                                  qualifiedName,
                                  "lookupUDT failed",
                                  decl->getLocation().printToString(Context->getSourceManager()));
        });
    }

    return mdbUDT;
}

lvtmdb::TypeObject *LogicalDepVisitor::lookupParentRecord(const clang::CXXRecordDecl *record, const char *warnMsg)
{
    if (!record) {
        return nullptr;
    }

    // base case
    if (!record->isLambda()) {
        return lookupUDT(record, warnMsg);
    }

    const clang::DeclContext *lambdaContext = record->getDeclContext();
    assert(lambdaContext);

    const clang::CXXRecordDecl *parent;
    const clang::CXXMethodDecl *method;

    switch (lambdaContext->getDeclKind()) {
    case clang::Decl::Kind::CXXMethod:
        method = clang::cast<clang::CXXMethodDecl>(lambdaContext);
        parent = method->getParent();
        break;

    case clang::Decl::Kind::CXXRecord:
        parent = clang::cast<clang::CXXRecordDecl>(lambdaContext);
        break;

    default:
        // I don't think there can be any other ways for a lambda to be inside
        // a class (lambdas as static fields don't seem to be allowed).
        parent = nullptr;
        break;
    }

    // recurse so we can handle lambdas-in-lambdas
    return lookupParentRecord(parent, warnMsg);
}

lvtmdb::TypeObject *LogicalDepVisitor::lookupType(const clang::Decl *decl,
                                                  clang::QualType qualType,
                                                  const char *warnMsg,
                                                  const bool resolveParent)
{
    if (qualType.isNull()) {
        return nullptr;
    }
    // remove qualifiers
    qualType = qualType.getAtomicUnqualifiedType();
    qualType.removeLocalConst();

    if (qualType->isFunctionPointerType() || qualType->isFunctionReferenceType() || qualType->isMemberPointerType()
        || qualType->isMemberFunctionPointerType()) {
        // TODO
        return nullptr;
    }

    // skip some normalization of this is a type alias
    // (e.g. typedef void * Pointer) because if the
    // pointer-ness is removed from Pointer, it becomes a void. We want to look
    // up the UDT for Pointer instead. Likewise for similar transformations.
    if (!typeIsTypedefNameType(qualType.getTypePtr())) {
        // remove reference-ness
        if (qualType->isReferenceType()) {
            // recurse so that we go back to the start, removing qualifiers etc
            return lookupType(decl, qualType.getNonReferenceType(), warnMsg, resolveParent);
        }
        // skip built in types e.g. int
        if (qualType->isBuiltinType() && !typeIsTypedefNameType(qualType.getTypePtr())) {
            return nullptr;
        }
        // remove pointer-ness
        if (qualType->isPointerType() && !typeIsTypedefNameType(qualType.getTypePtr())) {
            // recurse because we can have pointers to arrays of pointers to pointers etc
            return lookupType(decl, qualType->getPointeeType(), warnMsg, resolveParent);
        }
    }

    if (qualType->isDependentType()) {
        const clang::TemplateSpecializationType *spec = qualType->getAs<clang::TemplateSpecializationType>();
        if (spec) {
            const clang::TemplateName tmpl = spec->getTemplateName();

            // Catch cases where qualType is a template parameter
            // e.g. B in:
            // template <template <class A> class B>
            // class C;
            //
            // We have to check that it is the template name that is dependent,
            // because we could have a real non-template param type which is
            // dependent because its template parameters are dependent.
            if (tmpl.isDependent()) {
                return nullptr;
            }

            // Skip sugared types so that we don't resolve the typedef (or
            // whatever is going on here).
            // One example of this is re-exporting std::Type as bsl::Type.
            if (!qualType->getAs<clang::ElaboratedType>()) {
                const auto *tmplDecl = tmpl.getAsTemplateDecl();
                assert(tmplDecl);

                if (const auto *classTempl = clang::dyn_cast<clang::ClassTemplateDecl>(tmplDecl)) {
                    clang::CXXRecordDecl *record = classTempl->getTemplatedDecl();
                    assert(record);
                    if (resolveParent) {
                        return lookupUDT(record, warnMsg);
                    }
                    return lookupParentRecord(record, warnMsg);
                }
            }
        }

        // catch cases where clang cannot resolve the type (due to an expression
        // involving a template parameter, which could have a different type
        // in different template specializations).
        if (qualType.getAsString() == "<dependent type>") {
            return nullptr;
        }

        // catch raw usage of a template type param
        if (qualType->isTemplateTypeParmType()) {
            return nullptr;
        }
    }

    // catch arrays
    if (qualType->isArrayType()) {
        // Called "unsafe" because it discards qualifiers on the outer type.
        // We remove qualifiers anyway.
        const clang::ArrayType *array = qualType->getAsArrayTypeUnsafe();
        assert(array);
        return lookupType(decl, array->getElementType(), warnMsg, resolveParent);
    }

    // resolve decltype()
    if (qualType->isDecltypeType()) {
        const auto *declType = qualType->getAs<clang::DecltypeType>();
        assert(declType);
        clang::QualType resolvedType = declType->getUnderlyingType();
        return lookupType(decl, resolvedType, warnMsg, resolveParent);
    }

    // catch auto
    if (const auto *deducedType = qualType->getAs<clang::DeducedType>()) {
        clang::QualType target = deducedType->getDeducedType();
        if (!target.isNull()) {
            return lookupType(decl, target, warnMsg, resolveParent);
        }
        return nullptr;
    }

    // qualTypeToRecordDecl will resolve the typedef to the referenced type.
    // We don't want to do that so don't do this if we are looking at a typedef
    if (!typeIsTypedefNameType(qualType.getTypePtr())) {
        const clang::CXXRecordDecl *recordDecl = qualTypeToRecordDecl(qualType);
        if (recordDecl) {
            if (resolveParent) {
                return lookupUDT(recordDecl, warnMsg);
            }
            return lookupParentRecord(recordDecl, warnMsg);
        }
    }

    // For typedefs we will have skipped the desugaring stages earlier. This can
    // leave types not fully qualified so we must resolve to a fully qualified
    // type here.
    if (const auto *elab = qualType->getAs<clang::ElaboratedType>()) {
        // weirdly, for dependent-typed things, clang will desugar to something
        // *less* qualified so skip those here. TODO!
        if (!qualType->isDependentType()) {
            clang::QualType namedType = elab->getNamedType();
            return lookupType(decl, namedType, warnMsg, resolveParent);
        }
    }

    if (qualType->getTypeClass() == clang::Type::DependentName) {
        // this name of a type cannot be resolved by clang within this context
        // e.g.
        // template <class Imp>
        // struct Foo;
        //
        // template <class IMP>
        // struct Bar {
        //     typedef Foo<IMP>::Int Int; // <---- Foo<IMP>::Int is a DependentNameType
        // };
        //
        // struct Marker1;
        // struct Marker2;
        //
        // template <>
        // struct Foo<Marker1> {
        //     typedef long Int; // <--- Foo<IMP> could be a long
        // };
        //
        // template <>
        // struct Foo<Marker2> {
        //     typedef int Int; // <--- Foo<IMP> could be an int
        // };
        //
        // Or Foo<IMP> could be something else in an as-yet unseen specialization
        //
        // TODO: one solution would be to synthesize a UDT on demand for
        //       Foo<class IMP>::Int because this is kind of like declaring that
        //       this will exist for all relevant specializations
        return nullptr;
    }

    // we couldn't do things the easy way. There are two cases here:
    //    1. template <typename T> C<T>: here we want to get C
    //    2. qualType is a a type alias, which we should look up by name
    //
    // We also have to remove "typename" from the type

    clang::PrintingPolicy policy(Context->getLangOpts());
    policy.adjustForCPlusPlus();
    policy.FullyQualifiedName = true;
    policy.PrintCanonicalTypes = false; // canonical-types resolves typedefs
    policy.IncludeTagDefinition = false;
    policy.PolishForDeclaration = true;
    policy.SuppressTagKeyword = true;
    policy.TerseOutput = true;

    std::string typeString = qualType.getAsString(policy);

    typeString = removeStrPrefix(typeString, "typename ");

    typeString = truncStrAfterChar(typeString, '<'); // case 1

    typeString = trimTrailingSpaces(typeString);

    // lookup for all cases
    lvtmdb::TypeObject *ret = nullptr;
    d_memDb.withROLock([&] {
        ret = d_memDb.getType(typeString);
    });
    if (!ret) {
        const std::string message = "WARN: lookupType for " + typeString + " failed for " + warnMsg + "\n"
            + "      while processing decl at " + decl->getLocation().printToString(Context->getSourceManager()) + "\n";

        if (d_messageCallback) {
            auto& fnc = *d_messageCallback;
            fnc(message, ClpUtil::getThreadId());
        }

        d_memDb.withRWLock([&] {
            d_memDb.getOrAddError(lvtmdb::MdbUtil::ErrorKind::ParserError,
                                  typeString,
                                  "lookupType failed",
                                  decl->getLocation().printToString(Context->getSourceManager()));
        });
    }
    return ret;
}

std::string LogicalDepVisitor::getReturnType(const clang::FunctionDecl *fnDecl)
{
    // TODO this probably needs all of the normalization in lookupType
    //      and these should share code

    clang::QualType qualType = fnDecl->getReturnType();
    if (qualType.isNull()) {
        return "";
    }
    // remove qualifiers
    qualType = qualType.getAtomicUnqualifiedType();
    qualType.removeLocalConst();

    // remove typename keyword etc
    clang::PrintingPolicy policy(Context->getLangOpts());
    policy.adjustForCPlusPlus();
    policy.FullyQualifiedName = true;
    policy.PrintCanonicalTypes = true; // resolves typedefs
    policy.IncludeTagDefinition = false;
    policy.PolishForDeclaration = true;
    policy.SuppressTagKeyword = true;
    policy.TerseOutput = true;

    std::string typeString = qualType.getAsString(policy);

    typeString = removeStrPrefix(typeString, "typename ");
    typeString = truncStrAfterChar(typeString, '<');
    typeString = trimTrailingSpaces(typeString);
    return typeString;
}

void LogicalDepVisitor::processBaseClassTemplateArgs(const clang::Decl *decl,
                                                     lvtmdb::TypeObject *parent,
                                                     const clang::CXXRecordDecl::base_class_range& bases)
{
    for (const clang::CXXBaseSpecifier& baseSpecifier : bases) {
        // if the base class is a template, we need to usesInTheInterface
        // the template arguments
        const auto *recordType = baseSpecifier.getType()->getAs<clang::RecordType>();
        if (!recordType) {
            continue;
        }
        const auto *recordDecl = recordType->getDecl()->getDefinition();
        const auto *const tmplate = clang::dyn_cast<clang::ClassTemplateSpecializationDecl>(recordDecl);
        if (!tmplate) {
            // this base class isn't a template
            continue;
        }

        const clang::TemplateArgumentList& templateArgs = tmplate->getTemplateArgs();
        for (unsigned i = 0; i < templateArgs.size(); ++i) {
            const clang::TemplateArgument& arg = templateArgs[i];
            if (arg.getKind() != clang::TemplateArgument::ArgKind::Type) {
                continue;
            }

            clang::QualType type = arg.getAsType();
            lvtmdb::TypeObject *mdbType = lookupType(decl, type, __func__, false);
            if (!mdbType) {
                continue;
            }

            if (d_catchCodeAnalysisOutput) {
                debugRelationship(parent, mdbType, decl, false, "templated base");
            }
            ClpUtil::addUsesInInter(parent, mdbType);
        }
    }
}

bool LogicalDepVisitor::VisitCXXRecordDecl(clang::CXXRecordDecl *recordDecl)
{
    if (d_visitLog_p->alreadyVisited(recordDecl,
                                     clang::Decl::Kind::CXXRecord,
                                     recordDecl->getTemplateSpecializationKind())) {
        return true;
    }

    if (skipRecord(recordDecl)) {
        return true;
    }
    if (classIsTemplateSpecialization(recordDecl)) {
        // still add source file so that we don't end up with cases where the
        // template is forward declared but only template specializations are
        // defined (never the original template): leading to there being no
        // source file for this class
        const auto *spec = clang::cast<clang::ClassTemplateSpecializationDecl>(recordDecl);
        assert(spec); // we just checked it is a template specialization

        const clang::ClassTemplateDecl *templateDecl = spec->getSpecializedTemplate();
        assert(templateDecl);

        lvtmdb::TypeObject *udt = lookupUDT(templateDecl, "expected to find template declaration for specialization");
        if (!udt) {
            return true;
        }

        addUDTSourceFile(udt, recordDecl);
        return true;
    }

    lvtshr::UDTKind recordKind;
    if (recordDecl->isClass()) {
        recordKind = lvtshr::UDTKind::Class;
    } else if (recordDecl->isStruct()) {
        recordKind = lvtshr::UDTKind::Struct;
    } else if (recordDecl->isUnion()) {
        recordKind = lvtshr::UDTKind::Union;
    } else {
        const std::string message = "WARN: clang::CXXRecordDecl which is not a class, struct or union on"
            + recordDecl->getQualifiedNameAsString() + "\n" + "at"
            + recordDecl->getLocation().printToString(Context->getSourceManager());

        if (d_messageCallback) {
            auto& fnc = *d_messageCallback;
            fnc(message, ClpUtil::getThreadId());
        }

        d_memDb.withRWLock([&] {
            d_memDb.getOrAddError(lvtmdb::MdbUtil::ErrorKind::ParserError,
                                  recordDecl->getQualifiedNameAsString(),
                                  "clang::CXXRecordDecl which is not a class, struct or union",
                                  recordDecl->getLocation().printToString(Context->getSourceManager()));
        });

        return true;
    }

    lvtmdb::TypeObject *userDefinedTypePtr = addUDT(recordDecl, recordKind);
    if (!userDefinedTypePtr) {
        return true;
    }

    if (recordDecl->hasDefinition()) {
        processBaseClassTemplateArgs(recordDecl, userDefinedTypePtr, recordDecl->bases());

        for (const auto& I : recordDecl->bases()) {
            const auto *Ty = I.getType()->getAs<clang::RecordType>();
            if (!Ty) {
                continue;
            }
            const auto *Base = clang::cast<clang::CXXRecordDecl>(Ty->getDecl()->getDefinition());

            lvtmdb::TypeObject *parentClassPtr = lookupUDT(Base, "looking up record base class");
            if (parentClassPtr) {
                lvtmdb::TypeObject::addIsARelationship(userDefinedTypePtr, parentClassPtr);
            }
        }
    }

    return true;
}

std::string LogicalDepVisitor::getTemplateParameters(const clang::FunctionDecl *functionDecl)
{
    std::string templateParameters;

    if (functionDecl->getTemplatedKind() == clang::FunctionDecl::TK_FunctionTemplate) {
        clang::FunctionTemplateDecl *templateDecl = functionDecl->getDescribedFunctionTemplate();
        std::vector<std::string> arguments;
        clang::TemplateParameterList *parameterList = templateDecl->getTemplateParameters();
        for (auto *namedDecl : *parameterList) {
            std::string argument;
            argument += "typename ";
            argument += namedDecl->getQualifiedNameAsString();
            arguments.push_back(argument);
        }

        templateParameters = "template <" + boost::algorithm::join(arguments, ", ") + ">";
    }

    return templateParameters;
}

bool LogicalDepVisitor::VisitFunctionDecl(clang::FunctionDecl *functionDecl)
{
    if (d_visitLog_p->alreadyVisited(functionDecl, clang::Decl::Kind::Function, functionDecl->getTemplatedKind())) {
        return true;
    }

    std::string name = functionDecl->getNameAsString();
    std::string qualifiedName = functionDecl->getQualifiedNameAsString();
    std::string parentName = qualifiedName.substr(0, qualifiedName.length() - name.length() - 2);

    const auto *methodDecl = llvm::dyn_cast<clang::CXXMethodDecl>(functionDecl);
    if (methodDecl) {
        if (methodIsTemplateSpecialization(methodDecl)) {
            return true;
        }
        if (!methodDecl->hasExternalFormalLinkage()) {
            // internal linkage
            return true;
        }

        std::string signature = getSignature(functionDecl);
        std::string templateParameters = getTemplateParameters(functionDecl);
        std::string returnType = getReturnType(functionDecl);

        lvtmdb::TypeObject *parentClassPtr = lookupUDT(methodDecl->getParent(), "method parent class");
        if (!parentClassPtr) {
            return true;
        }

        lvtmdb::MethodObject *methodPtr = nullptr;
        d_memDb.withROLock([&] {
            methodPtr = d_memDb.getMethod(qualifiedName, signature, templateParameters, returnType);
        });
        if (methodPtr) {
            return true; // RETURN
        }

        d_memDb.withRWLock([&] {
            methodPtr = d_memDb.getOrAddMethod(std::move(qualifiedName),
                                               std::move(name),
                                               std::move(signature),
                                               std::move(returnType),
                                               std::move(templateParameters),
                                               static_cast<lvtshr::AccessSpecifier>(methodDecl->getAccess()),
                                               methodDecl->isVirtual(),
                                               methodDecl->isPure(),
                                               methodDecl->isStatic(),
                                               methodDecl->isConst(),
                                               parentClassPtr);
        });
        parentClassPtr->withRWLock([&] {
            parentClassPtr->addMethod(methodPtr);
        });
    } else {
        if (!functionDecl->isDefined()) {
            return true;
        }

        // don't consider globally visible functions because they are their own
        // architecturally-significant thing and should eventually be drawn on
        // diagrams as themselves
        if (!functionDecl->isGlobal()) {
            // find out what classes this function uses in its implementation
            // and store them in the StaticFnHandler
            // (local var decls are handled separately)
            std::vector<lvtmdb::TypeObject *> usesInImpls =
                listChildStatementRelations(functionDecl, functionDecl->getBody(), nullptr);
            for (lvtmdb::TypeObject *dep : usesInImpls) {
                if (!dep) {
                    continue;
                }
                d_staticFnHandler_p->addFnUses(functionDecl, dep);
            }

            // find out if this function calls any functions
            auto visitCallExpr = [this, functionDecl](const clang::CallExpr *callExpr) {
                const clang::FunctionDecl *callee = callExpr->getDirectCallee();
                if (callee && !callee->isGlobal()) {
                    d_staticFnHandler_p->addFnUses(functionDecl, callee);
                }
            };
            searchClangStmtAST<clang::CallExpr>(functionDecl->getBody(), visitCallExpr);

            // we don't add file-static functions to the database because they
            // don't have a globally unique name by which we can refer to them
            return true;
        }

        if (funcIsTemplateSpecialization(functionDecl)) {
            return true;
        }

        if (!functionDecl->hasExternalFormalLinkage()) {
            // internal linkage
            return true;
        }

        if (!functionDecl->isDefined()) {
            return true;
        }

        std::string signature = getSignature(functionDecl);
        std::string templateParameters = getTemplateParameters(functionDecl);
        std::string returnType = getReturnType(functionDecl);

        lvtmdb::FunctionObject *function = nullptr;
        d_memDb.withROLock([&] {
            function = d_memDb.getFunction(qualifiedName, signature, templateParameters, returnType);
        });
        if (function) {
            return true; // RETURN
        }

        lvtmdb::NamespaceObject *parentNamespace = nullptr;
        d_memDb.withRWLock([&] {
            parentNamespace = d_memDb.getNamespace(parentName);
            function = d_memDb.getOrAddFunction(std::move(qualifiedName),
                                                std::move(name),
                                                std::move(signature),
                                                std::move(returnType),
                                                std::move(templateParameters),
                                                parentNamespace);
        });
        if (parentNamespace) {
            parentNamespace->withRWLock([&] {
                parentNamespace->addFunction(function);
            });
        }
    }

    return true;
}

void LogicalDepVisitor::addField(lvtmdb::TypeObject *parent, const clang::ValueDecl *decl, const bool isStatic)
{
    // anon member e.g.
    // struct Foo {
    //     unsigned one;
    //     int :32; // padding
    //     unsigned two;
    // };
    if (decl->getName().empty()) {
        return;
    }
    if (!parent) {
        return;
    }

    const std::string qualifiedName = decl->getQualifiedNameAsString();

    lvtmdb::FieldObject *fieldPtr = nullptr;
    d_memDb.withROLock([&] {
        fieldPtr = d_memDb.getField(qualifiedName);
    });
    if (fieldPtr) {
        // already added to the database
        return;
    }

    const clang::QualType qualType = decl->getType();
    const std::string typeString = qualType.getAsString();

    d_memDb.withRWLock([&] {
        fieldPtr = d_memDb.getOrAddField(qualifiedName,
                                         decl->getNameAsString(),
                                         typeString,
                                         static_cast<lvtshr::AccessSpecifier>(decl->getAccess()),
                                         isStatic,
                                         parent);
    });
    parent->withRWLock([&] {
        parent->addField(fieldPtr);
    });

    const clang::TemplateSpecializationType *templateTypePtr =
        qualType.getNonReferenceType()->getAs<clang::TemplateSpecializationType>();
    if (templateTypePtr) {
        parseFieldTemplate(decl, templateTypePtr, parent, fieldPtr);
    }

    const clang::CXXRecordDecl *fieldRecord = qualTypeToRecordDecl(qualType);

    if (fieldRecord && fieldRecord->isLambda()) {
        // this variable is a lambda function
        const clang::CXXMethodDecl *callOperator = fieldRecord->getLambdaCallOperator();
        assert(callOperator);

        for (const clang::ParmVarDecl *param : callOperator->parameters()) {
            // param is a parameter to a lambda function
            processMethodArg(param, parent, decl->getAccess());
        }

        addReturnRelation(callOperator, parent, decl->getAccess());
        return;
    }
    writeFieldRelations(decl, qualType, parent, fieldPtr);
}

bool LogicalDepVisitor::VisitFieldDecl(clang::FieldDecl *fieldDecl)
{
    if (d_visitLog_p->alreadyVisited(fieldDecl, clang::Decl::Kind::Field)) {
        return true;
    }

    if (!fieldDecl->hasExternalFormalLinkage()) {
        // internal linkage
        return true;
    }

    clang::RecordDecl *record = fieldDecl->getParent();
    const auto *parentDecl = llvm::dyn_cast<clang::CXXRecordDecl>(record);
    if (!parentDecl) {
        return true;
    }
    if (classIsTemplateSpecialization(parentDecl)) {
        return true;
    }

    lvtmdb::TypeObject *parent = lookupParentRecord(parentDecl, "field parent");
    if (!parent) {
        return true;
    }

    // static fields are clang::VarDecls not clang::FieldDecls so this can never
    // be static
    addField(parent, fieldDecl, false);
    processChildStatements(fieldDecl, fieldDecl->getInClassInitializer(), parent);

    return true; // RETURN
}

bool LogicalDepVisitor::funcIsTemplateSpecialization(const clang::FunctionDecl *decl)
// clang gives us a callback when a template function is defined and
// again for each (explicit or implicit) specialization with concrete
// types.
//
// We've already got all the information we need from parsing the template
// definition. We don't want to parse with concrete types otherwise we
// will create relationships for the types used in the specialization.
{
    assert(decl);

    using TK = clang::FunctionDecl::TemplatedKind;
    const TK kind = decl->getTemplatedKind();

    return kind == TK::TK_MemberSpecialization || kind == TK::TK_FunctionTemplateSpecialization
        || kind == TK::TK_DependentFunctionTemplateSpecialization;
}

bool LogicalDepVisitor::classIsTemplateSpecialization(const clang::CXXRecordDecl *decl)
{
    assert(decl);
    using TSK = clang::TemplateSpecializationKind;
    return decl->getTemplateSpecializationKind() != TSK::TSK_Undeclared;
}

bool LogicalDepVisitor::methodIsTemplateSpecialization(const clang::CXXMethodDecl *decl)
{
    assert(decl);
    const clang::CXXRecordDecl *parent = decl->getParent();

    return funcIsTemplateSpecialization(decl) || classIsTemplateSpecialization(parent);
}

void LogicalDepVisitor::visitLocalVarDeclOrParam(clang::VarDecl *varDecl)
{
    assert(varDecl);

    // what are we declared inside of?
    const clang::DeclContext *declContext = varDecl->getParentFunctionOrMethod();
    if (!declContext) {
        return;
    }

    const clang::CXXMethodDecl *methodDecl = nullptr;
    const clang::FunctionDecl *funcDecl = nullptr;

    if ((methodDecl = clang::dyn_cast<clang::CXXMethodDecl>(declContext))) {
        if (methodIsTemplateSpecialization(methodDecl)) {
            return;
        }
    } else {
        funcDecl = clang::dyn_cast<clang::FunctionDecl>(declContext);
    }

    // What matters is if the *method* is a template specialization, not the var.
    // Conceptually this should be seeded by methodDecl->getTemplatedKind()
    // but as we just return early after a trivial comparison, it is quicker
    // to do that first.
    if (d_visitLog_p->alreadyVisited(varDecl, clang::Decl::Kind::Var)) {
        return;
    }

    // what is our type?
    const clang::QualType qualType = varDecl->getType();
    lvtmdb::TypeObject *type = lookupType(varDecl, qualType, "varDecl type", false);

    if (methodDecl) {
        // look up parent class then add relation for that
        const clang::CXXRecordDecl *containingRecord = methodDecl->getParent();
        assert(containingRecord);

        lvtmdb::TypeObject *containerDecl = lookupParentRecord(containingRecord, "containing class for varDecl");
        if (!containerDecl) {
            return;
        }

        // special case: method parameters:
        if (!varDecl->isLocalVarDecl()) {
            // we only call this method if isLocalVarDeclOrParam therefore varDecl
            // must be a parameter

            processMethodArg(varDecl, containerDecl);
            return;
        }

        // Add "container usesInTheImplementation type" if it isn't already recorded
        if (type) {
            // AS_PRIVATE = always usesInImpl
            addRelation(containerDecl, qualType, varDecl, clang::AccessSpecifier::AS_private, "varDecl");
        }
    } else if (funcDecl) {
        // add pseudo function relationship
        if (type) {
            d_staticFnHandler_p->addFnUses(funcDecl, type);
        }
    }
}

bool LogicalDepVisitor::VisitVarDecl(clang::VarDecl *varDecl)
{
    if (varDecl->isLocalVarDeclOrParm()) {
        visitLocalVarDeclOrParam(varDecl);
        return true; // RETURN
    }

    if (d_visitLog_p->alreadyVisited(varDecl, clang::Decl::Kind::Var, varDecl->getType()->isTemplateTypeParmType())) {
        return true;
    }

    if (varDecl->isStaticDataMember()) {
        const clang::DeclContext *context = varDecl->getDeclContext();
        // this cast can't fail because we already checked that we are a data
        // member
        const auto *record = clang::cast<clang::CXXRecordDecl>(context);
        if (classIsTemplateSpecialization(record)) {
            return true;
        }

        lvtmdb::TypeObject *parent = lookupParentRecord(record, "static data member parent");
        if (!parent) {
            return true;
        }

        addField(parent, varDecl, true);
        processChildStatements(varDecl, varDecl->getInit(), parent);

        return true;
    }

    std::string name = varDecl->getNameAsString();
    std::string qualifiedName = varDecl->getQualifiedNameAsString();
    std::string parentName = qualifiedName.substr(0, qualifiedName.length() - name.length() - 2);

    if (qualifiedName.empty()) {
        return true; // RETURN
    }

    lvtmdb::NamespaceObject *parentNamespace = nullptr;

    if (!parentName.empty() && parentName != name) {
        d_memDb.withROLock([&] {
            parentNamespace = d_memDb.getNamespace(parentName);
        });
        if (!parentNamespace) {
            return true; // RETURN
        }
    }

    lvtmdb::VariableObject *var = nullptr;
    d_memDb.withROLock([&] {
        var = d_memDb.getVariable(qualifiedName);
    });
    if (var) {
        return true; // RETURN
    }

    clang::QualType qualType = varDecl->getType();

    std::string typeString = qualType.getAsString();

    d_memDb.withRWLock([&] {
        var = d_memDb.getOrAddVariable(qualifiedName,
                                       std::move(name),
                                       std::move(typeString),
                                       !parentNamespace,
                                       parentNamespace);
    });
    if (parentNamespace) {
        parentNamespace->withRWLock([&] {
            parentNamespace->addVariable(var);
        });
    }
    return true;
}

const clang::CXXRecordDecl *LogicalDepVisitor::qualTypeToRecordDecl(const clang::QualType qualType)
{
    const clang::CXXRecordDecl *declPtr = nullptr;
    const clang::Type *typePtr = qualType.getTypePtr();

    if (typePtr->isPointerType() || typePtr->isReferenceType()) {
        declPtr = typePtr->getPointeeCXXRecordDecl();
    } else {
        declPtr = typePtr->getAsCXXRecordDecl();
    }

    return declPtr;
}

void LogicalDepVisitor::processMethodArg(const clang::VarDecl *varDecl,
                                         lvtmdb::TypeObject *containerDecl,
                                         clang::AccessSpecifier access)
{
    const clang::DeclContext *varContext = varDecl->getDeclContext();
    assert(varContext);

    if (varContext->getDeclKind() == clang::Decl::Kind::CXXMethod) {
        const auto *methodDecl = clang::cast<clang::CXXMethodDecl>(varContext);
        assert(methodDecl);

        const clang::CXXRecordDecl *methodParent = methodDecl->getParent();
        assert(methodParent);

        // figure out what our access specifier should be (if one wasn't already
        // given to us)
        if (access == clang::AS_none) {
            if (methodParent->isLambda()) {
                const clang::Decl::Kind parentContextKind = methodParent->getDeclContext()->getDeclKind();

                if (parentContextKind == clang::Decl::Kind::CXXMethod) {
                    // This lambda is used inside a method so it should so it
                    // should always be usesInTheImpl
                    access = clang::AS_private;
                } else {
                    // This is a lambda as a class field so
                    // parentContextKind == clang::Decl::Kind::CXXRecord
                    // We get called for this once from addField (which sets the
                    // correct access specifier) and once by VisitVarDecl for the
                    // argument (from which we cannot figure out the access
                    // specifier). If we got here then this is the second call,
                    // which can be safely ignored because the argument is
                    // already added
                    return;
                }
            } else {
                // method parent is not a lambda so this is a normal method
                // argument and should use the access specifier of the method
                access = methodDecl->getAccess();
            }
        }
        if (access == clang::AS_none) {
            // we won't add anything later anyway so fail early without troubling
            // the database
            return;
        }

        // normal (non-lambda, not local class, etc) will have been already
        // added to the database, in which case we need to link the argument to
        // the method
        lvtmdb::MethodObject *methodDeclarationPtr = nullptr;
        if (!skipRecord(methodParent)) {
            d_memDb.withROLock([&] {
                methodDeclarationPtr = d_memDb.getMethod(methodDecl->getQualifiedNameAsString(),
                                                         getSignature(methodDecl),
                                                         getTemplateParameters(methodDecl),
                                                         getReturnType(methodDecl));
            });
            if (!methodDeclarationPtr) {
                const std::string message = "WARN: failed to look up method " + methodDecl->getQualifiedNameAsString()
                    + " to add method argument\n";

                if (d_messageCallback) {
                    auto& fnc = *d_messageCallback;
                    fnc(message, ClpUtil::getThreadId());
                }

                d_memDb.withRWLock([&] {
                    d_memDb.getOrAddError(lvtmdb::MdbUtil::ErrorKind::ParserError,
                                          methodDecl->getQualifiedNameAsString(),
                                          "failed to look up method",
                                          methodDecl->getLocation().printToString(Context->getSourceManager()));
                });
            }
        }

        // if it is a template we also need to add all of the template parameters
        // to the database
        const clang::TemplateSpecializationType *templateTypePtr =
            varDecl->getType().getNonReferenceType()->getAs<clang::TemplateSpecializationType>();
        if (templateTypePtr) {
            parseArgumentTemplate(varDecl, templateTypePtr, access, containerDecl, methodDeclarationPtr);
        }

        // finally write the method-argument relation to the database
        const clang::QualType qualType = varDecl->getType();
        writeMethodArgRelation(varDecl, qualType, access, containerDecl, methodDeclarationPtr);

        // Process any expressions for initializing default arguments:
        if (varDecl->getKind() == clang::Decl::Kind::ParmVar) {
            const auto *paramVar = clang::cast<clang::ParmVarDecl>(varDecl);
            assert(paramVar);

            processChildStatements(varDecl, paramVar->getDefaultArg(), containerDecl);
        }
    }
}

void LogicalDepVisitor::writeMethodArgRelation(const clang::Decl *decl,
                                               const clang::QualType type,
                                               const clang::AccessSpecifier access,
                                               lvtmdb::TypeObject *parentClassPtr,
                                               lvtmdb::MethodObject *methodDeclarationPtr)
{
    lvtmdb::TypeObject *argClassPtr = lookupType(decl, type, __func__, false);
    if (argClassPtr && parentClassPtr && argClassPtr != parentClassPtr) {
        addRelation(parentClassPtr, type, decl, access, "method arg");
        if (methodDeclarationPtr) {
            methodDeclarationPtr->withRWLock([&] {
                methodDeclarationPtr->addArgumentType(argClassPtr);
            });
        }
    }
}

void LogicalDepVisitor::parseArgumentTemplate(const clang::Decl *decl,
                                              const clang::TemplateSpecializationType *templateTypePtr,
                                              const clang::AccessSpecifier access,
                                              lvtmdb::TypeObject *parentClassPtr,
                                              lvtmdb::MethodObject *methodDeclarationPtr)
{
    for (auto arg : templateTypePtr->template_arguments()) {
        if (arg.isDependent() || arg.getKind() == clang::TemplateArgument::Expression
            || arg.getKind() == clang::TemplateArgument::Template) {
            continue;
        }

        const clang::QualType qualType = arg.getAsType();
        writeMethodArgRelation(decl, qualType, access, parentClassPtr, methodDeclarationPtr);

        const clang::TemplateSpecializationType *ptr =
            qualType.getNonReferenceType()->getAs<clang::TemplateSpecializationType>();

        if (ptr != nullptr) {
            parseArgumentTemplate(decl, ptr, access, parentClassPtr, methodDeclarationPtr);
        }
    }
}

void LogicalDepVisitor::writeFieldRelations(const clang::Decl *decl,
                                            const clang::QualType fieldClass,
                                            lvtmdb::TypeObject *parentClassPtr,
                                            lvtmdb::FieldObject *fieldDeclarationPtr)
{
    lvtmdb::TypeObject *fieldClassPtr = lookupType(decl, fieldClass, __func__, false);
    if (fieldClassPtr && parentClassPtr && fieldClassPtr != parentClassPtr) {
        clang::AccessSpecifier access = clang::AS_none;
        fieldDeclarationPtr->withROLock([&] {
            access = static_cast<clang::AccessSpecifier>(fieldDeclarationPtr->access());
        });
        addRelation(parentClassPtr, fieldClass, decl, access, "field");

        fieldDeclarationPtr->withRWLock([&] {
            fieldDeclarationPtr->addType(fieldClassPtr);
        });
    }
}

void LogicalDepVisitor::parseFieldTemplate(const clang::Decl *decl,
                                           const clang::TemplateSpecializationType *templateTypePtr,
                                           lvtmdb::TypeObject *parentClassPtr,
                                           lvtmdb::FieldObject *fieldDeclarationPtr)
{
    for (auto arg : templateTypePtr->template_arguments()) {
        if (arg.isDependent() || arg.getKind() == clang::TemplateArgument::Expression
            || arg.getKind() == clang::TemplateArgument::Template) {
            continue;
        }

        clang::QualType qualType = arg.getAsType();
        writeFieldRelations(decl, qualType, parentClassPtr, fieldDeclarationPtr);

        const clang::TemplateSpecializationType *ptr =
            qualType.getNonReferenceType()->getAs<clang::TemplateSpecializationType>();

        if (ptr != nullptr) {
            parseFieldTemplate(decl, ptr, parentClassPtr, fieldDeclarationPtr);
        }
    }
}

std::string LogicalDepVisitor::getSignature(const clang::FunctionDecl *functionDecl)
{
    std::vector<std::string> arguments;
    std::string name(functionDecl->getNameAsString());

    for (auto *parameter : functionDecl->parameters()) {
        std::string argument(parameter->getType().getAsString());
        argument += " ";
        argument += parameter->getNameAsString();
        if (parameter->hasDefaultArg()) {
            clang::SourceRange range = parameter->getDefaultArgRange();
            clang::SourceManager& srcMgr = Context->getSourceManager();
            llvm::StringRef s = clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(range),
                                                            srcMgr,
                                                            Context->getLangOpts());
            argument += " = ";
            argument += std::string(s);
        }

        arguments.push_back(argument);
    }

    return name + "(" + boost::algorithm::join(arguments, ", ") + ")";
}

void LogicalDepVisitor::addReturnRelation(const clang::FunctionDecl *func,
                                          lvtmdb::TypeObject *parent,
                                          const clang::AccessSpecifier access)
{
    assert(func);
    assert(parent);

    parseReturnTemplate(func, func->getReturnType(), parent, access);
}

void LogicalDepVisitor::parseReturnTemplate(const clang::Decl *decl,
                                            const clang::QualType returnQType,
                                            lvtmdb::TypeObject *parent,
                                            const clang::AccessSpecifier access)
{
    if (returnQType->getTypeClass() == clang::Type::TypeClass::TemplateTypeParm) {
        return;
    }

    addRelation(parent, returnQType, decl, access, "method return");

    const clang::TemplateSpecializationType *ptr =
        returnQType.getNonReferenceType()->getAs<clang::TemplateSpecializationType>();
    if (ptr) {
        for (const auto arg : ptr->template_arguments()) {
            if (arg.getKind() == clang::TemplateArgument::ArgKind::Type) {
                parseReturnTemplate(decl, arg.getAsType(), parent, access);
            }
        }
    }
}

bool LogicalDepVisitor::VisitCXXMethodDecl(clang::CXXMethodDecl *methodDecl)
{
    assert(methodDecl);
    if (d_visitLog_p->alreadyVisited(methodDecl, clang::Decl::Kind::CXXMethod, methodDecl->getTemplatedKind())) {
        return true;
    }

    if (!methodDecl->hasExternalFormalLinkage()) {
        // internal linkage
        return true;
    }

    if (methodIsTemplateSpecialization(methodDecl)) {
        return true;
    }

    lvtmdb::TypeObject *parent = lookupParentRecord(methodDecl->getParent(), "method parent");
    if (!parent) {
        return true;
    }

    processChildStatements(methodDecl, methodDecl->getBody(), parent);
    addReturnRelation(methodDecl, parent, methodDecl->getAccess());

    return true;
}

void LogicalDepVisitor::processChildStatements(const clang::Decl *decl,
                                               const clang::Stmt *top,
                                               lvtmdb::TypeObject *parent)
// Wrap around listChildStatementRelations to add the relations
{
    if (!parent) {
        return;
    }

    std::vector<lvtmdb::TypeObject *> usesInImpls = listChildStatementRelations(decl, top, parent);
    for (lvtmdb::TypeObject *dep : usesInImpls) {
        ClpUtil::addUsesInImpl(parent, dep);
    }
}

std::vector<lvtmdb::TypeObject *> LogicalDepVisitor::listChildStatementRelations(const clang::Decl *decl,
                                                                                 const clang::Stmt *top,
                                                                                 lvtmdb::TypeObject *parent)
{
    std::vector<lvtmdb::TypeObject *> ret;
    if (!top) {
        return ret;
    }

    // what we actually want to do is VisitCXXTemporaryObjectExpr, but we need
    // to know which parentDecl the temporary object is inside, and I can't find
    // any neat way to do that. Instead, this method starts at top and
    // walks the AST looking for temporary objects.
    auto visitTemporary = [this, &parent, decl, &ret](const clang::Expr *temporary) {
        clang::QualType type = temporary->getType();

        lvtmdb::TypeObject *tempType = lookupType(decl, type, "temporary object type", false);
        if (!tempType) {
            return;
        }

        // Add "parent usesInTheImplementation tempType" if it isn't already recorded
        if (d_catchCodeAnalysisOutput) {
            debugRelationship(parent, tempType, decl, true, "temporary");
        }
        ret.push_back(tempType);

        // add any template arguments
        std::vector<lvtmdb::TypeObject *> args = getTemplateArguments(type, decl, "temporary template arg");
        for (lvtmdb::TypeObject *arg : args) {
            if (d_catchCodeAnalysisOutput) {
                debugRelationship(parent, arg, decl, true, "temporary template arg");
            }
            ret.push_back(arg);
        }
    };
    searchClangStmtAST<clang::CXXTemporaryObjectExpr>(top, visitTemporary);
    // as far as we are concerned these are the same as a temporary variable
    // e.g.
    // template <typename T> void method() { return C<T>; }
    searchClangStmtAST<clang::CXXUnresolvedConstructExpr>(top, visitTemporary);

    // similarly to the above, we want to visit clang::DeclRefExpr to look for
    // references to static class members (we catch non-static things on variable
    // declaration)
    auto visitDeclRefExpr = [this, &parent, decl, &ret](const clang::DeclRefExpr *declRefExpr) {
        const clang::ValueDecl *valueDecl = declRefExpr->getDecl();

        // static method call
        const auto *method = clang::dyn_cast<clang::CXXMethodDecl>(valueDecl);
        if (method) {
            const clang::CXXRecordDecl *targetDecl = method->getParent();
            assert(targetDecl);
            lvtmdb::TypeObject *target = lookupUDT(targetDecl, "static method call");
            if (!target) {
                // sometimes we miss things
                return;
            }

            if (d_catchCodeAnalysisOutput) {
                debugRelationship(parent, target, decl, true, "static method call");
            }
            ret.push_back(target);
            return;
        }

        // static member variable reference
        const auto *varDecl = clang::dyn_cast<clang::VarDecl>(valueDecl);
        if (varDecl) {
            const clang::DeclContext *varContext = varDecl->getDeclContext();
            if (varContext->getDeclKind() != clang::Decl::Kind::CXXRecord) {
                // we are referencing a variable which isn't a class member:
                // ignore
                return;
            }

            const auto *targetDecl = clang::cast<clang::CXXRecordDecl>(varContext);
            assert(targetDecl);
            lvtmdb::TypeObject *target = lookupUDT(targetDecl, "static field reference");
            if (!target) {
                return;
            }

            if (d_catchCodeAnalysisOutput) {
                debugRelationship(parent, target, decl, true, "static field reference");
            }
            ret.push_back(target);
            return;
        }
    };
    searchClangStmtAST<clang::DeclRefExpr>(top, visitDeclRefExpr);

    // similarly to the above, we want to visit clang::CallExpr to look for
    // templated function calls so we can record their template parameters
    // and to record calls to static functions
    auto visitCallExpr = [this, &parent, decl, &ret](const clang::CallExpr *declRefExpr) {
        const clang::FunctionDecl *func = declRefExpr->getDirectCallee();
        if (!func) {
            return;
        }

        // record call to the static function
        if (!func->isGlobal() && parent) {
            d_staticFnHandler_p->addUdtUses(parent, func);
        }

        // now handle template parameters
        if (!funcIsTemplateSpecialization(func)) {
            return;
        }

        // we need to record relations for the function template's parameters
        clang::FunctionTemplateSpecializationInfo *info = func->getTemplateSpecializationInfo();
        if (info) {
            const clang::TemplateArgumentList *args = info->TemplateArguments;
            assert(args);
            for (unsigned i = 0; i < args->size(); ++i) {
                const clang::TemplateArgument& arg = args->get(i);
                if (arg.getKind() != clang::TemplateArgument::Type) {
                    continue;
                }

                const clang::QualType qualType = arg.getAsType();

                lvtmdb::TypeObject *target = lookupType(decl, qualType, "function call template parameter", false);
                if (!target) {
                    continue;
                }

                if (d_catchCodeAnalysisOutput) {
                    debugRelationship(parent, target, decl, true, "template func parameter");
                }
                ret.push_back(target);
            }
        }
    };
    searchClangStmtAST<clang::CallExpr>(top, visitCallExpr);

    // Find calls to dependent scope member expr
    // e.g.
    // template <typename T>
    // ...
    // return D<T>::foo();
    // This is similar to clang::CXXUnresolvedConstructExpr but needs more wrapping
    auto visitDepScopeMember = [this, &parent, decl, &ret](const clang::CXXDependentScopeMemberExpr *expr) {
        std::string ss;
        llvm::raw_string_ostream s(ss);
        expr->printPretty(s, nullptr, clang::PrintingPolicy(Context->getLangOpts()));
        const std::string typeString = s.str();

        // we are expecting something looking like Type<TEMPLATE_PARAM>::method
        // clang can't extract Type from this (just saying <dependent type>)
        // but we can pull the string out and look for that in the database
        const std::size_t loc_template = typeString.find('<');
        if (loc_template == std::string::npos) {
            return;
        }
        const std::string className = typeString.substr(0, loc_template);

        lvtmdb::TypeObject *dest = nullptr;
        d_memDb.withROLock([&] {
            // we only have a string className, not any sort of clang::Decl
            // so we can't use lookup*
            dest = d_memDb.getType(className);
        });
        if (!dest) {
            // this is already an edge case of an edge case. Let's not stress
            // about a lookup failure here
            return;
        }

        if (d_catchCodeAnalysisOutput) {
            debugRelationship(parent, dest, decl, true, "CXXUnresolvedConstructExpr");
        }
        ret.push_back(dest);
    };
    searchClangStmtAST<clang::CXXDependentScopeMemberExpr>(top, visitDepScopeMember);

    return ret;
}

bool LogicalDepVisitor::VisitUsingDecl(clang::UsingDecl *usingDecl)
{
    assert(usingDecl);
    // getKind() is okay here because we don't have callbacks at different levels
    // of the hierarchy
    if (d_visitLog_p->alreadyVisited(usingDecl, usingDecl->getKind())) {
        return true;
    }

    // We don't want to create a type alias for every name created by a using
    // declaration
    // Skip anything at file context (not in a namespace or a class)
    // TODO: maybe we should be more strict here?
    const clang::DeclContext *context = usingDecl->getDeclContext();
    assert(context);
    if (context->isTranslationUnit()) {
        return true;
    }

    for (const clang::UsingShadowDecl *shadowDecl : usingDecl->shadows()) {
        // get referenced type
        const clang::NamedDecl *referenced = shadowDecl->getTargetDecl();
        assert(referenced);
        const clang::Type *type = nullptr;

        const auto *typeDecl = clang::dyn_cast<clang::TypeDecl>(referenced);
        if (typeDecl) {
            type = typeDecl->getTypeForDecl();
        } else {
            // why isn't this a TypeDecl grr
            const auto *ctd = clang::dyn_cast<clang::ClassTemplateDecl>(referenced);
            if (ctd) {
                const clang::CXXRecordDecl *record = ctd->getTemplatedDecl();
                type = record->getTypeForDecl();
            }
        }

        if (type) {
            visitTypeAlias(usingDecl, type->getCanonicalTypeInternal());
        }
    }

    return true;
}

bool LogicalDepVisitor::VisitTypedefNameDecl(clang::TypedefNameDecl *nameDecl)
{
    assert(nameDecl);
    // getKind() is okay here because we don't have callbacks at different levels of the hierarchy
    if (d_visitLog_p->alreadyVisited(nameDecl, nameDecl->getKind())) {
        return true;
    }

    // get referenced type
    const clang::QualType type = nameDecl->getUnderlyingType();

    visitTypeAlias(nameDecl, type);

    return true;
}

lvtmdb::TypeObject *LogicalDepVisitor::addUDT(const clang::NamedDecl *nameDecl, const lvtshr::UDTKind kind)
{
    auto isInAnonNamespace = [](const clang::NamedDecl *nameDecl) -> bool {
        const clang::DeclContext *context = nameDecl->getDeclContext();
        while (context) {
            if (context->isNamespace()) {
                const auto *nmspc = clang::cast<clang::NamespaceDecl>(context);
                if (nmspc && nmspc->isAnonymousNamespace()) {
                    return true;
                }
            }
            context = context->getParent();
        }
        return false;
    };

    // don't add UDTs in the anon namespace because they are not
    // significant and are not required to have a globally unique name
    if (isInAnonNamespace(nameDecl)) {
        return {};
    }

    lvtmdb::TypeObject *dbUDT = lookupUDT(nameDecl, nullptr);
    if (dbUDT) {
        addUDTSourceFile(dbUDT, nameDecl);
        return dbUDT;
    }

    // check if any parent contexts are template specializations, if so skip
    const clang::DeclContext *contextIter = nameDecl->getDeclContext();
    while (contextIter) {
        if (contextIter->isNamespace()) {
            // we have run out of parent classes
            break;
        }
        if (contextIter->getDeclKind() == clang::Decl::Kind::CXXRecord) {
            const auto *parentRecord = clang::cast<clang::CXXRecordDecl>(contextIter);
            assert(parentRecord);
            if (classIsTemplateSpecialization(parentRecord)) {
                return nullptr;
            }
        }
        if (contextIter->getDeclKind() == clang::Decl::Kind::ClassTemplateSpecialization) {
            return nullptr;
        }
        contextIter = contextIter->getParent();
    }

    // get parent context
    lvtmdb::TypeObject *parentClass = nullptr;
    lvtmdb::NamespaceObject *parentNamespace = nullptr;
    const clang::DeclContext *declContext = nameDecl->getDeclContext();
    const auto contextKind = declContext->getDeclKind();
    if (contextKind == clang::Decl::Kind::CXXRecord) {
        // lexically scoped within a class
        const auto *parentRecord = clang::cast<clang::CXXRecordDecl>(declContext);

        parentClass = lookupParentRecord(parentRecord, "TypeDefDecl parent");
        if (parentClass) {
            parentClass->withROLock([&] {
                parentNamespace = parentClass->parentNamespace();
            });
        }
    } else if (contextKind == clang::Decl::Kind::Namespace) {
        // lexically scoped within a namespace
        const auto *nmspc = clang::cast<clang::NamespaceDecl>(declContext);
        assert(nmspc);
        d_memDb.withROLock([&] {
            parentNamespace = d_memDb.getNamespace(nmspc->getQualifiedNameAsString());
        });
    }

    d_memDb.withRWLock([&] {
        dbUDT = d_memDb.getOrAddType(nameDecl->getQualifiedNameAsString(),
                                     nameDecl->getNameAsString(),
                                     kind,
                                     static_cast<lvtshr::AccessSpecifier>(nameDecl->getAccess()),
                                     parentNamespace,
                                     nullptr, // parent package, see addUDTSourceFile
                                     parentClass);
    });

    if (parentNamespace) {
        parentNamespace->withRWLock([&] {
            parentNamespace->addType(dbUDT);
        });
    }
    if (parentClass) {
        parentClass->withRWLock([&] {
            parentClass->addChild(dbUDT);
        });
    }

    addUDTSourceFile(dbUDT, nameDecl);

    return dbUDT;
}

void LogicalDepVisitor::addUDTSourceFile(lvtmdb::TypeObject *udt, const clang::Decl *decl)
{
    assert(udt);
    const std::string sourceFile = ClpUtil::getRealPath(decl->getLocation(), Context->getSourceManager());

    lvtmdb::FileObject *filePtr =
        ClpUtil::writeSourceFile(sourceFile, true, d_memDb, d_prefix, d_nonLakosianDirs, d_thirdPartyDirs);
    if (!filePtr) {
        return;
    }
    lvtmdb::ComponentObject *component = nullptr;
    lvtmdb::PackageObject *package = nullptr;
    filePtr->withROLock([&] {
        component = filePtr->component();
        package = filePtr->package();
    });

    udt->withRWLock([&] {
        udt->addFile(filePtr);
        if (component) {
            udt->addComponent(component);
        }
        if (package) {
            udt->setPackage(package);
        }
    });

    filePtr->withRWLock([&] {
        filePtr->addType(udt);
    });

    if (component) {
        component->withRWLock([&] {
            component->addType(udt);
        });
    }

    if (package) {
        package->withRWLock([&] {
            package->addType(udt);
        });
    }
}

void LogicalDepVisitor::visitTypeAlias(const clang::NamedDecl *nameDecl, const clang::QualType referencedType)
{
    // check if the type alias is publically accessible so that we don't
    // end up adding a local type alias to the global namespace
    const clang::DeclContext *context = nameDecl->getDeclContext();
    assert(context);
    if (context->isFunctionOrMethod()) {
        // we still need a uses-in-impl if we are inside a method
        if (const auto *const method = clang::dyn_cast<clang::CXXMethodDecl>(context)) {
            const clang::CXXRecordDecl *parentClass = method->getParent();
            assert(parentClass);

            lvtmdb::TypeObject *parent = lookupParentRecord(parentClass, "type alias in method parent class");
            if (parent) {
                // AS_PRIVATE = always usesInImpl
                addRelation(parent, referencedType, nameDecl, clang::AccessSpecifier::AS_private, "local type alias");
            }
        }
        return;
    }

    auto *dbUDT = addUDT(nameDecl, lvtshr::UDTKind::TypeAlias);
    if (!dbUDT) {
        return;
    }
    // TODO: the aliased type will never have methods, fields etc
    //       this is technically correct but could be surprising when the fact
    //       that this is a typedef is an implementation detail

    // AS_private = always usesInImpl
    addRelation(dbUDT, referencedType, nameDecl, clang::AccessSpecifier::AS_private, "type alias 1");

    lvtmdb::TypeObject *parentClass = nullptr;
    dbUDT->withROLock([&] {
        parentClass = dbUDT->parent();
    });
    if (parentClass) {
        addRelation(parentClass, dbUDT, nameDecl, nameDecl->getAccess(), "type alias 2");
    }
}

bool LogicalDepVisitor::VisitEnumDecl(clang::EnumDecl *enumDecl)
{
    assert(enumDecl);
    if (d_visitLog_p->alreadyVisited(enumDecl, enumDecl->getKind())) {
        return true;
    }

    // skip anon enum
    if (enumDecl->getNameAsString().empty()) {
        return true;
    }

    // skip internal linkage
    if (!enumDecl->hasExternalFormalLinkage()) {
        return true;
    }

    addUDT(enumDecl, lvtshr::UDTKind::Enum);

    return true;
}

std::vector<lvtmdb::TypeObject *>
LogicalDepVisitor::getTemplateArguments(const clang::QualType type, const clang::Decl *decl, const char *desc)
{
    // find any template arguments of the type
    const auto *const tmplSpec = clang::dyn_cast<clang::TemplateSpecializationType>(type);
    if (!tmplSpec) {
        return {};
    }

    std::vector<lvtmdb::TypeObject *> templateArgs;
#if CLANG_VERSION_MAJOR >= 16
    const clang::ArrayRef<clang::TemplateArgument> args = tmplSpec->template_arguments();

    for (unsigned i = 0; i < args.size(); ++i) {
        if (args[i].getKind() != clang::TemplateArgument::ArgKind::Type) {
            continue;
        }
        clang::QualType argQType = args[i].getAsType();

        lvtmdb::TypeObject *argType = lookupType(decl, argQType, desc, false);
        if (argType) {
            templateArgs.push_back(argType);
        }
    }
#else
    const clang::TemplateArgument *args = tmplSpec->getArgs();

    for (unsigned i = 0; i < tmplSpec->getNumArgs(); ++i) {
        if (args[i].getKind() != clang::TemplateArgument::ArgKind::Type) {
            continue;
        }
        clang::QualType argQType = args[i].getAsType();

        lvtmdb::TypeObject *argType = lookupType(decl, argQType, desc, false);
        if (argType) {
            templateArgs.push_back(argType);
        }
    }
#endif
    return templateArgs;
}

void LogicalDepVisitor::addRelation(lvtmdb::TypeObject *source,
                                    lvtmdb::TypeObject *target,
                                    const clang::Decl *decl,
                                    const clang::AccessSpecifier access,
                                    const char *desc)
{
    if (access == clang::AS_public || access == clang::AS_protected) {
        if (d_catchCodeAnalysisOutput) {
            debugRelationship(source, target, decl, false, desc);
        }
        ClpUtil::addUsesInInter(source, target);
    } else if (access == clang::AS_private) {
        if (d_catchCodeAnalysisOutput) {
            debugRelationship(source, target, decl, true, desc);
        }
        ClpUtil::addUsesInImpl(source, target);
    } else {
        std::string message = "WARN: unknown access specifier (clang::AccessSpecifier)" + std::to_string(access);
        if (desc) {
            message += " for " + std::string(desc);
        }
        if (decl) {
            message += " at " + decl->getLocation().printToString(Context->getSourceManager());
        }
        message += "\n";

        if (d_messageCallback) {
            auto& fnc = *d_messageCallback;
            fnc(message, ClpUtil::getThreadId());
        }
    }
}

void LogicalDepVisitor::addRelation(lvtmdb::TypeObject *source,
                                    const clang::QualType targetType,
                                    const clang::Decl *decl,
                                    const clang::AccessSpecifier access,
                                    const char *desc)
{
    bool usesInImpl;
    if (access == clang::AS_public || access == clang::AS_protected) {
        usesInImpl = false;
    } else if (access == clang::AS_private) {
        usesInImpl = true;
    } else {
        return;
    }

    lvtmdb::TypeObject *target = lookupType(decl, targetType, desc, false);
    if (target) {
        addRelation(source, target, decl, access, desc);
    }

    // handle if target has template arguments
    std::vector<lvtmdb::TypeObject *> args = getTemplateArguments(targetType, decl, desc);
    for (lvtmdb::TypeObject *arg : args) {
        if (d_catchCodeAnalysisOutput) {
            debugRelationship(source, arg, decl, usesInImpl, desc);
        }
        if (usesInImpl) {
            ClpUtil::addUsesInImpl(source, arg);
        } else {
            ClpUtil::addUsesInInter(source, arg);
        }
    }
}

void LogicalDepVisitor::debugRelationship(
    lvtmdb::TypeObject *source, lvtmdb::TypeObject *target, const clang::Decl *decl, bool impl, const char *desc)
{
    // TODO: Add QLoggingCategory enabled test for early return
    if (!source || !target) {
        return;
    }

    source->withROLock([&] {
        qDebug() << QString::fromStdString(source->qualifiedName());
    });
    if (impl) {
        qDebug() << " (uses in the implementation) ";
    } else {
        qDebug() << " (uses in the interface) ";
    }
    target->withROLock([&] {
        qDebug() << QString::fromStdString(target->qualifiedName());
    });

    qDebug() << " (" << desc << ") ";
    if (decl) {
        auto contextStr =
            QString::fromStdString(decl->getLocation().printToString(decl->getASTContext().getSourceManager()));

        qDebug() << contextStr;
    }
    qDebug() << "\n";
}

} // end namespace Codethink::lvtclp
