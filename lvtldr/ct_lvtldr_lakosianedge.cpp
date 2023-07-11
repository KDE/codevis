// ct_lvtldr_lakosianedge.cpp                              -*-C++-*-

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

#include <ct_lvtldr_lakosianedge.h>
#include <ct_lvtldr_lakosiannode.h>

namespace Codethink::lvtldr {

LakosianEdge::LakosianEdge(lvtshr::LakosRelationType type, LakosianNode *other): d_type(type), d_other(other)
{
    assert(other);
}

LakosianEdge::LakosianEdge(LakosianEdge&&) noexcept = default;
LakosianEdge::LakosianEdge(const LakosianEdge&) = default;

lvtshr::LakosRelationType LakosianEdge::type() const
{
    return d_type;
}

LakosianNode *LakosianEdge::other() const
{
    return d_other;
}

bool operator==(const LakosianEdge& lhs, const LakosianEdge& rhs)
{
    return lhs.type() == rhs.type() && lhs.other() == rhs.other();
}

} // namespace Codethink::lvtldr
