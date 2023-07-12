// ct_lvtldr_packagenodefields.h                                     -*-C++-*-

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

#ifndef DIAGRAM_SERVER_CT_LVTLDR_PACKAGENODEFIELDS_H
#define DIAGRAM_SERVER_CT_LVTLDR_PACKAGENODEFIELDS_H

#include <ct_lvtshr_uniqueid.h>

#include <optional>
#include <string>
#include <vector>

namespace Codethink::lvtldr {

struct PackageNodeFields {
    using RecordNumberType = Codethink::lvtshr::UniqueId::RecordNumberType;

    RecordNumberType id = -1;
    int version = -1;
    std::optional<RecordNumberType> parentId = std::nullopt;
    std::optional<RecordNumberType> sourceRepositoryId = std::nullopt;
    std::string name;
    std::string qualifiedName;
    std::string diskPath;
    std::vector<RecordNumberType> childPackagesIds;
    std::vector<RecordNumberType> childComponentsIds;
    std::vector<RecordNumberType> providerIds;
    std::vector<RecordNumberType> clientIds;
    std::vector<RecordNumberType> allowedDependenciesIds;
};

} // namespace Codethink::lvtldr

#endif // DIAGRAM_SERVER_CT_LVTLDR_PACKAGENODEFIELDS_H
