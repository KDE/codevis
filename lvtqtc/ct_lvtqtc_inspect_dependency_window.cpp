// ct_lvtqtc_inspect_dependency_window.cpp                               -*-C++-*-

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

#include <ct_lvtprj_projectfile.h>
#include <ct_lvtqtc_inspect_dependency_window.h>

#include <QDebug>
#include <QDesktopServices>
#include <QDialog>
#include <QFileInfo>
#include <QGraphicsScene>
#include <QHeaderView>
#include <QLabel>
#include <QLayout>
#include <QLine>
#include <QMessageBox>
#include <QUrl>

Q_DECLARE_METATYPE(Codethink::lvtldr::LakosianNode *)

namespace Codethink::lvtqtc {

InspectDependencyWindow::InspectDependencyWindow(lvtprj::ProjectFile const& projectFile,
                                                 LakosRelation const& r,
                                                 QDialog *parent):
    QDialog(parent), d_lakosRelation(r), d_projectFile(projectFile)
{
    setWindowModality(Qt::ApplicationModal);
    setWindowTitle(tr("Dependency inspector"));
    setMinimumWidth(600);
    setMinimumHeight(400);

    setupUi();

    d_contentsTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(d_contentsTable, &QTableWidget::customContextMenuRequested, this, [this](const QPoint& pos) {
        QTableWidgetItem *item = d_contentsTable->itemAt(pos);
        if (!item) {
            return;
        }

        auto *menu = item->data(MenuRole).value<QMenu *>();
        if (!menu) {
            return;
        }

        menu->exec(pos);
    });
    populateContentsTable();
}

void InspectDependencyWindow::setupUi()
{
    auto *from = d_lakosRelation.from();
    auto *to = d_lakosRelation.to();
    auto *layout = new QVBoxLayout{this};
    auto *titleLabel = new QLabel{this};
    titleLabel->setText(tr("Inspect dependencies from %1 to %2")
                            .arg(QString::fromStdString(from->name()), QString::fromStdString(to->name())));
    titleLabel->setStyleSheet("font-weight: bold;");

    layout->addWidget(titleLabel);
    d_contentsTable = new QTableWidget{this};
    d_contentsTable->setColumnCount(2);
    d_contentsTable->setHorizontalHeaderLabels({tr("From component"), tr("To component")});

    auto *header = d_contentsTable->horizontalHeader();
    header->setMinimumSectionSize(10);
    header->setSectionResizeMode(0, QHeaderView::Stretch);
    header->setSectionResizeMode(1, QHeaderView::Stretch);
    layout->addWidget(d_contentsTable);

    // make the scrollbar touch the window frame on the right.
    layout->setSpacing(0);
    layout->setContentsMargins(6, 6, 0, 6);
    setLayout(layout);
}

void InspectDependencyWindow::openFile(lvtldr::LakosianNode *pkg, lvtldr::LakosianNode *component, const QString& ext)
{
    auto prefix = d_projectFile.sourceCodePath();
    auto filepath = [&]() {
        if (pkg->parent()) {
            return QString::fromStdString(
                       (prefix / "groups" / pkg->parent()->name() / pkg->name() / component->name()).string())
                + ext;
        }
        return QString::fromStdString((prefix / "standalones" / pkg->name() / component->name()).string()) + ext;
    }();

    if (!QFileInfo::exists(filepath) || !QDesktopServices::openUrl(QUrl::fromLocalFile(filepath))) {
        showFileNotFoundWarning(filepath);
    }
}

void InspectDependencyWindow::showFileNotFoundWarning(QString const& filepath) const
{
    QMessageBox::warning(nullptr, tr("Warning"), tr("File not found: %1").arg(filepath));
}

void InspectDependencyWindow::populateContentsTable()
{
    auto *from = d_lakosRelation.from();
    auto *to = d_lakosRelation.to();

    auto createContentActionButton = [&](lvtldr::LakosianNode *fromChild) -> QMenu * {
        auto *menu = new QMenu();
        auto *open_source = menu->addAction(tr("Open source"));
        connect(open_source, &QAction::triggered, this, [this, from, fromChild] {
            openFile(from->internalNode(), fromChild, ".cpp");
        });

        auto *open_header = menu->addAction(tr("Open header"));
        connect(open_header, &QAction::triggered, this, [this, from, fromChild] {
            openFile(from->internalNode(), fromChild, ".h");
        });

        auto *open_test = menu->addAction(tr("Open test"));
        connect(open_test, &QAction::triggered, this, [this, from, fromChild] {
            openFile(from->internalNode(), fromChild, ".t.cpp");
        });
        return menu;
    };

    for (auto const& fromChild : from->internalNode()->children()) {
        for (auto const& dep : fromChild->providers()) {
            if (dep.other()->parent() == to->internalNode()) {
                auto const& toChild = dep.other();
                auto *fromItem = new QTableWidgetItem();
                fromItem->setText(QString::fromStdString(fromChild->name()));
                fromItem->setFlags(Qt::ItemFlag::ItemIsEnabled);
                auto *toItem = new QTableWidgetItem();
                toItem->setText(QString::fromStdString(toChild->name()));
                toItem->setFlags(Qt::ItemFlag::ItemIsEnabled);

                auto i = d_contentsTable->rowCount();
                d_contentsTable->insertRow(i);
                d_contentsTable->setItem(i, 0, fromItem);
                d_contentsTable->setItem(i, 1, toItem);
                fromItem->setData(MenuRole, QVariant::fromValue(createContentActionButton(fromChild)));
            }
        }
    }
}

} // namespace Codethink::lvtqtc
