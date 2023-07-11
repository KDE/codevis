// ct_lvtqtw_tabwidget.h                                             -*-C++-*-

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

#ifndef INCLUDED_LVTQTW_SPLITTERVIEW
#define INCLUDED_LVTQTW_SPLITTERVIEW

#include <lvtqtw_export.h>

#include <ct_lvtqtw_tabwidget.h>

#include <ct_lvtldr_lakosiannode.h>
#include <ct_lvtplg_pluginmanager.h>

#include <QSplitter>

#include <memory>

namespace Codethink::lvtqtc {
class GraphicsView;
class UndoManager;
}

namespace Codethink::lvtprj {
class ProjectFile;
}

class QGraphicsView;
class QString;

namespace Codethink::lvtqtw {

class LVTQTW_EXPORT SplitterView : public QSplitter
// Visualizes a Tree model and reacts to changes
{
    Q_OBJECT
  public:
    explicit SplitterView(lvtldr::NodeStorage& nodeStorage,
                          lvtprj::ProjectFile& projectFile,
                          lvtplg::PluginManager *pluginManager = nullptr,
                          QWidget *parent = nullptr);
    ~SplitterView() override;

    void setCurrentIndex(int idx);
    void setUndoManager(lvtqtc::UndoManager *undoManager);
    void closeAllTabs();

    Q_SIGNAL void currentTabChanged(Codethink::lvtqtw::TabWidget *wdg);

    void toggle();

    lvtqtc::GraphicsView *graphicsView();
    int totalOpenTabs();

  protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

  private:
    struct Private;
    std::unique_ptr<Private> d;
};

} // end namespace Codethink::lvtqtw

#endif
