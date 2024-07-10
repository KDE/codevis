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

#include <QFileInfo>
#include <catch2-local-includes.h>
#include <ct_lvttst_tmpdir.h>
#include <filesystem>

TEST_CASE("TMPDir")
{
    std::filesystem::path temp_dir_path;
    std::filesystem::path temp_file_path;
    {
        TmpDir dir("test_dir");
        temp_dir_path = dir.createDir("some_dir");

        REQUIRE(std::filesystem::exists(temp_dir_path));

        temp_file_path = dir.createTextFile("some_file.txt", "test contents");
        REQUIRE(std::filesystem::exists(temp_file_path));
    }

    REQUIRE(!std::filesystem::exists(temp_dir_path));
    REQUIRE(!std::filesystem::exists(temp_file_path));
}
