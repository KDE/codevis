// codevisdbusinterface.h                                       -*-C++-*-

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

#ifndef LAKOSDIAGRAMDBUSINTERFACE_H
#define LAKOSDIAGRAMDBUSINTERFACE_H

#include <QObject>
#include <mainwindow.h>

class CodeVisDBusInterface : public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.codethink.CodeVis")

  public:
    explicit CodeVisDBusInterface(MainWindow& mainWindow);

    // Tab Management
    // requests a new tab from the main window
  public Q_SLOTS:
    Q_SCRIPTABLE void requestNewTab();

    // requests to close the current tab
    Q_SCRIPTABLE void requestCloseCurrentTab();

    // toggles the split view on / off
    Q_SCRIPTABLE void requestToggleSplitView();

    // if split view is on, selects the left.
    Q_SCRIPTABLE void requestSelectLeftSplitView();

    // if split view is off, selects the right
    Q_SCRIPTABLE void requestSelectRightSplitView();

    // requests to close the application
    Q_SCRIPTABLE void requestQuit();

    // requests a database to open.
    Q_SCRIPTABLE bool requestLoadProject(const QString& project);

    // requests to load a package on the current tab
    Q_SCRIPTABLE void requestLoadPackage(const QString& qualifiedName);

    // requests to load a package on the current tab
    Q_SCRIPTABLE void requestLoadClass(const QString& qualifiedName);

    // requests to load a package on the current tab
    Q_SCRIPTABLE void requestLoadComponent(const QString& qualifiedName);

  private:
    MainWindow& mainWindow;
};

#endif
