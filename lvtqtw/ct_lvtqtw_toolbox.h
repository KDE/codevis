// ct_lvtqtw_toolbox.h                                            -*-C++-*-

/*
    SPDX-FileCopyrightText: 2007 Vladimir Kuznetsov <ks.vladimir@gmail.com>
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

#ifndef CT_LVTQTW_TOOLBOX_H
#define CT_LVTQTW_TOOLBOX_H

#include <QList>
#include <QWidget>

#include <lvtqtw_export.h>
#include <memory>

class CodeVisApplicationTestFixture;
class QAction;
class QToolButton;
class QPaintEvent;

namespace Codethink::lvtqtc {
class ITool;
}

namespace Codethink::lvtqtw {

class LVTQTW_EXPORT ToolBox : public QWidget
// Toolbox, a Command Pallete box that has two
// visualization options. A single column with
// text, or a grid like view.
// Each element should belong to a category, and
// you can separate categories by creating a separator.
{
    Q_OBJECT

  public:
    friend class ::CodeVisApplicationTestFixture;

    // CREATORS
    explicit ToolBox(QWidget *parent = nullptr);
    ~ToolBox() override;

    // MUTATORS
    QToolButton *createToolButton(const QString& category, QAction *action);
    QToolButton *createToolButton(const QString& category, lvtqtc::ITool *tool);
    void createGroup(const QString& category);
    void hideElements(const QString& category);
    void showElements(const QString& category);

  protected slots:
    void setInformativeView(bool isActive);

  private:
    QToolButton *getButtonNamed(const std::string& title) const;

    // DATA
    struct Private;
    std::unique_ptr<Private> d;
};

} // namespace Codethink::lvtqtw
#endif
