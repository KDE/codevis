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

#include <QBoxLayout>
#include <QFileDialog>
#include <QInputDialog>
#include <QTabWidget>
#include <QToolBar>
#include <QToolButton>

using namespace Codethink::lvtqtw;

QDir PluginEditor::basePluginPath()
{
    return QDir::homePath() + "/lks-plugins/";
}

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
    QAction *runPlugin = nullptr;
    QAction *closePlugin = nullptr;

    bool hasPlugin = false;
};

PluginEditor::PluginEditor(QWidget *parent): QWidget(parent), d(std::make_unique<PluginEditor::Private>())
{
    auto editor = KTextEditor::Editor::instance();

    d->docReadme = editor->createDocument(this);
    d->viewReadme = d->docReadme->createView(this);
    d->docPlugin = editor->createDocument(this);
    d->viewPlugin = d->docPlugin->createView(this);

    d->docReadme->setHighlightingMode("Markdown");
    d->docPlugin->setHighlightingMode("Python");

    d->openFile = new QAction(tr("Open Python Plugin"));
    d->openFile->setIcon(QIcon::fromTheme("document-open"));
    connect(d->openFile, &QAction::triggered, this, &PluginEditor::load);

    d->newPlugin = new QAction(tr("New Plugin"));
    d->newPlugin->setIcon(QIcon::fromTheme("document-new"));
    connect(d->newPlugin, &QAction::triggered, this, &PluginEditor::create);

    d->savePlugin = new QAction(tr("Save plugin"));
    d->savePlugin->setIcon(QIcon::fromTheme("document-save"));
    connect(d->savePlugin, &QAction::triggered, this, &PluginEditor::save);

    d->closePlugin = new QAction(tr("Close plugin"));
    d->closePlugin->setIcon(QIcon::fromTheme("document-close"));
    connect(d->closePlugin, &QAction::triggered, this, &PluginEditor::close);

    d->runPlugin = new QAction(tr("Run Script"));
    d->runPlugin->setIcon(QIcon::fromTheme("system-run"));

    auto *toolBar = new QToolBar(this);
    toolBar->addActions({d->newPlugin, d->openFile, d->savePlugin, d->closePlugin, d->runPlugin});

    d->documentViews = new QTabWidget();
    d->documentViews->addTab(d->viewReadme, QStringLiteral("README.md"));
    d->documentViews->addTab(d->viewPlugin, QString());

    auto *l = new QBoxLayout(QBoxLayout::TopToBottom);

    d->documentViews->setEnabled(false);

    l->addWidget(toolBar);
    l->addWidget(d->documentViews);
    l->setContentsMargins(0, 0, 0, 0);

    setLayout(l);
}

PluginEditor::~PluginEditor() = default;

void PluginEditor::close()
{
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

void PluginEditor::create()
{
    const QString pluginName = QInputDialog::getText(this, tr("Create a new Python Plugin"), tr("Plugin Name:"));

    // User canceled.
    if (pluginName.isEmpty()) {
        return;
    }

    QDir pluginPath(basePluginPath().path() + QStringLiteral("/") + pluginName);
    const bool success = pluginPath.mkpath(pluginPath.path());
    if (!success) {
        sendErrorMsg(tr("Error creating the Plugin folder"));
    }

    // TODO: Use a better approach than handcreating those files.
    QFile markdownFile(pluginPath.path() + QStringLiteral("/README.md"));
    markdownFile.open(QIODevice::ReadWrite);

    QFile pluginFile(pluginPath.path() + QStringLiteral("/") + pluginName + QStringLiteral(".py"));
    pluginFile.open(QIODevice::ReadWrite);

    loadByName(pluginName);
}

void PluginEditor::save()
{
    d->docPlugin->save();
    d->docReadme->save();
}

void PluginEditor::load()
{
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
}
