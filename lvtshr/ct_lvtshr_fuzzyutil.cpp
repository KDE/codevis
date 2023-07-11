// ct_lvtshr_fuzzyutil.cpp                                           -*-C++-*-

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

#include <ct_lvtshr_fuzzyutil.h>

#include <algorithm>
#include <vector>

namespace Codethink::lvtshr {

// the number means the number of different letters on each string.
// adapted from different algorithms
// 0 means that the strings are equal
// 1 means one different letter
// 2 means two different letters, and so on, untill string.size().
size_t FuzzyUtil::levensteinDistance(const std::string& source, const std::string& target)
{
    if (source.size() > target.size()) {
        return levensteinDistance(target, source);
    }

    const size_t min_size = source.size();
    const size_t max_size = target.size();
    std::vector<size_t> distances(min_size + 1);

    for (size_t i = 0; i <= min_size; ++i) {
        distances[i] = i;
    }

    for (size_t j = 1; j <= max_size; ++j) {
        size_t previous_diagonal = distances[0];

        distances[0] += 1;
        for (size_t i = 1; i <= min_size; ++i) {
            size_t previous_diagonal_save = distances[i];

            decltype(source.size()) idx_1 = i - 1;
            decltype(source.size()) idx_2 = j - 1;

            if (source[idx_1] == target[idx_2]) {
                distances[i] = previous_diagonal;
            } else {
                distances[i] = std::min({distances[i - 1], distances[i], previous_diagonal}) + 1;
            }

            previous_diagonal = previous_diagonal_save;
        }
    }

    return distances[min_size];
}

} // end namespace Codethink::lvtshr
