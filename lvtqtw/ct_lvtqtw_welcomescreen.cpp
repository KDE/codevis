// ct_lvtqtw_welcomescreen.cpp                                     -*-C++-*-

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

#include <ct_lvtqtw_welcomescreen.h>

#include <ui_ct_lvtqtw_welcomewidget.h>

using namespace Codethink::lvtqtw;

WelcomeScreen::WelcomeScreen(QWidget *parent): QWidget(parent), ui(std::make_unique<Ui::WelcomeWidget>())
{
    ui->setupUi(this);
    connect(ui->btnEmptyProject, &QPushButton::clicked, this, &WelcomeScreen::requestNewProject);
    connect(ui->btnGenerateFromSource, &QPushButton::clicked, this, &WelcomeScreen::requestParseProject);
    connect(ui->btnExistingProject, &QPushButton::clicked, this, &WelcomeScreen::requestExistingProject);

    ui->textBrowser->setSource(QUrl("qrc:/html/news.html"));
}

WelcomeScreen::~WelcomeScreen() = default;

#include "moc_ct_lvtqtw_welcomescreen.cpp"
