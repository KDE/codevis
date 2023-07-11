// ct_lvtcgn_codegentreemodel.h                                       -*-C++-*-

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

#ifndef _CT_LVTCGN_COGEDENTREEMODEL_H_INCLUDED
#define _CT_LVTCGN_COGEDENTREEMODEL_H_INCLUDED

#include <lvtcgn_gui_export.h>

#include <QList>
#include <QStandardItemModel>
#include <QVariant>

namespace Codethink::lvtcgn::mdl {
class ICodeGenerationDataProvider;
class IPhysicalEntityInfo;
} // namespace Codethink::lvtcgn::mdl

namespace Codethink::lvtcgn::gui {

enum CodeGenerationDataRole { EntityNameRole = Qt::UserRole, InfoReferenceRole = Qt::UserRole + 1 };

class LVTCGN_GUI_EXPORT CodeGenerationEntitiesTreeModel : public QStandardItemModel {
  public:
    enum class RecursiveExec { ContinueSearch, StopSearch };

    explicit CodeGenerationEntitiesTreeModel(mdl::ICodeGenerationDataProvider& dataProvider, QObject *parent = 0);
    void refreshContents();
    void recursiveExec(std::function<RecursiveExec(QStandardItem *)> f);

  private:
    void populateItemAndChildren(mdl::IPhysicalEntityInfo& info, QStandardItem *parent);
    void updateAllChildrenState(QStandardItem *item);
    void updateParentState(QStandardItem *item);

    mdl::ICodeGenerationDataProvider& dataProvider;

    bool itemChangedCascadeUpdate = true;
    // Helper boolean to control cascade updates in the itemChanged signal
};

} // namespace Codethink::lvtcgn::gui

#endif
