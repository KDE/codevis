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

#include <QApplication>
#include <QByteArray>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDateTime>
#include <QStandardPaths>
#include <QStyle>
#include <QStyleFactory>

#include <KAboutData>
#include <KCrash>
#include <KLocalizedString>

#include <mainwindow.h>

#include <cstdlib>
#include <iostream>

#include <QDebug>

#include <codevis_dbus_interface.h>
#include <ct_lvtldr_nodestorage.h>
#include <ct_lvtmdl_debugmodel.h>
#include <ct_lvtplg_pluginmanager.h>
#include <ct_lvtqtc_undo_manager.h>

#include <PluginManagerV2.h>
#include <preferences.h>

int main(int argc, char *argv[])
{
    // This should be called before the creation of the QApplication
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

#if (QT_VERSION <= QT_VERSION_CHECK(6, 0, 0))
    QCoreApplication::setAttribute(Qt::AA_DisableWindowContextHelpButton);
#endif

    QApplication app(argc, argv);
    KCrash::initialize();

    // setup translation string domain for the i18n calls
    KLocalizedString::setApplicationDomain("codevis");
    // create a KAboutData object to use for setting the application metadata
    KAboutData aboutData("codevis",
                         i18n("Codevis"),
                         "0.1",
                         i18n("Visualize and extract information from Large Codebases"),
                         KAboutLicense::BSDL, // Apache, but KAboutLicense lacks that.
                         i18n("Copyright 2023 KDE"),
                         QString(),
                         "https://invent.kde.org/sdk/codevis");

    // overwrite default-generated values of organizationDomain & desktopFileName
    aboutData.setOrganizationDomain("kde.org");
    aboutData.setDesktopFileName("org.kde.codevis");

    aboutData.addAuthor(i18n("Tomaz Canabrava"), i18n("Developer"), QStringLiteral("tcanabrava@kde.org"));
    aboutData.addAuthor(i18n("Tarcisio Fischer"),
                        i18n("Developer"),
                        QStringLiteral("tarcisio.fischer@codethink.co.uk"));
    aboutData.addAuthor(i18n("Richard Dale"), i18n("Developer"), QStringLiteral("richard.dale@codethink.co.uk"));
    aboutData.addAuthor(i18n("Tom Eccles"), i18n("Developer"));
    aboutData.addAuthor(i18n("Poppy Singleton"), i18n("Developer"));

    // set the application metadata
    KAboutData::setApplicationData(aboutData);

    MainWindow::initializeResource();

    // in GUI apps set the window icon manually, not covered by KAboutData
    // needed for environments where the icon name is not extracted from
    // the information in the application's desktop file
    QApplication::setWindowIcon(QIcon::fromTheme(QStringLiteral("codevis")));

    const QString folderPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + QDir::separator()
        + qApp->applicationName() // Folder
        + QDir::separator();

    QDir dir(folderPath);
    if (!dir.exists()) {
        bool created = dir.mkpath(folderPath);
        if (!created) {
            qDebug() << "Could not create the folder for crash dumps.";
        }
    }

    // Ensure standard number formatting is used for float and string conversions
    if (setlocale(LC_NUMERIC, "C") == nullptr) {
        std::cerr << "Failed to set locale" << std::endl;
        return EXIT_FAILURE;
    }

    QCommandLineParser parser;
    aboutData.setupCommandLine(&parser);

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
    aboutData.processCommandLine(&parser);

    if (parser.isSet(crashInfo)) {
        qInfo() << "Current Crash Dumps on folder:" << dir.absolutePath();
        const auto files = dir.entryList(QDir::Filter::NoDotAndDotDot);
        for (const QString& file : std::as_const(files)) {
            qInfo() << file;
        }
        return 0;
    }

    if (parser.isSet(resetSettings)) {
        Preferences::self()->setDefaults();
    }

    if (parser.isSet(resetProject)) {
        Preferences::setLastDocument("");
    }

    Q_INIT_RESOURCE(resources);

    // We need the debug model early to catch every possible debug message.
    Codethink::lvtmdl::DebugModel debugModel;
    qInstallMessageHandler(Codethink::lvtmdl::DebugModel::debugMessageHandler);

    // Path of plugins installed by GetNewStuff.
    // TODO Move those things to the 2nd gen pluginmanager.
    auto pluginSearchPaths = Preferences::pluginSearchPaths();
    pluginSearchPaths.append(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/plugins");
    auto pluginManager = Codethink::lvtplg::PluginManager{};
    pluginManager.loadPlugins(pluginSearchPaths);
    pluginManager.callHooksSetupPlugin();

    auto& pm = Codevis::PluginSystem::PluginManagerV2::self();
    pm.loadAllPlugins();

    auto sharedNodeStorage = Codethink::lvtldr::NodeStorage{};
    auto undoManager = Codethink::lvtqtc::UndoManager{};
    auto *mWindow = new MainWindow(sharedNodeStorage, &pluginManager, &undoManager, &debugModel);

    CodeVisDBusInterface dbusInterface(*mWindow); // cppcheck-suppress unreadVariable
    pluginManager.callHooksMainWindowReady(std::bind_front(&MainWindow::addMenuFromPlugin, mWindow));

    if (parser.isSet(inputFile)) {
        const bool isOpen = mWindow->openProjectFromPath(parser.value(inputFile));
        (void) isOpen; // NOLINT
    } else {
        const QString lastProject = Preferences::lastDocument();
        if (lastProject.size()) {
            const bool isOpen = mWindow->openProjectFromPath(lastProject);
            (void) isOpen; // NOLINT
        }
    }

    mWindow->show();

    int retValue = QApplication::exec();

    Preferences::self()->save();
    pluginManager.callHooksTeardownPlugin();
    return retValue;
}
