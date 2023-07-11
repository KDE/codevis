// ct_lvtqtw_errorview.cpp                                                              -*-C++-*-

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
#include <ct_lvtqtw_errorview.h>

#include <ui_ct_lvtqtw_errorview.h>

#include <ct_lvtmdl_errorsmodel.h>
#include <ct_lvtmdl_simpletextmodel.h>

#include <ct_lvtqtd_removedelegate.h>

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QMenu>
#include <QSettings>

namespace Codethink::lvtqtw {

struct ErrorView::Private {
    Codethink::lvtmdl::ErrorModelFilter filterModel;
    Codethink::lvtmdl::SimpleTextModel ignoreGroupModel;

    // We don't own the debug model, it should be created before the mainwindow
    // to not lose debug information. We only hold the pointer to it. it should
    // always be valid, after being set, untill the destruction of this widget.
    Codethink::lvtmdl::ErrorsModel *model = nullptr;

    // TODO: Maybe move this to a QComboBox class
    QString currentEditedTextOnCategoryCombobox;
};

ErrorView::ErrorView(QWidget *parent):
    QWidget(parent), ui(std::make_unique<Ui::ErrorView>()), d(std::make_unique<ErrorView::Private>())
{
    ui->setupUi(this);

    ui->tableView->setModel(&d->filterModel);
    ui->tableView->horizontalHeader()->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);

    ui->comboBox->setModel(&d->ignoreGroupModel);

    auto *delegate = new lvtqtd::RemoveDelegate();
    ui->comboBox->view()->setMouseTracking(true);
    ui->comboBox->view()->setItemDelegate(delegate);
    ui->comboBox->setInsertPolicy(QComboBox::InsertAtBottom);
    ui->comboBox->lineEdit()->setPlaceholderText(tr("Ignored Categories"));

    d->ignoreGroupModel.setStorageGroup("DebugIgnoreGroups");

    d->filterModel.setFilterCompilerMessages(ui->btnCompilerMessages->isChecked());
    d->filterModel.setFilterParseMessages(ui->btnParseMessages->isChecked());
    d->filterModel.setIgnoreCategoriesModel(&d->ignoreGroupModel);

    connect(ui->btnCompilerMessages,
            &QToolButton::toggled,
            &d->filterModel,
            &Codethink::lvtmdl::ErrorModelFilter::setFilterCompilerMessages);
    connect(ui->btnParseMessages,
            &QToolButton::toggled,
            &d->filterModel,
            &Codethink::lvtmdl::ErrorModelFilter::setFilterParseMessages);
    connect(ui->tableView->horizontalHeader(),
            &QWidget::customContextMenuRequested,
            this,
            &ErrorView::createColumnVisibilityMenu);
    connect(ui->filterLine, &QLineEdit::textChanged, &d->filterModel, &lvtmdl::ErrorModelFilter::setFilterString);
    connect(ui->invertFilterBtn, &QCheckBox::stateChanged, this, [this] {
        d->filterModel.setInvertMessageFilter(ui->invertFilterBtn->isChecked());
    });

    // The QComboBox loses the text as soon as there's a editingFinished, so we need to store the string
    // being edited, to use it later on.
    connect(ui->comboBox->lineEdit(), &QLineEdit::textEdited, this, [this] {
        d->currentEditedTextOnCategoryCombobox = ui->comboBox->lineEdit()->text();
    });

    connect(ui->comboBox->lineEdit(), &QLineEdit::editingFinished, this, [this] {
        if (!d->currentEditedTextOnCategoryCombobox.isEmpty()) {
            d->ignoreGroupModel.addString(d->currentEditedTextOnCategoryCombobox);
        }
        d->currentEditedTextOnCategoryCombobox = QString();
    });

    ui->tableView->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->btnCompilerMessages->setIcon(QIcon(":/icons/critical"));
    ui->btnParseMessages->setIcon(QIcon(":/icons/info"));

    auto *copyAction = new QAction();
    copyAction->setShortcut(QKeySequence::Copy);
    copyAction->setShortcutContext(Qt::ShortcutContext::WidgetWithChildrenShortcut);
    addAction(copyAction);
    connect(copyAction, &QAction::triggered, this, [this] {
        auto rows = ui->tableView->selectionModel()->selectedRows();

        QString result;
        for (const QModelIndex& idx : rows) {
            result += idx.data(Qt::ToolTipRole).toString();
        }

        qApp->clipboard()->setText(result);
    });
}

ErrorView::~ErrorView()
{
    saveColumnVisibilityState();
}

void ErrorView::setModel(lvtmdl::ErrorsModel *model)
{
    if (d->model) {
        disconnect(d->model, &QAbstractItemModel::rowsRemoved, ui->tableView, nullptr);
        disconnect(d->model, &QAbstractItemModel::rowsInserted, ui->tableView, nullptr);
    }

    d->filterModel.setSourceModel(model);
    d->model = model;

    connect(d->model, &QAbstractItemModel::rowsRemoved, ui->tableView, &QTableView::scrollToBottom);
    connect(d->model, &QAbstractItemModel::rowsInserted, ui->tableView, &QTableView::scrollToBottom);

    loadColumnVisibilityState();
}

void ErrorView::saveColumnVisibilityState()
{
    QSettings settings;
    settings.beginGroup("DebugTable");

    const int columnCount = ui->tableView->model()->columnCount();
    for (int i = 0; i < columnCount; i++) {
        settings.setValue(QStringLiteral("column_%1_visible").arg(i), !ui->tableView->isColumnHidden(i));
    }
}

void ErrorView::loadColumnVisibilityState()
{
    QSettings settings;
    settings.beginGroup("DebugTable");

    const int columnCount = ui->tableView->model()->columnCount();
    for (int i = 0; i < columnCount; i++) {
        const bool visible = settings.value(QStringLiteral("column_%1_visible").arg(i), true).toBool();
        ui->tableView->setColumnHidden(i, !visible);
    }
}

void ErrorView::createColumnVisibilityMenu(const QPoint& pos)
{
    QMenu menu;
    const int columnCount = ui->tableView->model()->columnCount();
    for (int i = 0; i < columnCount; i++) {
        const QString textAt = ui->tableView->model()->headerData(i, Qt::Orientation::Horizontal).toString();
        QAction *toggleVisibility = menu.addAction(textAt);
        toggleVisibility->setCheckable(true);
        toggleVisibility->setChecked(!ui->tableView->isColumnHidden(i));
        connect(toggleVisibility, &QAction::toggled, this, [this, i] {
            ui->tableView->setColumnHidden(i, !ui->tableView->isColumnHidden(i));
        });
    }
    menu.exec(ui->tableView->horizontalHeader()->mapToGlobal(pos));
}

} // namespace Codethink::lvtqtw
