// ct_lvtqtw_graphicsview.h                                     -*-C++-*-

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

#ifndef INCLUDED_LVTQTC_GRAPHICSVIEW
#define INCLUDED_LVTQTC_GRAPHICSVIEW

#include <lvtqtc_export.h>

#include <ct_lvtqtc_undo_manager.h>

#include <ct_lvtplg_pluginmanager.h>
#include <ct_lvtqtc_itool.h>
#include <ct_lvtshr_graphenums.h>

#include <QGraphicsView>
#include <QUndoCommand>

#include <memory>
#include <qevent.h>

class CodeVisApplicationTestFixture;
namespace Codethink::lvtclr {
class ColorManagement;
}
namespace Codethink::lvtldr {
class NodeStorage;
}
namespace Codethink::lvtprj {
class ProjectFile;
}

namespace Codethink::lvtqtc {

class LVTQTC_EXPORT GraphicsView : public QGraphicsView
// Visualizes a Tree model and reacts to changes
{
    Q_OBJECT
  public:
    friend class ::CodeVisApplicationTestFixture;

    // CREATORS
    explicit GraphicsView(Codethink::lvtldr::NodeStorage& nodeStorage,
                          lvtprj::ProjectFile const& projectFile,
                          QWidget *parent = nullptr);
    // Constructor

    ~GraphicsView() noexcept override;
    // Destructor

    // METHODS
    void setColorManagement(const std::shared_ptr<lvtclr::ColorManagement>& colorManagement);

    void setUndoManager(UndoManager *undoManager);

    bool updateClassGraph(const QString& fullyQualifiedClassName);
    // loads and displays the graph represented by the fully qualified class name

    bool updatePackageGraph(const QString& fullyQualifiedPackageName);
    // loads and displays the package represented by the fully qualified package name

    bool updateComponentGraph(const QString& fullyQualifiedComponentName);
    // loads and displays the package represented by the fully qualified component name

    bool updateGraph(const QString& fullyQualifiedName, lvtshr::DiagramType type);

    Q_SIGNAL void zoomFactorChanged(int zoomFactor);
    // The current zoom factor, emitted when the internal value changes

    Q_SIGNAL void visibleAreaChanged(QRectF rect);
    // we moved or zoomed in, the area changed.

    [[nodiscard]] int zoomFactor() const;

    void setZoomFactor(int zoomFactorInPercent);
    // set's the zoom factor in percents

    void calculateCurrentZoomFactor();
    // when we load a graph, we zoom out or zoom in, to fit the entire graph on screen
    // so we need to calculate how much this affected the zoom.

    void zoomIntoRect(const QPoint& topLeft, const QPoint& bottomRight);

    void fitAllInView();
    // Adjust zoom so that the whole scene is visible

    void fitMainEntityInView();
    // Adjust zoom so that the main entity is visible

    void setPluginManager(Codethink::lvtplg::PluginManager& pm);

    Q_SLOT void toggleMinimap(bool toggle);
    // Shows / Hides the minimap widget

    Q_SLOT void toggleLegend(bool toggle);
    // shows / hides the legend widget

    Q_SLOT void undoCommandReceived(QUndoCommand *command);

    Q_SIGNAL void packageNavigateRequested(const QString& qualifiedName);
    Q_SIGNAL void componentNavigateRequested(const QString& qualifiedName);
    Q_SIGNAL void classNavigateRequested(const QString& qualifiedName);

    Q_SIGNAL void graphLoadStarted();
    Q_SIGNAL void graphLoadFinished();

    Q_SIGNAL void requestNext();
    Q_SIGNAL void requestPrevious();

    Q_SIGNAL void onUndoCommandReceived(Codethink::lvtqtc::GraphicsView *, QUndoCommand *);

    Q_SIGNAL void errorMessage(const QString& message);

    void debugVisibleScreen();
    [[nodiscard]] lvtshr::DiagramType diagramType() const;

    void setCurrentTool(ITool *tool);
    // sets the tool that's controlling the view.
    // currently this is used just for the drawForeground
    // as the rest I can easily handle via eventFilter.
    // a better approach is to use the event filter for the drawForeground,
    // but there's no event for that, just a paintEvent.

    ITool *currentTool() const;

    // Called by the Search Box.
    Q_SLOT void setSearchMode(lvtshr::SearchMode mode);
    Q_SLOT void setSearchString(const QString& search);
    Q_SLOT void highlightedNextSearchElement();
    Q_SLOT void highlightedPreviousSearchElement();
    Q_SIGNAL void searchTotal(int nr);
    Q_SIGNAL void currentSearchItemHighlighted(int idx);
    void doSearch();

    /* Return all of the items of type T */
    template<typename T>
    QList<T *> allItemsByType() const
    {
        return commonItemsByType<T>(items());
    }

    /* Return all of the items of type T in the specific area */
    template<typename T>
    QList<T *> itemsByType(const QPoint& point) const
    {
        return commonItemsByType<T>(items(point));
    }

    /* Return all of the items of type T */
    template<typename T>
    QList<T *> itemsByType(QRect area, Qt::ItemSelectionMode mode) const
    {
        return commonItemsByType<T>(items(area, mode));
    }

    /* Return the first item of the type T at a specific position */
    template<typename T>
    T *itemByTypeAt(const QPoint& pos) const
    {
        const auto qItems = itemsByType<T>(pos);
        if (qItems.empty()) {
            return nullptr;
        }
        return qItems[0];
    }

  protected:
    void fitRectInView(QRectF const& r);

    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void drawForeground(QPainter *painter, const QRectF& rect) override;
    [[nodiscard]] GraphicsScene *graphicsScene() const;

    template<typename T>
    QList<T *> commonItemsByType(const QList<QGraphicsItem *>& items) const
    {
        QList<T *> byType;
        for (auto *item : items) {
            if (auto itemCast = dynamic_cast<T *>(item)) {
                byType.push_back(itemCast);
            }
        }
        return byType;
    }

  private:
    // DATA TYPES
    struct Private;
    std::unique_ptr<Private> d;
};

} // end namespace Codethink::lvtqtc

#endif
