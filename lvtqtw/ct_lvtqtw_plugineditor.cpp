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

#include <ct_lvtqtw_plugineditor.h>

#include <KMessageBox>

#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/View>

#include <QApplication>
#include <QBoxLayout>
#include <QFileDialog>
#include <QInputDialog>
#include <QTabWidget>
#include <QToolBar>
#include <QToolButton>

#include <ct_lvtplg_pluginmanager.h>

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

    auto *toolBar = new QToolBar(this);
    toolBar->addActions({d->newPlugin, d->openFile, d->savePlugin, d->closePlugin, d->reloadPlugin});

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

    save();
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
    markdownFile.open(QIODevice::ReadWrite);

    QFile pluginFile(pluginPath.path() + QStringLiteral("/") + pluginName + QStringLiteral(".py"));
    pluginFile.open(QIODevice::ReadWrite);

    loadByName(pluginName);
}

void PluginEditor::save()
{
    if (!d->pluginManager) {
        sendErrorMsg(tr("%1 was build without plugin support.").arg(qApp->applicationName()));
        return;
    }

    d->docPlugin->save();
    d->docReadme->save();
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
