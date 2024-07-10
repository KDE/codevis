// ct_lvtclp_compilerutil.t.cpp                                            -*-C++-*-

/*
 / / Copyright 2023 Codethink Ltd *<codethink@codethink.co.uk>
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

#include <ct_lvtclp_compilerutil.h>

#include <catch2-local-includes.h>
#include <iostream>
#include <sstream>
#include <string>

using namespace Codethink;

TEST_CASE("search for stddef")
{
#ifndef Q_OS_WINDOWS
    // TODO: forcing to gcc, but we should test if gcc is installed,
    // if clang is installed, and run this for both.
    std::optional<std::string> res = Codethink::lvtclp::CompilerUtil::runCompiler("gcc");
    REQUIRE(res.has_value());

    int begin_search_idx = 0;
    int end_search_idx = 0;
    int curr_idx = 0;
    std::istringstream f(res.value());
    std::string curr;

    // do not change the capitalization of the strings on the search.
    while (getline(f, curr, '\n')) {
        if (curr.find("search starts here") != curr.npos) {
            begin_search_idx = curr_idx;
        }
        if (curr.find("End of search") != curr.npos) {
            end_search_idx = curr_idx;
        }
        curr_idx += 1;
    }

    REQUIRE(begin_search_idx != end_search_idx);
    REQUIRE(begin_search_idx < end_search_idx);
    REQUIRE((end_search_idx - begin_search_idx) > 1);
#endif
}
