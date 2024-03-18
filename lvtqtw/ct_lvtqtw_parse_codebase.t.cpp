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

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTemporaryFile>

#include <catch2-local-includes.h>
#include <test-project-paths.h>
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

    void compileCommandsFolder()
    {
        qDebug() << ui->compileCommandsFolder->text();
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

std::string sample_compile_commands_json()
{
    auto PREFIX = std::string{TEST_QTW_PATH};
    auto compile_commands_json_path = PREFIX + "/sample_compile_commands.json";
    auto file = QFile(QString(compile_commands_json_path.c_str()));
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "sample_compile_commands.json was not found in the testfiles folder for lvtqtw";
    }
    QTextStream in(&file);
    auto content = in.readAll();
    file.close();
    return content.toStdString();
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

bool file_exists(const QString& file_path)
{
    QFileInfo inf(file_path);
    return inf.exists();
}

TEST_CASE_METHOD(QTApplicationFixture, "Build Folder Validation works correctly")
{
    // depending on requirements those tests can be more general(just that an error is shown)
    // or that the message contains a set of keywords
    auto parseCodeBaseDialog = ExposingParseCodebaseDialog{};
    parseCodeBaseDialog.show();

    // auto tempDir = TmpDir("parse_codebase_temp");
    // auto file = tempDir.createTextFile("compile_commands.json", sample_compile_commands_json());
    // auto folder_has_compile_commands_json = tempDir.path();
    // parseCodeBaseDialog.setCompileCommandsFolder(QString(folder_has_compile_commands_json.c_str()));
    // parseCodeBaseDialog.compileCommandsFolder();
    // qDebug() << parseCodeBaseDialog.buildFolderError() + "<-- message of build folder error";
    // auto checkedFilePath =
    //     QString(folder_has_compile_commands_json.c_str()) + QDir::separator() + "compile_commands.json";
    // // REQUIRE(file_exists(QString(checkedFilePath)));
    // REQUIRE(!parseCodeBaseDialog.buildFolderErrorIsVisible());
    /*
        QString filesInTmpDir;
        for (const auto& entry : std::filesystem::directory_iterator(tempDir.path())) {
            filesInTmpDir.append("File: ").append(entry.path().c_str());
        }

        qDebug() << QString(tempDir.path().c_str()) + QDir::separator() + "compile_commands.json";
        auto file_info = QFileInfo(QString(file.c_str()));
        qDebug() << "Full File path: " << file_info.absoluteFilePath().toStdString();

        qDebug() << "File List: "
                 << "\n"
                 << filesInTmpDir;*/
    auto folder_has_compile_commands_json = createFolderWithCompileCommandsJson("parse_codebase_temp");
    qDebug() << parseCodeBaseDialog.buildFolderError() + "<-- message of build folder error";
    parseCodeBaseDialog.setCompileCommandsFolder(QString(folder_has_compile_commands_json.c_str()));
    REQUIRE(!parseCodeBaseDialog.buildFolderErrorIsVisible());
    std::filesystem::remove_all(folder_has_compile_commands_json);

    parseCodeBaseDialog.setCompileCommandsFolder(QString("wsl://kde/build/codevis"));
    REQUIRE(parseCodeBaseDialog.buildFolderErrorIsVisible());
    REQUIRE(parseCodeBaseDialog.buildFolderError().contains("wsl"));

    parseCodeBaseDialog.setCompileCommandsFolder(QString(""));
    REQUIRE(parseCodeBaseDialog.buildFolderError().contains("empty"));
    REQUIRE(parseCodeBaseDialog.buildFolderErrorIsVisible());

    // folder without compile_commands_json
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
