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
#include <result/result.hpp>

// KDE
#include <KMessageBox>
#include <KNSWidgets/Button>
#include <KStandardGuiItem>
#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/View>

// Qt
#include <QApplication>
#include <QBoxLayout>
#include <QFileDialog>
#include <QInputDialog>
#include <QStandardPaths>
#include <QTabWidget>
#include <QToolBar>
#include <QToolButton>

// Own
#include <ct_lvtplg_pluginmanager.h>

using namespace Codethink::lvtqtw;

struct PluginEditor::Private {
    KTextEditor::Document *docReadme = nullptr;
    KTextEditor::View *viewReadme = nullptr;

    KTextEditor::Document *docPlugin = nullptr;
    KTextEditor::View *viewPlugin = nullptr;

    KTextEditor::Document *docLicense = nullptr;
    KTextEditor::View *viewLicense = nullptr;

    KTextEditor::Document *docMetadata = nullptr;
    KTextEditor::View *viewMetadata = nullptr;

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

    using RangeType = std::initializer_list<std::pair<KTextEditor::Document **, KTextEditor::View **>>;
    const auto elements = RangeType{{&d->docReadme, &d->viewReadme},
                                    {&d->docPlugin, &d->viewPlugin},
                                    {&d->docLicense, &d->viewLicense},
                                    {&d->docMetadata, &d->viewMetadata}};

    for (const auto& [doc, view] : elements) {
        (*doc) = editor->createDocument(this);
        (*view) = (*doc)->createView(d->documentViews);
        (*view)->setContextMenu((*view)->defaultContextMenu());
    }

    d->docReadme->setHighlightingMode("Markdown");
    d->docPlugin->setHighlightingMode("Python");
    d->docMetadata->setHighlightingMode("json");

    d->openFile = new QAction(tr("Open Python Plugin"));
    d->openFile->setIcon(QIcon::fromTheme("document-open"));
    connect(d->openFile, &QAction::triggered, this, &PluginEditor::load);

    d->newPlugin = new QAction(tr("New Plugin"));
    d->newPlugin->setIcon(QIcon::fromTheme("document-new"));
    connect(d->newPlugin, &QAction::triggered, this, &PluginEditor::requestCreatePythonPlugin);

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
    d->documentViews->addTab(d->viewLicense, QStringLiteral("LICENSE"));
    d->documentViews->addTab(d->viewMetadata, QStringLiteral("metadata.json"));
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

void PluginEditor::requestCreatePythonPlugin()
{
    const QString newPluginPath = QFileDialog::getExistingDirectory();
    if (newPluginPath.isEmpty()) {
        return;
    }

    createPythonPlugin(newPluginPath);
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

void PluginEditor::createPythonPlugin(const QString& pluginDir)
{
    if (!d->pluginManager) {
        sendErrorMsg(tr("%1 was build without plugin support.").arg(qApp->applicationName()));
        qDebug() << QStringLiteral("%1 was build without plugin support.").arg(qApp->applicationName());
        return;
    }

    // User canceled.
    if (pluginDir.isEmpty()) {
        return;
    }

    QDir pluginPath(pluginDir);
    if (!pluginPath.isEmpty()) {
        sendErrorMsg(tr("Please select an empty folder for the new python plugin"));
        qDebug() << "Please select an empty folder for the new python plugin";
        return;
    }

    const QString absPluginPath = pluginPath.absolutePath();

    const bool success = pluginPath.mkpath(pluginPath.path());
    if (!success) {
        sendErrorMsg(tr("Error creating the Plugin folder"));
        qDebug() << "Error creating the Plugin folder";
        return;
    }

    const auto files = std::initializer_list<std::pair<QString, QString>>{
        {QStringLiteral("LICENSE"), QStringLiteral("LICENSE")},
        {QStringLiteral("METADATA"), QStringLiteral("metadata.json")},
        {QStringLiteral("PLUGIN"), absPluginPath.split("/").last() + QStringLiteral(".py")},
        {QStringLiteral("README"), QStringLiteral("README.md")},
    };

    for (const auto& [qrc, local] : files) {
        if (!QFile::copy(QStringLiteral(":/python_templates/%1").arg(qrc), absPluginPath + QDir::separator() + local)) {
            qDebug() << "Error creating the README file";
            return;
        }

        QFile localFile(absPluginPath + QDir::separator() + local);
        if (!localFile.exists()) {
            qDebug() << "File does not exists on disk";
        }

        if (!localFile.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner)) {
            qDebug() << "Error setting file permissions";
            return;
        }
    }

    qDebug() << "Plugin files created successfully, loading the plugin";
    loadByPath(pluginDir);
}

cpp::result<void, PluginEditor::Error> PluginEditor::save()
{
    if (!d->pluginManager) {
        sendErrorMsg(tr("%1 was build without plugin support.").arg(qApp->applicationName()));
        return cpp::fail(
            Error{e_Errors::Error,
                  QStringLiteral("%1 was build without plugin support.").arg(qApp->applicationName()).toStdString()});
    }

    for (auto *doc : {d->docPlugin, d->docLicense, d->docMetadata, d->docReadme}) {
        if (doc->url().isEmpty()) {
            continue;
        }
        if (!doc->save()) {
            sendErrorMsg(tr("Error saving plugin."));
            std::cout << "Error saving " << doc->url().toString().toStdString() << "\n";
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
        qDebug() << "Error name is empty";
        return;
    }

    const QString pluginPath = fName.split(QDir::separator()).last();
    loadByPath(pluginPath);
}

void PluginEditor::loadByPath(const QString& pluginPath)
{
    if (!d->pluginManager) {
        sendErrorMsg(tr("%1 was build without plugin support.").arg(qApp->applicationName()));
        return;
    }

    QFileInfo readmeInfo(pluginPath, QStringLiteral("README.md"));
    QFileInfo licenseInfo(pluginPath, QStringLiteral("LICENSE"));
    QFileInfo metadataInfo(pluginPath, QStringLiteral("metadata.json"));
    QFileInfo pluginInfo(pluginPath, pluginPath.split(QDir::separator()).last() + QStringLiteral(".py"));

    const QString errMsg = tr("Missing required file: %1");
    if (!readmeInfo.exists()) {
        qDebug() << pluginPath << readmeInfo.absoluteFilePath() << "Error 1";
        sendErrorMsg(errMsg.arg(QStringLiteral("README.md")));
        return;
    }

    if (!pluginInfo.exists()) {
        qDebug() << pluginInfo.absoluteFilePath() << "Error 2";
        sendErrorMsg(errMsg.arg(pluginInfo.fileName()));
        return;
    }

    d->docReadme->openUrl(QUrl::fromLocalFile(readmeInfo.absoluteFilePath()));
    d->docPlugin->openUrl(QUrl::fromLocalFile(pluginInfo.absoluteFilePath()));
    d->docLicense->openUrl(QUrl::fromLocalFile(metadataInfo.absoluteFilePath()));
    d->docMetadata->openUrl(QUrl::fromLocalFile(pluginInfo.absoluteFilePath()));

    d->documentViews->setTabText(3, pluginInfo.fileName());

    d->documentViews->setEnabled(true);
    d->currentPluginFolder = pluginPath;

    qDebug() << "Opened sucessfully";
}
