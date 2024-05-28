// ct_lvtqtw_tabwidget.h                                             -*-C++-*-

/*
 * // Copyright 2023 Codethink Ltd <codethink@codethink.co.uk>
 * // SPDX-License-Identifier: Apache-2.0
 * //
 * // Licensed under the Apache License, Version 2.0 (the "License");
 * // you may not use this file except in compliance with the License.
 * // You may obtain a copy of the License at
 * //
 * //     http://www.apache.org/licenses/LICENSE-2.0
 * //
 * // Unless required by applicable law or agreed to in writing, software
 * // distributed under the License is distributed on an "AS IS" BASIS,
 * // WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * // See the License for the specific language governing permissions and
 * // limitations under the License.
 */

#ifndef INCLUDED_LVTQTW_SQLEDITOR
#define INCLUDED_LVTQTW_SQLEDITOR

#include <lvtqtw_export.h>

#include <ct_lvtqtw_tabwidget.h>

#include <ct_lvtldr_lakosiannode.h>
#include <ct_lvtmdl_sqlmodel.h>
#include <ct_lvtplg_pluginmanager.h>

#include <QSplitter>

#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/View>

class QTableView;

namespace Codethink::lvtqtw {

class LVTQTW_EXPORT SqlEditor : public QWidget
// Visualizes a Tree model and reacts to changes
{
    Q_OBJECT
  public:
    SqlEditor(lvtldr::NodeStorage& sharedStorage, QWidget *parent = 0);

  private:
    lvtmdl::SqlModel *model;
    QTableView *tableView;
};
} // namespace Codethink::lvtqtw

#endif
