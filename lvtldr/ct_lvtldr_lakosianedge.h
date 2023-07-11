// ct_lvtldr_lakosianedge.h                                          -*-C++-*-

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

#ifndef INCLUDED_CT_LVTLDR_LAKOSIANEDGE
#define INCLUDED_CT_LVTLDR_LAKOSIANEDGE

#include <lvtldr_export.h>

#include <ct_lvtshr_graphenums.h>
#include <ct_lvtshr_uniqueid.h>

#include <boost/signals2.hpp>

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <result/result.hpp>

namespace Codethink::lvtldr {

class LakosianNode;

// ==========================
// class LakosianEdge
// ==========================

class LVTLDR_EXPORT LakosianEdge {
    // Models a relation between LakosianNodes

  private:
    // DATA
    lvtshr::LakosRelationType d_type;
    LakosianNode *d_other;

  public:
    LakosianEdge(lvtshr::LakosRelationType type, LakosianNode *other);

    ~LakosianEdge() noexcept = default;

    LakosianEdge(LakosianEdge&& other) noexcept;
    LakosianEdge(const LakosianEdge& other);

    // ACCESSORS
    [[nodiscard]] lvtshr::LakosRelationType type() const;
    [[nodiscard]] LakosianNode *other() const;
};

LVTLDR_EXPORT bool operator==(LakosianEdge const& lhs, LakosianEdge const& rhs);

} // namespace Codethink::lvtldr

#endif // INCLUDED_CT_LVTLDR_LAKOSIANEDGE
