// ct_lvtldr_repositorynodefields.h                                        -*-C++-*-

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

#ifndef DIAGRAM_SERVER_CT_LVTLDR_REPOSITORYNODEFIELDS_H
#define DIAGRAM_SERVER_CT_LVTLDR_REPOSITORYNODEFIELDS_H

#include <ct_lvtshr_uniqueid.h>

#include <optional>
#include <string>
#include <vector>

namespace Codethink::lvtldr {

struct RepositoryNodeFields {
    using RecordNumberType = Codethink::lvtshr::UniqueId::RecordNumberType;

    RecordNumberType id = -1;
    int version = -1;
    std::string name;
    std::string qualifiedName;
    std::string diskPath;
    std::vector<RecordNumberType> childPackagesIds;
};

} // namespace Codethink::lvtldr

#endif // DIAGRAM_SERVER_CT_LVTLDR_COMPONENTNODEFIELDS_H
