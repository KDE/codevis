// ct_lvtqtw_welcomescreen.h                                     -*-C++-*-

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

#ifndef INCLUDED_LVTQTW_WELCOMESCREEN
#define INCLUDED_LVTQTW_WELCOMESCREEN

#include <lvtqtw_export.h>

#include <QWidget>

#include <memory>

namespace Ui {
class WelcomeWidget;
}
class QPaintEvent;

namespace Codethink::lvtqtw {

class LVTQTW_EXPORT WelcomeScreen : public QWidget {
    Q_OBJECT
  public:
    explicit WelcomeScreen(QWidget *parent = nullptr);
    ~WelcomeScreen();

    Q_SIGNAL void requestNewProject();
    Q_SIGNAL void requestParseProject();
    Q_SIGNAL void requestExistingProject();

  private:
    struct Private;
    std::unique_ptr<Ui::WelcomeWidget> ui;
};

} // namespace Codethink::lvtqtw

#endif
