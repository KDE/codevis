// ct_lvtldr_freefunctionnodefields.h                                        -*-C++-*-

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

#ifndef DIAGRAM_SERVER_CT_LVTLDR_FREEFUNCTIONNODEFIELDS_H
#define DIAGRAM_SERVER_CT_LVTLDR_FREEFUNCTIONNODEFIELDS_H

#include <ct_lvtshr_uniqueid.h>

#include <optional>
#include <string>
#include <vector>

namespace Codethink::lvtldr {

struct FreeFunctionNodeFields {
    using RecordNumberType = Codethink::lvtshr::UniqueId::RecordNumberType;

    RecordNumberType id = -1;
    int version = -1;
    std::string name;
    std::string qualifiedName;

    // Heuristic data - Free functions are actually related to source files, but we prepare them
    // as component ID and store as a pseudo-field to ease search. This should be reviewed as soon as
    // we decide between (a) Add a ComponentId field in the DB _or_ (b) Move this to be lazy evaluated
    // on the class (FreeFunctionNode).
    std::optional<RecordNumberType> componentId = std::nullopt;

    std::vector<RecordNumberType> callerIds;
    std::vector<RecordNumberType> calleeIds;
};

} // namespace Codethink::lvtldr

#endif // DIAGRAM_SERVER_CT_LVTLDR_FREEFUNCTIONNODEFIELDS_H
