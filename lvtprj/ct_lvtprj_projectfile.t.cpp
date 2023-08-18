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

    CHECK(file.isOpen() == false);

    auto err = file.createEmpty();
    CHECK(!err.has_error());
    CHECK(file.isOpen() == true);

    file.setProjectName("CodeVis");
    file.setProjectInformation("Sligthly longer line of information");
    file.setSourceCodePath("/some/path/src/");

    CHECK(file.location() == "");

    // Open location uses a random directory name, I can't check for equality.
    CHECK(file.openLocation().empty() == false);

    CHECK(file.projectName() == "CodeVis");
    CHECK(file.projectInformation() == "Sligthly longer line of information");
    CHECK(file.sourceCodePath() == "/some/path/src/");

    err = file.saveAs(project_path, Codethink::lvtprj::ProjectFile::BackupFileBehavior::Discard);
    REQUIRE(err.has_value());

    CHECK(!err.has_error());
    CHECK(file.location() == project_path);

    Codethink::lvtprj::ProjectFile project2;
    err = project2.open(project_path);
    if (err.has_error()) {
        FAIL("Unexpected error: " << err.error().errorMessage);
    }

    // check that the information of both projects is the same.
    CHECK(project2.projectName() == file.projectName());
    CHECK(project2.projectInformation() == file.projectInformation());
    CHECK(project2.location() == file.location());

    // those needs to be different since there are two instances of the same project open.
    CHECK(project2.openLocation() != file.openLocation());

    fs::path project_path_without_suffix = fs::temp_directory_path() / "ct_lvt_testprojects" / "testproject";
    err = file.saveAs(project_path_without_suffix, Codethink::lvtprj::ProjectFile::BackupFileBehavior::Discard);
    CHECK(!err.has_error());

    QString locationWithoutSuffix = QString::fromStdString(file.location().string());
    CHECK(locationWithoutSuffix.endsWith(".lks"));

    // cleanup
    try {
        std::filesystem::remove(project_path);
        std::filesystem::remove(project_backup);
    } catch (...) {
    };
}
