// ct_lvtshr_loaderinfo.cpp                                           -*-C++-*-

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

#include <ct_lvtshr_loaderinfo.h>

namespace Codethink::lvtshr {

LoaderInfo::LoaderInfo() = default;

LoaderInfo::LoaderInfo(bool hasAllChildren, bool hasParent, bool hasAllEdges):
    d_valid(true), d_hasAllChildren(hasAllChildren), d_hasParent(hasParent), d_hasAllEdges(hasAllEdges)
{
}

bool LoaderInfo::isValid() const
{
    return d_valid;
}

std::optional<bool> LoaderInfo::hasAllChildren() const
{
    if (!d_valid) {
        return {};
    }
    return d_hasAllChildren;
}

std::optional<bool> LoaderInfo::hasParent() const
{
    if (!d_valid) {
        return {};
    }
    return d_hasParent;
}

std::optional<bool> LoaderInfo::hasAllEdges() const
{
    if (!d_valid) {
        return {};
    }
    return d_hasAllEdges;
}

void LoaderInfo::setHasParent(bool hasParent)
{
    d_hasParent = hasParent;
}

} // end namespace Codethink::lvtshr
