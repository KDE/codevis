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

#include <QApplication>
#include <QDebug>
#include <QModelIndex>
#include <QMouseEvent>
#include <QPainter>
#include <QStyleOptionViewItem>

#include <ct_lvtmdl_modelhelpers.h>
#include <ct_lvtqtc_iconhelpers.h>
#include <ct_lvtqtd_packageviewdelegate.h>

namespace Codethink::lvtqtd {
struct PackageViewDelegate::Private { };

PackageViewDelegate::PackageViewDelegate(QObject *parent):
    QStyledItemDelegate(parent), d(std::make_unique<PackageViewDelegate::Private>())
{
}

PackageViewDelegate::~PackageViewDelegate() noexcept = default;

void PackageViewDelegate::paint(QPainter *painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyledItemDelegate::paint(painter, option, index);
    if (!index.isValid()) {
        return;
    }

    QRect rect = option.rect.adjusted(1, 1, -1, -1);
    rect.setX(rect.x() + rect.width() - rect.height());
    rect.setWidth(rect.height());

    if (!index.data(lvtmdl::ModelRoles::e_RecursiveLakosian).toBool()) {
        static QIcon warningIcon(":/icons/yellow-warning");
        warningIcon.paint(painter, rect);
    }
}

void PackageViewDelegate::initStyleOption(QStyleOptionViewItem *option, const QModelIndex& index) const
{
    QStyledItemDelegate::initStyleOption(option, index);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    if (auto *v4 = qstyleoption_cast<QStyleOptionViewItemV4 *>(option)) {
        const auto origIcon = index.data(Qt::DecorationRole).value<QIcon>();
        v4->icon = IconHelpers::iconFrom(origIcon);
    }
#else
    if (auto *v4 = qstyleoption_cast<QStyleOptionViewItem *>(option)) {
        const auto origIcon = index.data(Qt::DecorationRole).value<QIcon>();
        v4->icon = IconHelpers::iconFrom(origIcon);
    }
#endif
}

} // namespace Codethink::lvtqtd
