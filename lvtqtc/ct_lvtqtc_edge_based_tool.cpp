// ct_lvtqtc_edge_based_tool.cpp                                                                               -*-C++-*-

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

#include <ct_lvtldr_lakosiannode.h>
#include <ct_lvtqtc_edge_based_tool.h>
#include <ct_lvtqtc_graphicsview.h>
#include <ct_lvtqtc_lakosentity.h>
#include <ct_lvtshr_functional.h>

#include <QDebug>
#include <QGraphicsLineItem>

namespace Codethink::lvtqtc {

struct EdgeBasedTool::Private {
    LakosEntity *fromItem = nullptr;
    LakosEntity *toItem = nullptr;
};

EdgeBasedTool::EdgeBasedTool(const QString& name, const QString& tooltip, const QIcon& icon, GraphicsView *gv):
    ITool(name, tooltip, icon, gv), d(std::make_unique<Private>())
{
}

EdgeBasedTool::~EdgeBasedTool() = default;

void EdgeBasedTool::activate()
{
    Q_EMIT sendMessage(tr("Select the source Element"), KMessageWidget::Information);
    ITool::activate();
}

void EdgeBasedTool::mousePressEvent(QMouseEvent *event)
{
    if (event->modifiers()) {
        event->setAccepted(false);
    }
    const auto qItems = graphicsView()->itemsByType<LakosEntity>(event->pos());
    if (qItems.empty()) {
        event->setAccepted(false);
    }
}

void EdgeBasedTool::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    const auto qItems = graphicsView()->itemsByType<LakosEntity>(event->pos());
    if (qItems.empty()) {
        event->setAccepted(false);
        return;
    }

    if (!d->fromItem) {
        d->fromItem = qItems.at(0);
        Q_EMIT sendMessage(tr("Source element: <strong>%1</strong>, Select the target Element")
                               .arg(QString::fromStdString(d->fromItem->name())),
                           KMessageWidget::Information);
        return;
    }

    d->toItem = qItems.at(0);
    if (!d->toItem) {
        return;
    }

    if (d->toItem->hasRelationshipWith(d->fromItem) || d->fromItem->hasRelationshipWith(d->toItem)) {
        Q_EMIT sendMessage(
            tr("You can't connect %1 to %2, they already have a connection.")
                .arg(QString::fromStdString(d->fromItem->name()), QString::fromStdString(d->toItem->name())),
            KMessageWidget::Error);
        deactivate();
        return;
    }

    if (d->fromItem && d->toItem) {
        if (run(d->fromItem, d->toItem)) {
            Q_EMIT sendMessage(QString(), KMessageWidget::Information);
        }
        deactivate();
    }
}

void EdgeBasedTool::deactivate()
{
    d->fromItem = nullptr;
    d->toItem = nullptr;

    ITool::deactivate();
}

std::vector<std::pair<LakosEntity *, LakosEntity *>> EdgeBasedTool::calculateHierarchy(LakosEntity *source,
                                                                                       LakosEntity *target)
{
    qCDebug(LogTool) << "Calculating hierarchy for" << QString::fromStdString(source->name()) << "and"
                     << QString::fromStdString(target->name());
    std::vector<std::pair<LakosEntity *, LakosEntity *>> ret;
    while (source->parentItem() && target->parentItem()) {
        source = qgraphicsitem_cast<LakosEntity *>(source->parentItem());
        target = qgraphicsitem_cast<LakosEntity *>(target->parentItem());

        const auto& iSource = source->internalNode();
        const auto& iTarget = target->internalNode();

        if (iSource == iTarget) {
            // Early return if found a common ancestor
            break;
        }

        if (!iSource->hasProvider(iTarget)) {
            ret.emplace_back(std::make_pair(source, target));
        }
    }

    // we need to add the items in order, first to the toplevel element
    // that lacks a connection, to the inner element.
    std::reverse(ret.begin(), ret.end());

    return ret;
}
} // namespace Codethink::lvtqtc
