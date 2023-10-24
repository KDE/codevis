// ct_lvtqtw_graphtabelement.h                               -*-C++-*-

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

#ifndef DEFINED_CT_LVTQTW_GRAPHTABELEMENT_H
#define DEFINED_CT_LVTQTW_GRAPHTABELEMENT_H

#include <lvtqtw_export.h>

#include <ct_lvtldr_lakosiannode.h>
#include <ct_lvtplg_pluginmanager.h>

#include <ct_lvtshr_graphenums.h>

#include <kmessagewidget.h>

#include <QWidget>
#include <memory>

class QString;
class CodeVisApplicationTestFixture;

namespace Ui {
class GraphTabElement;
}
namespace Codethink::lvtqtc {
class GraphicsView;
class ITool;
} // namespace Codethink::lvtqtc
namespace Codethink::lvtldr {
class NodeStorage;
}
namespace Codethink::lvtprj {
class ProjectFile;
}

namespace Codethink::lvtqtw {

class LVTQTW_EXPORT GraphTabElement : public QWidget {
    Q_OBJECT
  public:
    friend class ::CodeVisApplicationTestFixture;

    // enum for backwards / forward navigation.
    enum HistoryType {
        NoHistory, // don't record on history
        History // record on history
    };

    GraphTabElement(Codethink::lvtldr::NodeStorage& nodeStorage,
                    lvtprj::ProjectFile const& projectFile,
                    QWidget *parent);
    ~GraphTabElement() override;

    bool setCurrentGraph(const QString& fullyQualifiedName, HistoryType historyType, lvtshr::DiagramType diagramType);

    void setCurrentDiagramFromHistory(int idx);
    // connected with the currentIndexChanged from the History Model.

    [[nodiscard]] lvtqtc::GraphicsView *graphicsView() const;

    Q_SIGNAL void historyUpdate(const QString& fullyQualifiedName, lvtshr::DiagramType type);
    // triggered when we change the displayed graph due to one of the history
    // buttons

    Q_SIGNAL void sendMessage(const QString& message, KMessageWidget::MessageType type);

    void toggleFilterVisibility();

    void setPluginManager(Codethink::lvtplg::PluginManager& pm);

  protected:
    void resizeEvent(QResizeEvent *ev) override;

  private:
    void setupToolBar(Codethink::lvtldr::NodeStorage& nodeStorage);
    std::vector<lvtqtc::ITool *> tools() const;

    std::unique_ptr<Ui::GraphTabElement> ui;
    struct Private;
    std::unique_ptr<Private> d;
};

} // namespace Codethink::lvtqtw
#endif
