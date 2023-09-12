// ct_lvtqtc_pluginmanagerutils.h                                    -*-C++-*-

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

#ifndef DIAGRAM_SERVER_CT_LVTQTC_PLUGINMANAGERUTILS_H
#define DIAGRAM_SERVER_CT_LVTQTC_PLUGINMANAGERUTILS_H

#include <lvtqtc_export.h>

#include <ct_lvtplg_handlertreewidget.h>
#include <ct_lvtplg_pluginmanager.h>
#include <ct_lvtqtc_graphicsscene.h>

#include <QStandardItem>
#include <QTreeView>

namespace Codethink::lvtqtc {

class LVTQTC_EXPORT PluginManagerQtUtils {
  public:
    static PluginTreeWidgetHandler
    createPluginTreeWidgetHandler(lvtplg::PluginManager *pm, std::string const& id, GraphicsScene *gs);

  private:
    static PluginTreeItemHandler createPluginTreeItemHandler(lvtplg::PluginManager *pm,
                                                             QTreeView *treeView,
                                                             QStandardItemModel *treeModel,
                                                             QStandardItem *item,
                                                             GraphicsScene *gs);

    static PluginTreeItemClickedActionHandler createPluginTreeItemClickedActionHandler(lvtplg::PluginManager *pm,
                                                                                       QTreeView *treeView,
                                                                                       QStandardItemModel *treeModel,
                                                                                       QStandardItem *item,
                                                                                       GraphicsScene *gs);

    static PluginGraphicsViewHandler createPluginGraphicsViewHandler(GraphicsScene *gs);
};

} // namespace Codethink::lvtqtc

#endif // DIAGRAM_SERVER_CT_LVTQTC_PLUGINMANAGERUTILS_H
