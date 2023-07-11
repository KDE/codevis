// ct_lvtqtw_itool.h                                                 -*-C++-*-

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

#ifndef INCLUDED_LVTQTC_ITOOL
#define INCLUDED_LVTQTC_ITOOL

#include <lvtqtc_export.h>

#include <QGraphicsView>
#include <QIcon>
#include <QLoggingCategory>
#include <QObject>
#include <QUndoCommand>

#include <kmessagewidget.h>

#include <memory>

Q_DECLARE_LOGGING_CATEGORY(LogTool)

class QMouseEvent;
class QGraphicsSceneMouseEvent;
class QToolButton;

namespace Codethink::lvtldr {
class NodeStorage;
}

namespace Codethink::lvtqtc {
class GraphicsView;
class GraphicsScene;

class LVTQTC_EXPORT ITool : public QObject {
    /* ITool. Interface for Tools that acts on the canvas.
     * An active tool has access to mouse and keyboard presses, and can act directly on
     * the view (the QGraphicsView) or the scene (QGraphicsScene) adding items,
     * triggering state changes, moving things around, etc.
     */
    Q_OBJECT

  public:
    ~ITool() noexcept override;

    [[nodiscard]] QAction *action() const;
    [[nodiscard]] QString name() const;
    [[nodiscard]] QString toolTip() const;
    [[nodiscard]] QIcon icon() const;

    void setEnabled(bool enabled);

    virtual void activate();
    // sets this tool as the active one.

    virtual void deactivate();
    // deactivates the tool.

    // Those acts on the View.
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
    virtual void drawForeground(QPainter *painter, const QRectF& rect);

    // those acts on the scene
    virtual void sceneMousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void sceneMouseMoveEvent(QGraphicsSceneMouseEvent *event);
    virtual void sceneMouseReleaseEvent(QGraphicsSceneMouseEvent *event);

    virtual void keyPressEvent(QKeyEvent *event);
    virtual void keyReleaseEvent(QKeyEvent *event);

    bool eventFilter(QObject *obj, QEvent *ev) override;

    Q_SIGNAL void activated();
    Q_SIGNAL void deactivated();
    Q_SIGNAL void undoCommandCreated(QUndoCommand *undoCommand);
    Q_SIGNAL void sendMessage(const QString& message, KMessageWidget::MessageType type);

  protected:
    ITool(const QString& name, const QString& tooltip, const QIcon& icon, GraphicsView *gv);

    [[nodiscard]] GraphicsView *graphicsView() const;
    [[nodiscard]] GraphicsScene *graphicsScene() const;

  private:
    void setupInfra(bool install);
    struct Private;
    std::unique_ptr<Private> d;
};

} // namespace Codethink::lvtqtc

#endif
