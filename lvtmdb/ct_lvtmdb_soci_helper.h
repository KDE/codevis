// ct_lvtmdb_soci_helper.h                                         -*-C++-*-

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

#ifndef INCLUDED_CT_LVTMDB_SOCI_HELPER
#define INCLUDED_CT_LVTMDB_SOCI_HELPER

namespace Codethink::lvtmdb {
struct SociHelper {
    //  Small helper methods and enums to be used from soci writer and
    // soci reader.

    enum class Key : int {
        Invalid = 0,
        DatabaseState = 1,
        Version = 2,
    };

    enum class Version : int {
        Unknown = 0,
        Jan22 = 1,
        March22 = 2,
        March23 = 3,
    };

    static constexpr int CURRENT_VERSION = static_cast<int>(Version::March23);
};

} // namespace Codethink::lvtmdb
#endif
