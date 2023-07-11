// ct_lvtqtw_itool.cpp                                               -*-C++-*-

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

#include <ct_lvtqtc_itool.h>

#include <ct_lvtqtc_graphicsscene.h>
#include <ct_lvtqtc_graphicsview.h>

#include <QAction>
#include <QDebug>
#include <QEvent>
#include <QGraphicsSceneMouseEvent>
#include <QKeyEvent>
#include <QToolButton>

// in one source file
Q_LOGGING_CATEGORY(LogTool, "log.itool")

namespace Codethink::lvtqtc {
struct ITool::Private {
    QString name;
    QString tooltip;
    QIcon icon;
    GraphicsView *graphicsView = nullptr;
    QAction *action = nullptr;
    lvtldr::NodeStorage *nodeStorage = nullptr;
};

ITool::ITool(const QString& name, const QString& tooltip, const QIcon& icon, GraphicsView *gv):
    d(std::make_unique<ITool::Private>())
{
    d->name = name;
    d->tooltip = tooltip;
    d->icon = icon;
    d->graphicsView = gv;

    d->action = new QAction();
    d->action->setIcon(d->icon);
    d->action->setText(d->name);
    d->action->setToolTip(d->tooltip);
    d->action->setCheckable(true);

    connect(d->action, &QAction::toggled, this, [this](bool value) {
        value ? activate() : deactivate();

        // this won't re-trigger the toggled signal, and it's needed
        // to actually select the button on click, because of the
        // invisible button on the button group being selected after
        // one is deactivated. see this example:
        // btn1 is active, we click in btn2.
        // btn2 get's active, this disables btn1
        // because btn1 got deactivated, invisibleButton will get active, deactivating btn2
        // so we need to check the 2nd button twice.
        d->action->setChecked(value);
    });

    connect(this, &lvtqtc::ITool::undoCommandCreated, d->graphicsView, &lvtqtc::GraphicsView::undoCommandReceived);
}

ITool::~ITool() noexcept
{
    d->action->deleteLater();
}

QAction *ITool::action() const
{
    return d->action;
}

QString ITool::name() const
{
    return d->name;
}

QString ITool::toolTip() const
{
    return d->tooltip;
}

QIcon ITool::icon() const
{
    return d->icon;
}

void ITool::setEnabled(bool enabled)
{
    d->action->setEnabled(enabled);
}

bool ITool::eventFilter(QObject *obj, QEvent *ev)
{
    if (ev->type() == QEvent::KeyPress) {
        auto *keyEv = static_cast<QKeyEvent *>(ev); // NOLINT

        // If the user manually cancels the action by pressing esc, also clean the message message.
        if (keyEv->key() == Qt::Key_Escape) {
            d->action->setChecked(false);
            Q_EMIT sendMessage(QString(), KMessageWidget::Information);
        }
    }

    if (obj == d->graphicsView) {
        switch (ev->type()) {
        case QEvent::KeyRelease:
            keyReleaseEvent(static_cast<QKeyEvent *>(ev)); // NOLINT
            return ev->isAccepted();
        case QEvent::KeyPress:
            keyPressEvent(static_cast<QKeyEvent *>(ev)); // NOLINT
            return ev->isAccepted();
        default:
            return false;
        }
        return false;
    }

    if (obj == d->graphicsView->viewport()) {
        switch (ev->type()) {
        case QEvent::MouseMove:
            mouseMoveEvent(static_cast<QMouseEvent *>(ev)); // NOLINT
            return ev->isAccepted();
        case QEvent::MouseButtonPress:
            mousePressEvent(static_cast<QMouseEvent *>(ev)); // NOLINT
            return ev->isAccepted();
        case QEvent::MouseButtonRelease:
            mouseReleaseEvent(static_cast<QMouseEvent *>(ev)); // NOLINT
            return ev->isAccepted();
        default:
            return false;
        }
        return true;
    }

    if (obj == d->graphicsView->scene()) {
        switch (ev->type()) {
        case QEvent::GraphicsSceneMouseMove:
            sceneMouseMoveEvent(static_cast<QGraphicsSceneMouseEvent *>(ev)); // NOLINT
            return ev->isAccepted();
        case QEvent::GraphicsSceneMousePress:
            sceneMousePressEvent(static_cast<QGraphicsSceneMouseEvent *>(ev)); // NOLINT
            return ev->isAccepted();
        case QEvent::GraphicsSceneMouseRelease:
            sceneMouseReleaseEvent(static_cast<QGraphicsSceneMouseEvent *>(ev)); // NOLINT
            return ev->isAccepted();
        default:
            return false;
        }
        return true;
    }
    return false;
}

GraphicsView *ITool::graphicsView() const
{
    return d->graphicsView;
}

GraphicsScene *ITool::graphicsScene() const
{
    return qobject_cast<GraphicsScene *>(d->graphicsView->scene());
}

void ITool::setupInfra(bool install)
{
    if (install) {
        d->graphicsView->installEventFilter(this);
        d->graphicsView->viewport()->installEventFilter(this);
        d->graphicsView->scene()->installEventFilter(this);
        d->graphicsView->setCurrentTool(this);
        d->graphicsView->setFocus(Qt::OtherFocusReason);
    } else {
        d->graphicsView->removeEventFilter(this);
        d->graphicsView->viewport()->removeEventFilter(this);
        d->graphicsView->scene()->removeEventFilter(this);
        d->graphicsView->setCurrentTool(nullptr);
    }
}

void ITool::activate()
{
    qCDebug(LogTool) << name() << "Activated";
    // because of the double selection needed - for the invisibleButton
    // we need to, before activating anything, making sure this anything
    // is not already connected to the view.
    setupInfra(false);
    setupInfra(true);
    Q_EMIT activated();
}

void ITool::deactivate()
{
    qCDebug(LogTool) << name() << "Deactivate";
    setupInfra(false);
    Q_EMIT deactivated();
}

void ITool::mousePressEvent(QMouseEvent *event)
{
    qCDebug(LogTool) << name() << "Mouse Press Event ignored";
    event->setAccepted(false);
    Q_UNUSED(event);
}

void ITool::mouseMoveEvent(QMouseEvent *event)
{
    event->setAccepted(false);
    Q_UNUSED(event);
}

void ITool::mouseReleaseEvent(QMouseEvent *event)
{
    qCDebug(LogTool) << name() << "Mouse Press Release ignored";
    event->setAccepted(false);
    Q_UNUSED(event);
}

void ITool::keyPressEvent(QKeyEvent *event)
{
    qCDebug(LogTool) << name() << "Key Press Event ignored";
    event->setAccepted(false);
    Q_UNUSED(event);
}

void ITool::keyReleaseEvent(QKeyEvent *event)
{
    qCDebug(LogTool) << name() << "Key Release ignored";
    event->setAccepted(false);
    Q_UNUSED(event);
}

void ITool::drawForeground(QPainter *painter, const QRectF& rect)
{
    Q_UNUSED(painter);
    Q_UNUSED(rect);
}

// those acts on the scene
void ITool::sceneMousePressEvent(QGraphicsSceneMouseEvent *event)
{
    qCDebug(LogTool) << name() << "Scene Mouse Press Event Ignored";
    event->setAccepted(false);
    Q_UNUSED(event);
}

void ITool::sceneMouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    event->setAccepted(false);
    Q_UNUSED(event);
}

void ITool::sceneMouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    qCDebug(LogTool) << name() << "Scene Mouse Release Event Ignored";
    event->setAccepted(false);
    Q_UNUSED(event);
}

} // namespace Codethink::lvtqtc
