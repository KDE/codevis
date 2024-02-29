// ct_lvtqtw_parse_codebase.t.cpp -*-C++-*-

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

#include <ct_lvtqtw_parse_codebase.h>
#include <fstream>
#include <ui_ct_lvtqtw_parse_codebase.h>

#include <ct_lvttst_fixture_qt.h>
#include <ct_lvttst_tmpdir.h>

#include <filesystem>

#include <catch2-local-includes.h>

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTemporaryFile>
using namespace Codethink::lvtqtw;

class ExposingParseCodebaseDialog : public ParseCodebaseDialog {
  public:
    bool sourceFolderErrorIsVisible()
    {
        return ui->projectSourceFolderError->isVisible();
    }

    bool buildFolderErrorIsVisible()
    {
        return ui->projectBuildFolderError->isVisible();
    }

    void setCompileCommandsFolder(const QString& compileCommandsFolder)
    {
        ui->compileCommandsFolder->setText(compileCommandsFolder);
    }

    void setSourceFolder(const QString& sourceFolder)
    {
        ui->sourceFolder->setText(sourceFolder);
    }

    QString buildFolderError()
    {
        return ui->projectBuildFolderError->text();
    }

    QString sourceFolderError()
    {
        return ui->projectSourceFolderError->text();
    }
};

QString sample_compile_commands_json()
{
    return QString(R"(
        [
        {
            'directory': "/home/user/kde/build/codevis",
            "command": "some command here",
            "file": "some file here",
            "output": "some output here",
        }
        ]
           )");
}

std::filesystem::path createFolderWithCompileCommandsJson(const std::string& folder_name)
{
    std::filesystem::path folder_path = folder_name;
    auto file_path = folder_path / "compile_commands.json";

    std::filesystem::create_directory(folder_path);
    std::ofstream output_file(file_path);
    if (output_file.is_open()) {
        output_file << sample_compile_commands_json();
        output_file.close();
    }

    assert(std::filesystem::exists(file_path));

    return folder_path;
}

TEST_CASE_METHOD(QTApplicationFixture, "Build Folder Validation works correctly")
{
    // depending on requirements those tests can be more general(just that an error is shown)
    // or that the message contains a set of keywords
    auto parseCodeBaseDialog = ExposingParseCodebaseDialog{};
    parseCodeBaseDialog.show();

    auto folder_has_compile_commands_json = createFolderWithCompileCommandsJson("parse_codebase_temp");

    parseCodeBaseDialog.setCompileCommandsFolder(QString(folder_has_compile_commands_json.string().c_str()));
    qDebug() << parseCodeBaseDialog.buildFolderError() + "<-- message of build folder error";
    REQUIRE(!parseCodeBaseDialog.buildFolderErrorIsVisible());
    std::filesystem::remove_all(folder_has_compile_commands_json);
    // should not show error

    // set a wsl folder
    parseCodeBaseDialog.setCompileCommandsFolder(QString("wsl://kde/build/codevis"));
    REQUIRE(parseCodeBaseDialog.buildFolderErrorIsVisible());
    REQUIRE(parseCodeBaseDialog.buildFolderError().contains("wsl"));
    // should show an error

    // set empty string folder
    parseCodeBaseDialog.setCompileCommandsFolder(QString(""));
    REQUIRE(parseCodeBaseDialog.buildFolderError().contains("empty"));
    REQUIRE(parseCodeBaseDialog.buildFolderErrorIsVisible());
    // should show an error

    // set folder without compile_commands_json
    auto dir2 = QTemporaryDir();

    parseCodeBaseDialog.setCompileCommandsFolder(dir2.path());
    REQUIRE(parseCodeBaseDialog.buildFolderErrorIsVisible());
    REQUIRE(parseCodeBaseDialog.buildFolderError().contains("compile_commands.json"));
}

TEST_CASE_METHOD(QTApplicationFixture, "Source Folder validation works correctly")
{
    auto parseCodeBaseDialog = ExposingParseCodebaseDialog{};
    parseCodeBaseDialog.show();
    // set wsl folder
    parseCodeBaseDialog.setSourceFolder(QString("wsl://kde/build/codevis"));
    REQUIRE(parseCodeBaseDialog.sourceFolderErrorIsVisible());
    REQUIRE(parseCodeBaseDialog.sourceFolderError().contains("wsl"));

    // set empty string folder
    parseCodeBaseDialog.setSourceFolder(QString(""));
    REQUIRE(parseCodeBaseDialog.sourceFolderErrorIsVisible());
    REQUIRE(parseCodeBaseDialog.sourceFolderError().contains("empty"));

    // set ok folder
    auto okDir = QTemporaryDir();
    // add files (what files should be added?)

    parseCodeBaseDialog.setSourceFolder(okDir.path());
    REQUIRE(!parseCodeBaseDialog.sourceFolderErrorIsVisible());
}
