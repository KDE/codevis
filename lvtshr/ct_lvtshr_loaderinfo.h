// ct_lvtgshr_loaderinfo.h                                            -*-C++-*-

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

//@PURPOSE: Store information about what the loader did and did not load for
//          a particular vertex
//
//@CLASSES:
//  lvtshr::LoaderInfo
//
//@SEE_ALSO: lvtldr::PhysicalLoader

#ifndef INCLUDED_CT_LVTSHR_LOADERINFO
#define INCLUDED_CT_LVTSHR_LOADERINFO

#include <lvtshr_export.h>

#include <optional>

namespace Codethink::lvtshr {

class LVTSHR_EXPORT LoaderInfo {
  private:
    // PRIVATE DATA
    bool d_valid : 1 = false;

    bool d_hasAllChildren : 1 = false;
    bool d_hasParent : 1 = false;
    bool d_hasAllEdges : 1 = false;

  public:
    // CREATORS
    LoaderInfo();
    // Default constructor for d_valid = false

    LoaderInfo(bool hasAllChildren, bool hasParent, bool hasAllEdges);
    // Constructor for valid = true

    // ACCESSORS
    [[nodiscard]] bool isValid() const;
    // returns whether we have information for this vertex. If this returns
    // true then it is safe to assume all accessors returning std::optional
    // can be dereferenced, otherwise none of them can be dereferenced

    [[nodiscard]] std::optional<bool> hasAllChildren() const;
    // If isValid(), returns if this vertex already has all of its children

    [[nodiscard]] std::optional<bool> hasParent() const;
    // If isValid(), returns if this vertex already has its direct parent
    // loaded

    [[nodiscard]] std::optional<bool> hasAllEdges() const;
    // If isValid(), returns if this vertex already has all of its edges
    // loaded

    // MODIFIERS
    void setHasParent(bool hasParent);
};

} // end namespace Codethink::lvtshr

#endif // INCLUDED_CT_LVTSHR_LOADERINFO
