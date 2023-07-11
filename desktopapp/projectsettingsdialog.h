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

#ifndef PROJECTSETTINGS_DIALOG_H
#define PROJECTSETTINGS_DIALOG_H

#include <ct_lvtprj_projectfile.h>

#include <QDialog>
#include <memory>

namespace Ui {
class ProjectSettingsDialog;
}

class ProjectSettingsDialog : public QDialog {
    Q_OBJECT
  public:
    explicit ProjectSettingsDialog(Codethink::lvtprj::ProjectFile& projectFile);
    ~ProjectSettingsDialog() override;

  protected:
    void applyChanges();
    void searchSourceCodePath();

    std::unique_ptr<Ui::ProjectSettingsDialog> ui;
    Codethink::lvtprj::ProjectFile& d_projectFile;
};

#endif
