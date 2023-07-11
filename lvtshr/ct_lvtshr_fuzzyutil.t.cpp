// ct_lvtshr_fuzzyutil.t.cpp                                          -*-C++-*-

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

#include <array>
#include <string>
#include <tuple>
#include <utility>

#include <catch2/catch.hpp>

using Codethink::lvtshr::FuzzyUtil;

TEST_CASE("Fuzzy util")
{
    using TestCase = std::tuple<std::string, std::string, size_t>;

    static const std::array<TestCase, 11> tests = {TestCase{"", "", 0},
                                                   TestCase{"a", "", 1},
                                                   TestCase{"", "a", 1},
                                                   TestCase{"kitten", "sitting", 3},
                                                   TestCase{"12", "", 2},
                                                   TestCase{"akitten", "asitting", 3},
                                                   TestCase{"akitten", "sitting", 4},
                                                   TestCase{"kitten", "asitting", 4},
                                                   TestCase{"pkg", "package", 4},
                                                   TestCase{"book", "back", 2},
                                                   TestCase{"apple", "jklfdas", 7}};

    for (const auto& test : tests) {
        const auto& lhs = std::get<0>(test);
        const auto& rhs = std::get<1>(test);
        const auto expDist = std::get<2>(test);
        const auto dist = FuzzyUtil::levensteinDistance(lhs, rhs);
        CHECK(expDist == dist);
    }
}
