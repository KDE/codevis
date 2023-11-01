// ct_lvtclp_visitlog.cpp                                              -*-C++-*-

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

#include <ct_lvtclp_visitlog.h>

#include <clang/Basic/SourceLocation.h>

#include <unordered_set>

namespace {

struct CallbackId {
    clang::SourceLocation location;
    clang::Decl::Kind declKind;
    int templateKind;
    // We can't store the clang enum here because different Decl kinds have
    // different template enums: e.g. FunctionDecl::TemplatedKind

    explicit CallbackId(const clang::Decl *decl, const clang::Decl::Kind kind, const int templateKind):
        location(decl->getLocation()), declKind(kind), templateKind(templateKind)
    {
    }
};

inline bool operator==(const CallbackId& lhs, const CallbackId& rhs)
{
    return lhs.declKind == rhs.declKind && lhs.templateKind == rhs.templateKind && lhs.location == rhs.location;
}

} // namespace

namespace std {
template<>
struct hash<CallbackId>
// Implementation of std::hash for CallbackId
{
    std::size_t operator()(const CallbackId& id) const noexcept
    {
        const unsigned loc = id.location.getRawEncoding();
        const unsigned kind = id.declKind;

        // Hash will be composed of:
        // [32bit loc] [16bit kind] [16bit templateKind]
        size_t hash = 0;
        hash |= static_cast<size_t>(loc) << 32;
        hash |= (static_cast<size_t>(kind) & 0x000000ff) << 16;
        hash |= static_cast<size_t>(id.templateKind) & 0x000000ff;

        return hash;
    }
};
} // namespace std

namespace Codethink::lvtclp {

struct VisitLog::Private {
    std::unordered_set<CallbackId> visited;
};

VisitLog::VisitLog(): d(std::make_unique<VisitLog::Private>())
{
}

VisitLog::~VisitLog() noexcept = default;

bool VisitLog::alreadyVisited(const clang::Decl *decl, const clang::Decl::Kind kind, const int templateKind)
{
    auto [_, constructed] = d->visited.emplace(decl, kind, templateKind);
    return !constructed;
}

} // end namespace Codethink::lvtclp
