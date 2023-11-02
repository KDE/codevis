// ct_lvtclp_staticfnhandler.h                                        -*-C++-*-

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

#ifndef INCLUDED_CT_LVTCLP_STATICFNHANDLER
#define INCLUDED_CT_LVTCLP_STATICFNHANDLER

//@PURPOSE: Temporary storage of which classes use each static function and
//          what relationships that implies. This should be re-created for each
//          file
//
//@CLASSES:
//  lvtclp::StaticFnHandler Not for use outside lvtclp
//
//@SEE_ALSO: lvtclp::CodebaseDbVisitor
//
//@DESCRIPTION:
//  It is an implementation detail whether a class method references something
//  directly or via a static free function. Methods from multiple classes might
//  use the same static free function, in which case all of those classes should
//  get the uses-in-the-implementation relationships implied by the free
//  function. It would not be appropriate to add file scope free functions to
//  the database and so we should cache all of the information here then add
//  the database realtionships at the end of AST processing for a particular
//  translation unit.

#include <clang/AST/Decl.h>

#include <memory>

namespace Codethink::lvtmdb {
class ObjectStore;
}
namespace Codethink::lvtmdb {
class TypeObject;
}
namespace Codethink::lvtmdb {
class FunctionObject;
}

namespace Codethink::lvtclp {

// ==============================
// class StaticFnHandler
// ==============================

class StaticFnHandler {
    // Each instance should be unique to a given file because we allow static
    // functions

    // PRIVATE TYPES
    struct Private;

    // DATA
    std::unique_ptr<Private> d;

  public:
    // CREATORS
    explicit StaticFnHandler(lvtmdb::ObjectStore& memDb);

    ~StaticFnHandler() noexcept;

    // MANIPULATORS
    void addFnUses(const clang::FunctionDecl *decl, lvtmdb::TypeObject *udt);
    // Function described by decl uses-in-the-impl udt
    void addFnUses(const clang::FunctionDecl *decl, const clang::FunctionDecl *dep);
    // Function described by decl uses-in-the-impl dep
    void addUdtUses(lvtmdb::TypeObject *udt, const clang::FunctionDecl *decl);
    // udt uses-in-the-impl function described by decl
    void addCallgraphDep(lvtmdb::FunctionObject *source_f, lvtmdb::FunctionObject *target_f);

    void writeOutToDb();
    // (Once traversal of a translation unit is done) write out the
    // relationships to the database

    void reset();
    // Clear all saved state (except the database session pointer)

  private:
    // MANIPULATORS
    void flattenFn(std::size_t sourceFn);
};

} // end namespace Codethink::lvtclp

#endif
