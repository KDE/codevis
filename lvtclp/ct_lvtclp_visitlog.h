// ct_lvtclp_visitlog.h                                                -*-C++-*-

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

#ifndef INCLUDED_CT_LVTCLP_VISITLOG
#define INCLUDED_CT_LVTCLP_VISITLOG

//@PURPOSE: Remember which `clang::Decl`s we have already visited
//
//@CLASSES:
//  lvtclp::VisitLog: Not for use outside lvtclp
//
//@SEE_ALSO: lvtclp::CodebaseDbVisitor
//
//@DESCRIPTION:
//  CodebaseDbVistor consists of callbacks for different clang::Decl kinds.
//  Each callback is for a particular clang::SourceLocation. We can skip any
//  decls we have already seen (e.g. in a header file). The tuple
//  <Decl::Kind, SourceLocation, templateKind> uniquely identifies the callback.
//  We need the Decl::Kind because we can have multiple callbacks for the same
//  source location e.g. VisitFunctionDecl and VisitMethodDecl.
//  We need the templateKind because clang will give us callbacks for each
//  specialization.

#include <memory>

#include <clang/AST/DeclBase.h>

namespace Codethink::lvtclp {

// ==============================
// class VisitLog
// ==============================

class VisitLog {
    // Store which decl callbacks we have already processed

    // PRIVATE TYPES
    struct Private;

    // DATA
    std::unique_ptr<Private> d;

  public:
    // CREATORS
    VisitLog();

    ~VisitLog() noexcept;

    // MANIPULATORS
    bool alreadyVisited(const clang::Decl *decl, clang::Decl::Kind kind, int templateKind = -1);
    // Returns if we have already visited this decl.
    // If we haven't already visited this decl, it is now considered visited
};

} // end namespace Codethink::lvtclp

#endif
