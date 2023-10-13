// ct_lvtprj_project_file.t.cpp                                         -*-C++-*-

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

#include <ct_lvtprj_projectfile.h>

#include <filesystem>
#include <iostream>

#include <QString> // for .endsWith

#include <catch2-local-includes.h>

namespace fs = std::filesystem;

TEST_CASE("Project Management")
{
    fs::path project_path = fs::temp_directory_path() / "ct_lvt_testprojects" / "testproject.lks";
    fs::path project_backup = fs::temp_directory_path() / "ct_lvt_testprojects" / "testproject.lks";

    Codethink::lvtprj::ProjectFile file;

    REQUIRE(file.isOpen() == false);

    auto err = file.createEmpty();
    REQUIRE(!err.has_error());
    REQUIRE(file.isOpen() == true);

    file.setProjectName("CodeVis");
    file.setProjectInformation("Sligthly longer line of information");
    file.setSourceCodePath("/some/path/src/");

    REQUIRE(file.location() == "");

    // Open location uses a random directory name, I can't check for equality.
    REQUIRE(file.openLocation().empty() == false);

    REQUIRE(file.projectName() == "CodeVis");
    REQUIRE(file.projectInformation() == "Sligthly longer line of information");
    REQUIRE(file.sourceCodePath() == "/some/path/src/");

    err = file.saveAs(project_path, Codethink::lvtprj::ProjectFile::BackupFileBehavior::Discard);
    if (err.has_error()) {
        FAIL("Unexpected error: " << err.error().errorMessage);
    }
    REQUIRE(err.has_value());
    REQUIRE(!err.has_error());
    REQUIRE(file.location() == project_path);

    Codethink::lvtprj::ProjectFile project2;
    err = project2.open(project_path);
    if (err.has_error()) {
        FAIL("Unexpected error: " << err.error().errorMessage);
    }

    // check that the information of both projects is the same.
    REQUIRE(project2.projectName() == file.projectName());
    REQUIRE(project2.projectInformation() == file.projectInformation());
    REQUIRE(project2.location() == file.location());

    // those needs to be different since there are two instances of the same project open.
    REQUIRE(project2.openLocation() != file.openLocation());

    fs::path project_path_without_suffix = fs::temp_directory_path() / "ct_lvt_testprojects" / "testproject";
    err = file.saveAs(project_path_without_suffix, Codethink::lvtprj::ProjectFile::BackupFileBehavior::Discard);
    if (err.has_error()) {
        FAIL("Unexpected error: " << err.error().errorMessage);
    }
    REQUIRE(!err.has_error());

    QString locationWithoutSuffix = QString::fromStdString(file.location().string());
    REQUIRE(locationWithoutSuffix.endsWith(".lks"));

    // cleanup
    try {
        std::filesystem::remove(project_path);
        std::filesystem::remove(project_backup);
    } catch (...) {
    };
}
