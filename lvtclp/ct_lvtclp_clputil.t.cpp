// ct_lvtclp_clputil.t.cpp                                            -*-C++-*-

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

#include <ct_lvtclp_clputil.h>

#include <catch2/catch.hpp>

using namespace Codethink;

TEST_CASE("normalisePath tests")
{
    // Common usage cases
    REQUIRE(lvtclp::ClpUtil::normalisePath("/home/abc/xxx/project/abc/", "/home/abc/xxx/project") == "abc/");
    REQUIRE(lvtclp::ClpUtil::normalisePath("/home/abc/xxx/project/../project/abc/", "/home/abc/xxx/project") == "abc/");
    REQUIRE(lvtclp::ClpUtil::normalisePath("/home/abc/xxx/project/../abc/", "/home/abc/xxx") == "abc/");

    // Invalid prefix cases
    REQUIRE(lvtclp::ClpUtil::normalisePath("/home/abc/xxx/project/abc/", "/different/path/")
            == "/home/abc/xxx/project/abc/");
    REQUIRE(lvtclp::ClpUtil::normalisePath("/home/abc/xxx/project/../project/abc/", "/different/path/")
            == "/home/abc/xxx/project/abc/");

    // Edge cases
    // Trailling '/' on the prefix path was being evaluated as invalid prefix. This test has been added to avoid
    // regression.
    REQUIRE(lvtclp::ClpUtil::normalisePath("/home/abc/xxx/project/abc/", "/home/abc/xxx/project/") == "abc/");

    // Non-canonical prefix path was being wrongly processed. This test has been added to avoid regression.
    REQUIRE(lvtclp::ClpUtil::normalisePath("/home/abc/xxx/project/abc/", "/home/abc/xxx/project/../")
            == "project/abc/");
}

TEST_CASE("Lakosian rules matching tests")
{
    REQUIRE(lvtclp::ClpUtil::isComponentOnPackageGroup("/abc/abcd/abcd_component.cpp"));
    REQUIRE(lvtclp::ClpUtil::isComponentOnPackageGroup("/abc/abcd/abcd_component.h"));
    REQUIRE(lvtclp::ClpUtil::isComponentOnPackageGroup("/xxx/xxxd/xxxd_component.cpp"));
    REQUIRE(lvtclp::ClpUtil::isComponentOnPackageGroup("/abc/abcd+otherstuff/abcd_component.cpp"));
    REQUIRE(
        lvtclp::ClpUtil::isComponentOnPackageGroup("/some/path/prefixes/doesnt/matter/xxx/xxxd/xxxd_component.cpp"));
    REQUIRE_FALSE(lvtclp::ClpUtil::isComponentOnPackageGroup("/xxxy/xxxyd/xxxyd_component.cpp"));
    REQUIRE_FALSE(lvtclp::ClpUtil::isComponentOnPackageGroup("/xx/xxd/xxd_component.cpp"));
    REQUIRE_FALSE(lvtclp::ClpUtil::isComponentOnPackageGroup("/xxx/randomname/xxx_component.cpp"));

    REQUIRE(lvtclp::ClpUtil::isComponentOnStandalonePackage("/s_abcabc/s_abcabc_component.cpp"));
    REQUIRE(lvtclp::ClpUtil::isComponentOnStandalonePackage("/abcabc/s_abcabc_component.cpp"));
    REQUIRE(lvtclp::ClpUtil::isComponentOnStandalonePackage("/abcabc/ct_abcabc_component.cpp"));
    REQUIRE(lvtclp::ClpUtil::isComponentOnStandalonePackage("/abcabc/ct_abcabc_component_with_many_underlines.cpp"));
    REQUIRE(
        lvtclp::ClpUtil::isComponentOnStandalonePackage("/abcabc/ct_abcabc_component_with____many___underlines.cpp"));
    REQUIRE_FALSE(lvtclp::ClpUtil::isComponentOnStandalonePackage("/boost/ct_abcabc_component.cpp"));
}

TEST_CASE("is File Ignored")
{
    REQUIRE(lvtclp::ClpUtil::isFileIgnored("moc_katewaiter.cpp", {"moc_*"}));
    REQUIRE(lvtclp::ClpUtil::isFileIgnored("moc_katewaiter.cpp", {"abcde*", "moc_*"}));
    REQUIRE_FALSE(lvtclp::ClpUtil::isFileIgnored("someotherfile.cpp", {"moc_*"}));
}
