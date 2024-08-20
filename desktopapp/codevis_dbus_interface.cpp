// codevisdbusinterface.cpp                                     -*-C++-*-

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

#include <codevis_dbus_interface.h>

// dbus is not linux only but conan is having issues compiling the
// needed library, so blocking it on Mac and Windows temporarily.
#ifdef Q_OS_LINUX
#include <QDBusConnection>
#include <codevisadaptor.h>
#endif

CodeVisDBusInterface::CodeVisDBusInterface(MainWindow& mainWindow): mainWindow(mainWindow)
{
#ifdef Q_OS_LINUX
    new CodeVisAdaptor(this);

    auto bus = QDBusConnection::sessionBus();
    bus.registerObject("/Application", this);
    bus.registerService("org.codethink.CodeVis");
#endif
}

void CodeVisDBusInterface::requestNewTab()
{
    mainWindow.newTab();
}

void CodeVisDBusInterface::requestCloseCurrentTab()
{
    mainWindow.closeCurrentTab();
}

// toggles the split view on / off
void CodeVisDBusInterface::requestToggleSplitView()
{
    mainWindow.toggleSplitView();
}

// requests to close the application
void CodeVisDBusInterface::requestQuit()
{
    mainWindow.close();
}

// requests a database to open.
bool CodeVisDBusInterface::requestLoadProject(const QString& projectPath)
{
    return mainWindow.openProjectFromPath(projectPath);
}

// requests to load a class on the current tab
void CodeVisDBusInterface::requestLoadPackage(const QString& qualifiedName)
{
    mainWindow.setCurrentGraphFromString(Codethink::lvtmdl::NodeType::Enum::e_Package, qualifiedName);
}

// requests to load a class on the current tab
void CodeVisDBusInterface::requestLoadClass(const QString& qualifiedName)
{
    mainWindow.setCurrentGraphFromString(Codethink::lvtmdl::NodeType::Enum::e_Class, qualifiedName);
}

// requests to load a class on the current tab
void CodeVisDBusInterface::requestLoadComponent(const QString& qualifiedName)
{
    mainWindow.setCurrentGraphFromString(Codethink::lvtmdl::NodeType::Enum::e_Component, qualifiedName);
}

void CodeVisDBusInterface::requestSelectLeftSplitView()
{
    mainWindow.selectLeftSplitView();
}

void CodeVisDBusInterface::requestSelectRightSplitView()
{
    mainWindow.selectRightSplitView();
}

#include "moc_codevis_dbus_interface.cpp"
