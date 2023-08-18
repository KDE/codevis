// ct_lvtshr_stringhelpers.t.cpp                                                                               -*-C++-*-

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

#include <ct_lvtshr_stringhelpers.h>

#include <catch2-local-includes.h>

using Codethink::lvtshr::StrUtil;

TEST_CASE("beginsWith")
{
    REQUIRE(StrUtil::beginsWith("bla", "b"));
    REQUIRE(StrUtil::beginsWith("bla", "bl"));
    REQUIRE(StrUtil::beginsWith("bla", "bla"));
    REQUIRE(StrUtil::beginsWith("pkg_component", "pkg_"));

    REQUIRE_FALSE(StrUtil::beginsWith("bla", "l"));
    REQUIRE_FALSE(StrUtil::beginsWith("bla", "la"));
    REQUIRE_FALSE(StrUtil::beginsWith("bla", "a"));
    REQUIRE_FALSE(StrUtil::beginsWith("pkg_component", "kg_"));
}
