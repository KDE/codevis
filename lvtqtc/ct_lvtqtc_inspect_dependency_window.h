// ct_lvtqtc_inspect_dependency_window.h                               -*-C++-*-

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
#ifndef DIAGRAM_SERVER_CT_LVTQTC_INSPECT_DEPENDENCY_WINDOW_H
#define DIAGRAM_SERVER_CT_LVTQTC_INSPECT_DEPENDENCY_WINDOW_H

#include <ct_lvtqtc_lakosrelation.h>

#include <ct_lvtldr_lakosiannode.h>
#include <ct_lvtqtc_graphicsscene.h>
#include <ct_lvtqtc_lakosentity.h>

#include <QDialog>
#include <QTableWidget>

#include <lvtqtc_export.h>

namespace Codethink::lvtprj {
class ProjectFile;
}

namespace Codethink::lvtldr {
class LakosianNode;
}

namespace Codethink::lvtqtc {

class LVTQTC_EXPORT InspectDependencyWindow : public QDialog {
  public:
    InspectDependencyWindow(lvtprj::ProjectFile const& projectFile, LakosRelation const& r, QDialog *parent = nullptr);

  protected:
    enum ContentTableRoles { InternalNodeDataRole = Qt::UserRole + 1, MenuRole };

    void setupUi();
    void openFile(lvtldr::LakosianNode *pkg, lvtldr::LakosianNode *component, const QString& ext);
    void populateContentsTable();

    virtual void showFileNotFoundWarning(QString const& filepath) const;

    LakosRelation const& d_lakosRelation;
    lvtprj::ProjectFile const& d_projectFile;
    QTableWidget *d_contentsTable = nullptr;
};

} // namespace Codethink::lvtqtc

#endif // DIAGRAM_SERVER_CT_LVTQTC_INSPECT_DEPENDENCY_WINDOW_H
