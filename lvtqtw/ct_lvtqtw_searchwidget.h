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

#ifndef INCLUDED_LVTQTW_SEARCHWIDGET
#define INCLUDED_LVTQTW_SEARCHWIDGET

#include <lvtqtw_export.h>

#include <ct_lvtshr_graphenums.h>

#include <QWidget>

#include <memory>

class QHideEvent;

namespace Ui {
class SearchWidget;
}

namespace Codethink::lvtqtw {

class LVTQTW_EXPORT SearchWidget : public QWidget {
    Q_OBJECT
  public:
    explicit SearchWidget(QWidget *parent = nullptr);
    ~SearchWidget() override;

    Q_SIGNAL void searchModeChanged(lvtshr::SearchMode mode);
    Q_SIGNAL void searchStringChanged(const QString& search);
    Q_SIGNAL void requestNextElement();
    Q_SIGNAL void requestPreviousElement();

    Q_SLOT void setNumberOfMatchedItems(int nr);
    Q_SLOT void setCurrentItem(int nr);

  protected:
    bool eventFilter(QObject *object, QEvent *event) override;
    void hideEvent(QHideEvent *ev) override;
    void showEvent(QShowEvent *ev) override;

  private:
    void calculateNrOfElementsLabel();

    struct Private;
    std::unique_ptr<Private> d;
    std::unique_ptr<Ui::SearchWidget> ui;
};

} // namespace Codethink::lvtqtw

#endif
