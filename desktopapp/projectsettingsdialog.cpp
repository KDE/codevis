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

#include <projectsettingsdialog.h>
#include <ui_projectsettingsdialog.h>

#include <QDebug>
#include <QFileDialog>

ProjectSettingsDialog::ProjectSettingsDialog(Codethink::lvtprj::ProjectFile& projectFile):
    ui(std::make_unique<Ui::ProjectSettingsDialog>()), d_projectFile(projectFile)
{
    ui->setupUi(this);

    ui->projectNameValue->setText(QString::fromStdString(d_projectFile.projectName()));
    ui->sourceCodePathValue->setText(QString::fromStdString(d_projectFile.sourceCodePath().string()));

    auto *applyBtn = ui->buttonBox->button(QDialogButtonBox::StandardButton::Apply);
    connect(applyBtn, &QPushButton::clicked, this, &ProjectSettingsDialog::applyChanges);
    connect(ui->searchSourceCodePathButton, &QPushButton::clicked, this, &ProjectSettingsDialog::searchSourceCodePath);
}

ProjectSettingsDialog::~ProjectSettingsDialog() = default;

void ProjectSettingsDialog::applyChanges()
{
    d_projectFile.setProjectName(ui->projectNameValue->text().toStdString());
    d_projectFile.setSourceCodePath(ui->sourceCodePathValue->text().toStdString());
    close();
}

void ProjectSettingsDialog::searchSourceCodePath()
{
    auto dir = QFileDialog::getExistingDirectory(this, tr("Source Directory"), QDir::homePath());
    if (dir.isEmpty()) {
        // User hits cancel
        return;
    }
    ui->sourceCodePathValue->setText(dir);
}
