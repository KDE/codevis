// ct_lvtgshr_fuzzyutil.h                                             -*-C++-*-

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

//@PURPOSE: Provide functions useful for fuzzy search
//
//@CLASSES:
//  lvtshr::FuzzyUtil

#ifndef INCLUDED_CT_LVTSHR_FUZZYUTIL
#define INCLUDED_CT_LVTSHR_FUZZYUTIL

#include <lvtshr_export.h>

#include <string>

namespace Codethink::lvtshr {

struct LVTSHR_EXPORT FuzzyUtil {
    // CLASS METHODS
    static size_t levensteinDistance(const std::string& source, const std::string& target);
    // Calculate the levenstein distance between two strings
};

} // end namespace Codethink::lvtshr

#endif // INCLUDED_CT_LVTSHR_FUZZYUTIL
