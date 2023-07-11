// projectsettingsdialog.t.cpp                                                                                 -*-C++-*-

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

#include <catch2/catch.hpp>
#include <ct_lvtprj_projectfile.h>
#include <ct_lvttst_fixture_qt.h>
#include <projectsettingsdialog.h>
#include <ui_projectsettingsdialog.h>

using namespace Codethink::lvtprj;

class ProjectSettingsDialogForTesting : public ProjectSettingsDialog {
  public:
    using ProjectSettingsDialog::ProjectSettingsDialog;

    std::string projectNameText()
    {
        return ui->projectNameValue->text().toStdString();
    }

    void setProjectNameText(std::string const& s)
    {
        return ui->projectNameValue->setText(QString::fromStdString(s));
    }

    std::string sourcePathText()
    {
        return ui->sourceCodePathValue->text().toStdString();
    }

    void setSourcePathText(std::string const& s)
    {
        return ui->sourceCodePathValue->setText(QString::fromStdString(s));
    }

    void clickApply()
    {
        ui->buttonBox->button(QDialogButtonBox::StandardButton::Apply)->click();
    }
};

TEST_CASE_METHOD(QTApplicationFixture, "Basic workflow")
{
    auto projectModel = ProjectFile{};

    projectModel.setProjectName("MyProject");
    projectModel.setSourceCodePath("/a/b/c/");

    auto dialog = ProjectSettingsDialogForTesting{projectModel};
    dialog.show();

    REQUIRE(dialog.projectNameText() == "MyProject");
    REQUIRE(dialog.sourcePathText() == "/a/b/c/");
    dialog.setProjectNameText("OtherName");
    dialog.setSourcePathText("/d/e/f/");

    // Changes are not applied while the button "apply" is not clicked
    REQUIRE(projectModel.projectName() == "MyProject");
    REQUIRE(projectModel.sourceCodePath() == "/a/b/c/");
    dialog.clickApply();
    REQUIRE(projectModel.projectName() == "OtherName");
    REQUIRE(projectModel.sourceCodePath() == "/d/e/f/");
}
