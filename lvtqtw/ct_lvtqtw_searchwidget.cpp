// ct_lvtqtw_searchwidget.h                                            -*-C++-*-

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

#include <ct_lvtqtw_searchwidget.h>
#include <ui_ct_lvtqtw_searchwidget.h>

#include <QAction>
#include <QActionGroup>
#include <QKeyEvent>

using namespace Codethink::lvtqtw;

struct SearchWidget::Private {
    int elementsFound = 0;
    int currentElement = 0;

    QToolButton *lastBtn = nullptr;
};

SearchWidget::SearchWidget(QWidget *parent):
    QWidget(parent), d(std::make_unique<SearchWidget::Private>()), ui(std::make_unique<Ui::SearchWidget>())
{
    ui->setupUi(this);
    connect(ui->btnNext, &QToolButton::clicked, this, [this] {
        d->lastBtn = ui->btnNext;
        Q_EMIT requestNextElement();
    });

    connect(ui->btnPrevious, &QToolButton::clicked, this, [this] {
        d->lastBtn = ui->btnPrevious;
        Q_EMIT requestPreviousElement();
    });

    auto *actions = new QActionGroup(this);
    actions->setExclusive(true);

    auto *ignoreCaseAction = new QAction(tr("Ignore Case"));
    auto *matchCaseAction = new QAction(tr("Match Case"));
    auto *regularExpression = new QAction(tr("Regular Expression"));

    for (const auto& [action, enum_] : std::initializer_list<std::pair<QAction *, lvtshr::SearchMode>>{
             {ignoreCaseAction, lvtshr::SearchMode::CaseInsensitive},
             {matchCaseAction, lvtshr::SearchMode::CaseSensitive},
             {regularExpression, lvtshr::SearchMode::RegularExpressions}}) {
        action->setCheckable(true);
        actions->addAction(action);
        ui->btnConfig->addAction(action);
        connect(action, &QAction::triggered, this, [this, action = action, enum_ = enum_](bool toggled) {
            if (action->isChecked()) {
                Q_EMIT searchModeChanged(enum_);
            }
        });
    }
    ignoreCaseAction->setChecked(true);
    d->lastBtn = ui->btnNext;

    connect(ui->searchLine, &QLineEdit::textChanged, this, &SearchWidget::searchStringChanged);
    calculateNrOfElementsLabel();
    ui->searchLine->installEventFilter(this);
}

SearchWidget::~SearchWidget() = default;

bool SearchWidget::eventFilter(QObject *object, QEvent *event)
{
    Q_UNUSED(object);
    if (event->type() == QEvent::KeyPress) {
        auto *ev = static_cast<QKeyEvent *>(event); // NOLINT
        if (ev->key() == Qt::Key_Return) {
            d->lastBtn->click();
        }
        if (ev->key() == Qt::Key_Escape) {
            setVisible(false);
        }
    }
    return false;
}

void SearchWidget::setNumberOfMatchedItems(int nr)
{
    d->elementsFound = nr;
    d->currentElement = 0;
    calculateNrOfElementsLabel();
}

void SearchWidget::setCurrentItem(int nr)
{
    d->currentElement = nr;
    calculateNrOfElementsLabel();
}

void SearchWidget::calculateNrOfElementsLabel()
{
    const QString text =
        tr("%1 out of %2 ocurrences").arg(d->elementsFound == 0 ? 0 : d->currentElement).arg(d->elementsFound);

    ui->lblFound->setText(text);
    ui->btnNext->setEnabled(d->elementsFound != 0);
    ui->btnPrevious->setEnabled(d->elementsFound != 0);
}

void SearchWidget::hideEvent(QHideEvent *ev)
{
    Q_UNUSED(ev);
    ui->searchLine->setText(QString());
    setNumberOfMatchedItems(0);
    setCurrentItem(0);
}

void SearchWidget::showEvent(QShowEvent *ev)
{
    Q_UNUSED(ev);
    ui->searchLine->setFocus();
}

#include "moc_ct_lvtqtw_searchwidget.cpp"
