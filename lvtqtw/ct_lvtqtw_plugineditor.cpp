// ct_lvtqtw_plugineditor.cpp                                             -*-C++-*-

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

#include "result/result.hpp"
#include <ct_lvtqtw_plugineditor.h>

#include <KMessageBox>

#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/View>

#include <QApplication>
#include <QBoxLayout>
#include <QFileDialog>
#include <QInputDialog>
#include <QStandardPaths>
#include <QTabWidget>
#include <QToolBar>
#include <QToolButton>

#include <KNSWidgets/Button>

#include <ct_lvtplg_pluginmanager.h>
#include <kmessagebox.h>
#include <kstandardguiitem.h>

using namespace Codethink::lvtqtw;

struct PluginEditor::Private {
    KTextEditor::Document *docReadme = nullptr;
    KTextEditor::View *viewReadme = nullptr;

    KTextEditor::Document *docPlugin = nullptr;
    KTextEditor::View *viewPlugin = nullptr;

    QTabWidget *documentViews = nullptr;

    // Those QActions will probably move to something else when we use KXmlGui
    QAction *openFile = nullptr;
    QAction *newPlugin = nullptr;
    QAction *savePlugin = nullptr;
    QAction *reloadPlugin = nullptr;
    QAction *closePlugin = nullptr;
    KNSWidgets::Button *getNewButton = nullptr;

    lvtplg::PluginManager *pluginManager = nullptr;

    QString currentPluginFolder;
    QString basePluginPath = QDir::homePath() + "/lks-plugins/";

    bool hasPlugin = false;
};

PluginEditor::PluginEditor(QWidget *parent): QWidget(parent), d(std::make_unique<PluginEditor::Private>())
{
    d->documentViews = new QTabWidget();

    auto *editor = KTextEditor::Editor::instance();
    if (!editor) {
        std::cout << "Error getting the ktexteditor\n";
        abort();
    }

    d->docReadme = editor->createDocument(this);
    d->viewReadme = d->docReadme->createView(d->documentViews);
    d->docPlugin = editor->createDocument(this);
    d->viewPlugin = d->docPlugin->createView(d->documentViews);

    d->docReadme->setHighlightingMode("Markdown");
    d->docPlugin->setHighlightingMode("Python");

    d->viewReadme->setContextMenu(d->viewReadme->defaultContextMenu());
    d->viewPlugin->setContextMenu(d->viewPlugin->defaultContextMenu());

    d->openFile = new QAction(tr("Open Python Plugin"));
    d->openFile->setIcon(QIcon::fromTheme("document-open"));
    connect(d->openFile, &QAction::triggered, this, &PluginEditor::load);

    d->newPlugin = new QAction(tr("New Plugin"));
    d->newPlugin->setIcon(QIcon::fromTheme("document-new"));
    connect(d->newPlugin, &QAction::triggered, this, [this] {
        create();
    });

    d->savePlugin = new QAction(tr("Save plugin"));
    d->savePlugin->setIcon(QIcon::fromTheme("document-save"));
    connect(d->savePlugin, &QAction::triggered, this, &PluginEditor::save);

    d->closePlugin = new QAction(tr("Close plugin"));
    d->closePlugin->setIcon(QIcon::fromTheme("document-close"));
    connect(d->closePlugin, &QAction::triggered, this, &PluginEditor::close);

    d->reloadPlugin = new QAction(tr("Reload Script"));
    d->reloadPlugin->setIcon(QIcon::fromTheme("system-run"));
    connect(d->reloadPlugin, &QAction::triggered, this, &PluginEditor::reloadPlugin);

    d->getNewButton = new KNSWidgets::Button(this);
    d->getNewButton->setConfigFile(QStringLiteral("codevis.knsrc"));
    d->getNewButton->setText("Download Scripts");
    connect(d->getNewButton,
            &KNSWidgets::Button::dialogFinished,
            this,
            &Codethink::lvtqtw::PluginEditor::getNewScriptFinished);

    auto *toolBar = new QToolBar(this);
    toolBar->addActions({d->newPlugin, d->openFile, d->savePlugin, d->closePlugin, d->reloadPlugin});
    toolBar->addWidget(d->getNewButton);

    d->documentViews->addTab(d->viewReadme, QStringLiteral("README.md"));
    d->documentViews->addTab(d->viewPlugin, QString());

    auto *l = new QBoxLayout(QBoxLayout::TopToBottom);

    d->documentViews->setEnabled(false);

    l->addWidget(toolBar);
    l->addWidget(d->documentViews);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(0);

    setLayout(l);
}

PluginEditor::~PluginEditor() = default;

#if KNEWSTUFF_VERSION >= QT_VERSION_CHECK(5, 91, 0)
#if QT_VERSION > QT_VERSION_CHECK(6, 0, 0)
void PluginEditor::getNewScriptFinished(const QList<KNSCore::Entry>& changedEntries)
#else
void PluginEditor::getNewScriptFinished(const QList<KNSCore::EntryInternal>& changedEntries)
#endif
#else
void PluginEditor::getNewScriptFinished(const KNSCore::Entry::List& changedEntries)
#endif
{
    bool installed = false;
    // Our plugins are gzipped, and the installedFiles returns with `/*` in the end to denote
    // multiple files. we need to chop that off when we want to install it.
    QList<QString> installedPlugins;
    for (const auto& entry : qAsConst(changedEntries)) {
        switch (entry.status()) {
        case KNSCore::Entry::Installed: {
            if (entry.installedFiles().count() == 0) {
                continue;
            }

            QString filename = entry.installedFiles()[0];
            // remove /* from the string.
            installedPlugins.append(filename.chopped(2));

            installed = true;
        } break;
        case KNSCore::Entry::Deleted:
            KMessageBox::information(
                this,
                tr("Codevis doesn't support hot reloading of plugins, Save your work and restart the application."),
                tr("Restart Required"));
            break;
        case KNSCore::Entry::Invalid:
        case KNSCore::Entry::Installing:
        case KNSCore::Entry::Downloadable:
        case KNSCore::Entry::Updateable:
        case KNSCore::Entry::Updating:
            // Not interesting.
            break;
        }
    }

    // Refresh the plugins installed by GetNewStuff
    if (installed) {
        for (const auto& installedPlugin : installedPlugins) {
            d->pluginManager->reloadPlugin(installedPlugin);
        }
    }
}

QDir PluginEditor::basePluginPath()
{
    return d->basePluginPath;
}

void PluginEditor::setBasePluginPath(const QString& path)
{
    d->basePluginPath = path;
}

void PluginEditor::reloadPlugin()
{
    // Implementation notes:
    // Reloading a single plugin will either lose it's state or put it in a bad state (depending on how the plugin is
    // implemented). Main issue that I'm thinking is the plugin data (registerPluginData and getPluginData). Need to
    // check if we'll re-run the setup hooks or just leave the plugin in the last-state. Perhaps having a dedicated
    // button to "reload" and another to "restart" plugin (Restart would run the setup hook and cleanup data
    // structures?)
    if (!d->pluginManager) {
        sendErrorMsg(tr("%1 was build without plugin support.").arg(qApp->applicationName()));
        return;
    }

    auto res = save();
    if (res.has_error()) {
        // no need to try to reload anything that has errors.
        return;
    }

    d->pluginManager->reloadPlugin(d->currentPluginFolder);
}

void PluginEditor::setPluginManager(lvtplg::PluginManager *manager)
{
    d->pluginManager = manager;
}

void PluginEditor::close()
{
    if (!d->pluginManager) {
        sendErrorMsg(tr("%1 was build without plugin support.").arg(qApp->applicationName()));
        return;
    }

    if (d->docPlugin->isModified() || d->docReadme->isModified()) {
        auto saveAction = KGuiItem(tr("Save Plugins"));
        auto discardAction = KGuiItem(tr("Discard Changes"));

        const bool saveThings = KMessageBox::questionTwoActions(this,
                                                                tr("Plugin has changed, Save it?"),
                                                                tr("Save plugins?"),
                                                                saveAction,
                                                                discardAction)
            == KMessageBox::ButtonCode::PrimaryAction;

        if (saveThings) {
            d->docPlugin->save();
            d->docReadme->save();
        }
    }
    d->docPlugin->closeUrl();
    d->docReadme->closeUrl();
    d->documentViews->setEnabled(false);
}

void PluginEditor::create(const QString& pluginName)
{
    if (!d->pluginManager) {
        sendErrorMsg(tr("%1 was build without plugin support.").arg(qApp->applicationName()));
        return;
    }

    const QString internalPluginName = pluginName.isEmpty()
        ? QInputDialog::getText(this, tr("Create a new Python Plugin"), tr("Plugin Name:"))
        : pluginName;

    // User canceled.
    if (internalPluginName.isEmpty()) {
        return;
    }

    QDir pluginPath(basePluginPath().path() + QStringLiteral("/") + internalPluginName);
    QFileInfo pluginPathInfo(pluginPath.path());
    if (pluginPathInfo.exists()) {
        auto firstAction = KGuiItem(tr("Open Existing"));
        auto secondAction = KGuiItem(tr("Create New"));

        const bool openExsting = KMessageBox::questionTwoActions(this,
                                                                 tr("Plugin has changed, Save it?"),
                                                                 tr("Save plugins?"),
                                                                 firstAction,
                                                                 secondAction)
            == KMessageBox::ButtonCode::PrimaryAction;
        if (openExsting) {
            loadByName(internalPluginName);
            return;
        }

        pluginPath.removeRecursively();
    }

    const bool success = pluginPath.mkpath(pluginPath.path());
    if (!success) {
        sendErrorMsg(tr("Error creating the Plugin folder"));
        return;
    }

    QFile markdownFile(pluginPath.path() + QStringLiteral("/README.md"));
    markdownFile.open(QIODevice::WriteOnly);
    markdownFile.close();

    QFile pluginFile(pluginPath.path() + QStringLiteral("/") + internalPluginName + QStringLiteral(".py"));
    pluginFile.open(QIODevice::WriteOnly);
    pluginFile.close();

    loadByName(internalPluginName);
}

cpp::result<void, PluginEditor::Error> PluginEditor::save()
{
    if (!d->pluginManager) {
        sendErrorMsg(tr("%1 was build without plugin support.").arg(qApp->applicationName()));
        return cpp::fail(
            Error{e_Errors::Error,
                  QStringLiteral("%1 was build without plugin support.").arg(qApp->applicationName()).toStdString()});
    }

    if (!d->docPlugin->url().isEmpty()) {
        if (!d->docPlugin->save()) {
            sendErrorMsg(tr("Error saving plugin."));
            return cpp::fail(Error{e_Errors::Error, "Error saving plugin"});
        }
    }
    if (!d->docReadme->url().isEmpty()) {
        if (!d->docReadme->save()) {
            sendErrorMsg(tr("%1 was build without plugin support.").arg(qApp->applicationName()));
            return cpp::fail(Error{e_Errors::Error, "Error saving plugin"});
        }
    }

    return {};
}

void PluginEditor::load()
{
    if (!d->pluginManager) {
        sendErrorMsg(tr("%1 was build without plugin support.").arg(qApp->applicationName()));
        return;
    }

    const QString fName = QFileDialog::getExistingDirectory(this, tr("Python Script File"), QDir::homePath());

    // User clicked "cancel", not an error.
    if (fName.isEmpty()) {
        return;
    }

    const QString pluginName = fName.split(QDir::separator()).last();
    loadByName(pluginName);
}

void PluginEditor::loadByName(const QString& pluginName)
{
    if (!d->pluginManager) {
        sendErrorMsg(tr("%1 was build without plugin support.").arg(qApp->applicationName()));
        return;
    }

    const QString thisPluginPath = basePluginPath().path() + QStringLiteral("/") + pluginName;

    QFileInfo readmeInfo(thisPluginPath, QStringLiteral("README.md"));
    QFileInfo pluginInfo(thisPluginPath, pluginName + QStringLiteral(".py"));

    const QString errMsg = tr("Missing required file: %1");
    if (!readmeInfo.exists()) {
        sendErrorMsg(errMsg.arg(QStringLiteral("README.md")));
        return;
    }

    if (!pluginInfo.exists()) {
        sendErrorMsg(errMsg.arg(pluginName + QStringLiteral(".py")));
        return;
    }

    d->docReadme->openUrl(QUrl::fromLocalFile(thisPluginPath + QStringLiteral("/README.md")));
    d->docPlugin->openUrl(
        QUrl::fromLocalFile(thisPluginPath + QStringLiteral("/") + pluginName + QStringLiteral(".py")));
    d->documentViews->setTabText(1, pluginName + QStringLiteral(".py"));

    d->documentViews->setEnabled(true);
    d->currentPluginFolder = thisPluginPath;
}
