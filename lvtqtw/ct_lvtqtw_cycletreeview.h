// ct_lvtqtw_CYCLETREEVIEW.h                               -*-C++-*-

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

#ifndef DEFINED_CT_LVTQTW_CYCLETREEVIEW_H
#define DEFINED_CT_LVTQTW_CYCLETREEVIEW_H

#include <lvtqtw_export.h>

#include <QTreeView>
#include <memory>

namespace Codethink::lvtqtw {

class LVTQTW_EXPORT CycleTreeView : public QTreeView {
    Q_OBJECT
  public:
    CycleTreeView(QWidget *parent = nullptr);
    ~CycleTreeView() override;

    void setStringForEmptyState(const QString& emptyState);
    void setStringForInitialState(const QString& initialState);

    void setInitialState();
    // We haven't started to search for cycles yet.

    void setEmptyState();
    // We searched for cycles, but found none.

    void setModel(QAbstractItemModel *model) override;

  private:
    struct Private;
    std::unique_ptr<Private> d;
};

} // namespace Codethink::lvtqtw

#endif
