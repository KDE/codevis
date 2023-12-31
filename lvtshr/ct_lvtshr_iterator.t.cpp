// ct_lvtshr_iterator.t.cpp                                                                                    -*-C++-*-

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

#include <ct_lvtshr_iterator.h>
#include <vector>

#include <catch2/catch.hpp>

using Codethink::lvtshr::enumerate;

TEST_CASE("enumerate")
{
    auto vec = std::vector<std::size_t>{1, 2, 3, 4, 5};
    for (auto const& [i, v] : enumerate(vec)) {
        REQUIRE(i == v - 1);
    }
}
