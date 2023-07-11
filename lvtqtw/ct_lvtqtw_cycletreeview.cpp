// ct_lvtqtw_cycletreeview.cpp                               -*-C++-*-

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

#include <ct_lvtmdl_circular_relationships_model.h>
#include <ct_lvtqtw_backgroundeventfilter.h>
#include <ct_lvtqtw_cycletreeview.h>

#include <QDebug>

namespace Codethink::lvtqtw {

struct CycleTreeView::Private {
    BackgroundEventFilter bgFilter;
    QString emptyState;
    QString initialState;
    lvtmdl::CircularRelationshipsModel *model = nullptr;
};

CycleTreeView::CycleTreeView(QWidget *parent): QTreeView(parent), d(std::make_unique<CycleTreeView::Private>())
{
    viewport()->installEventFilter(&d->bgFilter);
    setStringForEmptyState(tr("We found no cycles during search"));
    setStringForInitialState(tr("Search for cycles"));
    setInitialState();
    setHeaderHidden(true);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
}

CycleTreeView::~CycleTreeView() = default;

void CycleTreeView::setStringForEmptyState(const QString& emptyState)
{
    d->emptyState = emptyState;
    update();
}

void CycleTreeView::setStringForInitialState(const QString& initialState)
{
    d->initialState = initialState;
    update();
}

void CycleTreeView::setInitialState()
{
    d->bgFilter.setBackgroundText(d->initialState);
    viewport()->update();
    update();
}

void CycleTreeView::setEmptyState()
{
    d->bgFilter.setBackgroundText(d->emptyState);
    viewport()->update();
    update();
}

void CycleTreeView::setModel(QAbstractItemModel *model)
{
    if (d->model) {
        disconnect(d->model, nullptr, this, nullptr);
    }
    d->model = dynamic_cast<lvtmdl::CircularRelationshipsModel *>(model);
    if (!d->model) {
        return;
    }

    connect(d->model, &lvtmdl::CircularRelationshipsModel::emptyState, this, &CycleTreeView::setEmptyState);
    connect(d->model, &lvtmdl::CircularRelationshipsModel::initialState, this, &CycleTreeView::setInitialState);
    QTreeView::setModel(model);
}

} // namespace Codethink::lvtqtw
