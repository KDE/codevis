// ct_lvtmdb_util.h                                                   -*-C++-*-

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

#ifndef INCLUDED_CT_LVTMDB_UTIL
#define INCLUDED_CT_LVTMDB_UTIL

#include <lvtmdb_export.h>

#include <algorithm>
#include <vector>

namespace Codethink::lvtmdb {

// ==========================
// class MdbUtil
// ==========================

struct LVTMDB_EXPORT MdbUtil {
  public:
    // CLASS METHODS
    template<class OBJECT>
    static void pushBackUnique(std::vector<OBJECT *>& vec, OBJECT *obj)
    {
        const auto it = std::find(vec.begin(), vec.end(), obj);
        if (it == vec.end()) {
            vec.push_back(obj);
        }
    }

    enum class ErrorKind {
        CompilerError, // an actual error from clang
        ParserError, // an error from our tool
    };
};

} // namespace Codethink::lvtmdb

#endif // INCLUDED_CT_LVTMDB_UTIL
