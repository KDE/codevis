// desktopapp.m.cpp                                                  -*-C++-*-

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

#include <backward.hpp>

#include <QApplication>
#include <QByteArray>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDateTime>
#include <QStandardPaths>
#include <QStyle>
#include <QStyleFactory>

#include <mainwindow.h>

#include <cstdlib>
#include <filesystem>
#include <iostream>

#include <QDebug>

#include <codevis_dbus_interface.h>
#include <ct_lvtldr_nodestorage.h>
#include <ct_lvtmdl_debugmodel.h>
#include <ct_lvtplg_pluginmanager.h>
#include <ct_lvtqtc_undo_manager.h>

#include <preferences.h>

static void setupPath(char *argv[]) // NOLINT
{
    const std::filesystem::path argv0(argv[0]);
    const std::filesystem::path appimagePath = argv0.parent_path();

    qputenv("CT_LVT_BINDIR", QByteArray::fromStdString(appimagePath.string()));
}

void setApplicationStyle()
{
    QStyle *style = QApplication::style();

    // This style is the fallback style for Qt, it is basically
    // a windows95 version of the style bundled with each Qt app.
    // but there are other styles we can try that are not as rough.

    // documentation asks this to be set before the creation of QApplication
    if (!style || style->objectName() == "windows") {
        // "macintosh" theme only exists on osx, this will not break anything.
        for (const auto *newStyle : {"windowsvista", "fusion", "macintosh", "windows"}) {
            auto *retStyle = QApplication::setStyle(newStyle);
            if (retStyle) {
                break;
            }
        }
    }
}

int main(int argc, char *argv[])
{
    setApplicationStyle();

    // This should be called before the creation of the QApplication
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

#if (QT_VERSION <= QT_VERSION_CHECK(6, 0, 0))
    QCoreApplication::setAttribute(Qt::AA_DisableWindowContextHelpButton);
#endif

    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("Codethink");
    QCoreApplication::setOrganizationDomain("codethink.com");
    QCoreApplication::setApplicationName("Codevis");

    const QString folderPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + QDir::separator()
        + qApp->applicationName() // Folder
        + QDir::separator();

    const QString filePath = folderPath + qApp->applicationName() // File name
        + "_CrashDump_" + QDateTime::currentDateTime().toString(Qt::ISODate) + ".dump";

    QDir dir(folderPath);
    if (!dir.exists()) {
        bool created = dir.mkpath(folderPath);
        if (!created) {
            qDebug() << "Could not create the folder for crash dumps.";
        }
    }

    std::filesystem::path dumpFile(filePath.toStdString());
    backward::SignalHandling sh(backward::SignalHandling::make_default_signals(), dumpFile);

    // Ensure standard number formatting is used for float and string conversions
    if (setlocale(LC_NUMERIC, "C") == nullptr) {
        std::cerr << "Failed to set locale" << std::endl;
        return EXIT_FAILURE;
    }

    QCommandLineParser parser;
    parser.setApplicationDescription("CodeVis");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption inputFile(QStringList({"project-file"}),
                                 QObject::tr("[file path] a .lks file"),
                                 QObject::tr("file"));

    QCommandLineOption crashInfo(QStringList({"crash-info"}), QObject::tr("show the saved backtraces and exits."));

    QCommandLineOption resetSettings(
        QStringList({"reset-settings"}),
        QObject::tr("Reset the internal settings to vendor defaults and opens a fresh instance."));

    QCommandLineOption resetProject(
        QStringList({"reset-last-project"}),
        QObject::tr("Resets the last project, this can be used if the project makes the application crash."));

    parser.addOption(inputFile);
    parser.addOption(crashInfo);
    parser.addOption(resetSettings);
    parser.addOption(resetProject);

    parser.process(app);

    if (parser.isSet(crashInfo)) {
        qInfo() << "Current Crash Dumps on folder:" << dir.absolutePath();
        const auto files = dir.entryList(QDir::Filter::NoDotAndDotDot);
        for (const QString& file : std::as_const(files)) {
            qInfo() << file;
        }
        return 0;
    }

    if (parser.isSet(resetSettings)) {
        Preferences::self()->loadDefaults();
    }

    if (parser.isSet(resetProject)) {
        Preferences::self()->document()->setLastDocument("");
    }

    Q_INIT_RESOURCE(resources);

    setupPath(argv);

    // We need the debug model early to catch every possible debug message.
    Codethink::lvtmdl::DebugModel debugModel;
    qInstallMessageHandler(Codethink::lvtmdl::DebugModel::debugMessageHandler);

    auto pluginManager = Codethink::lvtplg::PluginManager{};
    pluginManager.loadPlugins();
    pluginManager.callHooksSetupPlugin();

    auto sharedNodeStorage = Codethink::lvtldr::NodeStorage{};
    auto undoManager = Codethink::lvtqtc::UndoManager{};
    MainWindow mWindow{sharedNodeStorage, &pluginManager, &undoManager, &debugModel};
    CodeVisDBusInterface dbusInterface{mWindow}; // cppcheck-suppress unreadVariable

    if (parser.isSet(inputFile)) {
        const bool isOpen = mWindow.openProjectFromPath(parser.value(inputFile));
        (void) isOpen; // NOLINT
    } else {
        const QString lastProject = Preferences::self()->document()->lastDocument();
        if (lastProject.size()) {
            const bool isOpen = mWindow.openProjectFromPath(lastProject);
            (void) isOpen; // NOLINT
        }
    }

    mWindow.show();

    int retValue = QApplication::exec();

    Preferences::self()->sync();
    pluginManager.callHooksTeardownPlugin();
    return retValue;
}
