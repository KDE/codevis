// Copyright 2024 Codethink Ltd <codethink@codethink.co.uk>
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

#include <ct_lvtprj_projectfile.h>
#include <merge_project_databases.h>

#include <catch2-local-includes.h>

#include <QTemporaryDir>

#include <iostream>

// the test to merge multiple databases already exists in `lvtmdb`.
// this just validates no crashes while using project helpers::mergeProjectDatabase
TEST_CASE("Test Merge Databases")
{
    QTemporaryDir prjFolders;
    std::filesystem::path tmp_path = prjFolders.path().toStdString();

    auto projectFile1 = Codethink::lvtprj::ProjectFile{};
    auto projectFile2 = Codethink::lvtprj::ProjectFile{};
    auto err1 = projectFile1.createEmpty();
    REQUIRE_FALSE(err1.has_error());

    auto err2 = projectFile2.createEmpty();
    REQUIRE_FALSE(err2.has_error());

    auto err3 = projectFile1.saveAs(tmp_path / "prj1.lks", Codethink::lvtprj::ProjectFile::BackupFileBehavior::Keep);
    REQUIRE_FALSE(err3.has_error());

    auto err4 = projectFile2.saveAs(tmp_path / "prj2.lks", Codethink::lvtprj::ProjectFile::BackupFileBehavior::Keep);
    REQUIRE_FALSE(err4.has_error());

    auto err5 = Codethink::MergeProjects::mergeDatabases({tmp_path / "prj1.lks", tmp_path / "prj2.lks"},
                                                         tmp_path / "resulting_prj.lks",
                                                         std::nullopt);
    if (err5.has_error()) {
        std::cout << err5.error().what << "\n";
    }
    REQUIRE_FALSE(err5.has_error());
}
