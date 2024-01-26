// ct_lvtplg_pluginmanager.cpp                                       -*-C++-*-

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

#include <ct_lvtplg_basicpluginhandlers.h>
#include <ct_lvtplg_basicpluginhooks.h>
#include <ct_lvtplg_pluginmanager.h>
#ifdef ENABLE_PYTHON_PLUGINS
#include <ct_lvtplg_pythonlibrarydispatcher.h>
#endif
#include <ct_lvtplg_sharedlibrarydispatcher.h>

#include <KPluginMetaData>
#include <QCoreApplication>

namespace {
template<typename HookFunctionType, typename HandlerType>
void callSingleHook(const std::string& hookName,
                    HandlerType&& handler,
                    Codethink::lvtplg::AbstractLibraryDispatcher *library)
{
    if (!library->isEnabled()) {
        return;
    }

    auto resolveContext = library->resolve(hookName);
    if (resolveContext->hook) {
        reinterpret_cast<HookFunctionType>(resolveContext->hook)(&handler);
    }
}

template<typename HookFunctionType, typename HandlerType>
void callAllHooks(
    const std::string& hookName,
    HandlerType&& handler,
    std::unordered_map<std::string, std::unique_ptr<Codethink::lvtplg::AbstractLibraryDispatcher>>& libraries)
{
    QDebug dbg(QtDebugMsg);
    for (auto const& [_, pluginLib] : libraries) {
        if (!pluginLib->isEnabled()) {
            continue;
        }

        callSingleHook<HookFunctionType, HandlerType>(hookName, std::forward<HandlerType>(handler), pluginLib.get());
    }
}

template<typename DispatcherType>
void tryInstallPlugin(
    QString const& pluginDir,
    std::unordered_map<std::string, std::unique_ptr<Codethink::lvtplg::AbstractLibraryDispatcher>>& libraries)
{
    // TODO: This should check if the plugin is already loaded before attempting
    // to load it (because of knewstuff).
    QDebug dbg(QtDebugMsg);

    dbg << "(" << DispatcherType::name << "): Trying to install plugin " << pluginDir << "... ";
    if (!DispatcherType::isValidPlugin(pluginDir)) {
        dbg << "Invalid plugin.";
        return;
    }

    auto lib = DispatcherType::loadSinglePlugin(pluginDir);
    if (!lib) {
        dbg << "Could not load *valid* plugin: " << pluginDir;
        return;
    }

    auto id = lib->pluginId();
    libraries[id] = std::move(lib);
    dbg << "Plugin successfully loaded!";
}

template<typename DispatcherType>
void tryReloadPlugin(
    QString const& pluginDir,
    std::unordered_map<std::string, std::unique_ptr<Codethink::lvtplg::AbstractLibraryDispatcher>>& libraries)
{
    QDebug dbg(QtDebugMsg);

    dbg << "(" << DispatcherType::name << "): Trying to reload plugin " << pluginDir << "... ";
    const bool res = DispatcherType::reloadSinglePlugin(pluginDir);
    if (!res) {
        dbg << "Could not load *valid* plugin: " << pluginDir;
        return;
    }
}

void loadSinglePlugin(
    const QString& pluginDir,
    std::unordered_map<std::string, std::unique_ptr<Codethink::lvtplg::AbstractLibraryDispatcher>>& libraries)
{
    tryInstallPlugin<Codethink::lvtplg::SharedLibraryDispatcher>(pluginDir, libraries);
#ifdef ENABLE_PYTHON_PLUGINS
    tryInstallPlugin<Codethink::lvtplg::PythonLibraryDispatcher>(pluginDir, libraries);
#endif
}

} // namespace
namespace Codethink::lvtplg {

void PluginManager::loadPlugins(const QList<QString>& searchPaths)
{
    for (const auto& pluginsStr : searchPaths) {
        QDir pluginsPath(pluginsStr);

        qDebug() << "Loading plugins from " << pluginsPath.path() << "...";
        if (!pluginsPath.exists()) {
            qDebug() << pluginsPath.path() << "does not exists, skipping plugin search for it.";
            continue;
        }
        if (!pluginsPath.isReadable()) {
            qDebug() << pluginsPath.path() << "exists but it's not readable, skipping plugin search for it.";
            continue;
        }

        pluginsPath.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
        pluginsPath.setSorting(QDir::Name);
        const auto entryInfoList = pluginsPath.entryInfoList();
        qDebug() << entryInfoList.size() << "is the size of the number of files here";
        for (auto const& fileInfo : qAsConst(entryInfoList)) {
            qDebug() << "\tTesting fileInfo" << fileInfo;
            const auto pluginDir = fileInfo.absoluteFilePath();
            loadSinglePlugin(pluginDir, libraries);
        }
    }

    qDebug() << "Loaded plugins:";
    for (auto const& [_, p] : this->libraries) {
        qDebug() << "+ " << QString::fromStdString(p->fileName());
    }
}

void PluginManager::reloadPlugin(const QString& pluginFolder)
{
    for (auto const& [_, p] : this->libraries) {
        const QString loadedLibrary = QString::fromStdString(p->fileName());

        if (loadedLibrary.startsWith(pluginFolder)) {
            removePlugin(pluginFolder);
            loadSinglePlugin(pluginFolder, libraries);
            return;
        }
    }

    QFileInfo pluginPath(pluginFolder);
    if (!pluginPath.exists()) {
        return;
    }

    tryInstallPlugin<SharedLibraryDispatcher>(pluginPath.absoluteFilePath(), libraries);
#ifdef ENABLE_PYTHON_PLUGINS
    tryInstallPlugin<PythonLibraryDispatcher>(pluginPath.absoluteFilePath(), libraries);
#endif
}

void PluginManager::removePlugin(const QString& pluginFolder)
{
    for (auto const& [key, p] : this->libraries) {
        const QString loadedLibrary = QString::fromStdString(p->fileName());

        if (loadedLibrary.startsWith(pluginFolder)) {
            auto handler = PluginSetupHandler{std::bind_front(&PluginManager::registerPluginData, this),
                                              std::bind_front(&PluginManager::getPluginData, this),
                                              std::bind_front(&PluginManager::unregisterPluginData, this)};

            callSingleHook<hookTeardownPlugin_f>("hookTeardownPlugin", handler, p.get());
            p->unload();
            this->libraries.erase(key);
            return;
        }
    }
}

std::vector<std::string> PluginManager::getPluginsMetadataFilePaths() const
{
    auto metadataFilePaths = std::vector<std::string>{};
    for (auto const& [_, lib] : this->libraries) {
        metadataFilePaths.push_back(lib->metadataFilePath());
    }
    return metadataFilePaths;
}

std::optional<std::reference_wrapper<AbstractLibraryDispatcher>>
PluginManager::getPluginById(std::string const& id) const
{
    if (!this->libraries.contains(id)) {
        return std::nullopt;
    }
    return *this->libraries.at(id);
}

void PluginManager::callHooksSetupPlugin()
{
    callAllHooks<hookSetupPlugin_f>("hookSetupPlugin",
                                    PluginSetupHandler{std::bind_front(&PluginManager::registerPluginData, this),
                                                       std::bind_front(&PluginManager::getPluginData, this),
                                                       std::bind_front(&PluginManager::unregisterPluginData, this)},
                                    libraries);
}

void PluginManager::callHooksTeardownPlugin()
{
    callAllHooks<hookTeardownPlugin_f>("hookTeardownPlugin",
                                       PluginSetupHandler{std::bind_front(&PluginManager::registerPluginData, this),
                                                          std::bind_front(&PluginManager::getPluginData, this),
                                                          std::bind_front(&PluginManager::unregisterPluginData, this)},
                                       libraries);
}

void PluginManager::callHooksContextMenu(getAllEntitiesInCurrentView_f const& getAllEntitiesInCurrentView,
                                         getEntityByQualifiedName_f const& getEntityByQualifiedName,
                                         getEdgeByQualifiedName_f const& getEdgeByQualifiedName,
                                         registerContextMenu_f const& registerContextMenu)
{
    callAllHooks<hookGraphicsViewContextMenu_f>(
        "hookGraphicsViewContextMenu",
        PluginContextMenuHandler{std::bind_front(&PluginManager::getPluginData, this),
                                 getAllEntitiesInCurrentView,
                                 getEntityByQualifiedName,
                                 registerContextMenu,
                                 getEdgeByQualifiedName},
        libraries);
}

void PluginManager::callHooksSetupDockWidget(createPluginDock_f const& createPluginDock,
                                             addDockWdgTextField_f const& addDockWdgTextField,
                                             addTree_f const& addTree)
{
    callAllHooks<hookSetupDockWidget_f>("hookSetupDockWidget",
                                        PluginDockWidgetHandler{std::bind_front(&PluginManager::getPluginData, this),
                                                                createPluginDock,
                                                                addDockWdgTextField,
                                                                addTree},
                                        libraries);
}

void PluginManager::callHooksSetupEntityReport(getEntity_f const& getEntity, addReport_f const& addReport)
{
    callAllHooks<hookSetupEntityReport_f>(
        "hookSetupEntityReport",
        PluginEntityReportHandler{std::bind_front(&PluginManager::getPluginData, this), getEntity, addReport},
        libraries);
}

void PluginManager::callHooksPhysicalParserOnHeaderFound(getSourceFile_f const& getSourceFile,
                                                         getIncludedFile_f const& getIncludedFile,
                                                         getLineNo_f const& getLineNo)
{
    callAllHooks<hookPhysicalParserOnHeaderFound_f>(
        "hookPhysicalParserOnHeaderFound",
        PluginPhysicalParserOnHeaderFoundHandler{std::bind_front(&PluginManager::getPluginData, this),
                                                 getSourceFile,
                                                 getIncludedFile,
                                                 getLineNo},
        libraries);
}

void PluginManager::callHooksPluginLogicalParserOnCppCommentFoundHandler(getFilename_f const& getFilename,
                                                                         getBriefText_f const& getBriefText,
                                                                         getStartLine_f const& getStartLine,
                                                                         getEndLine_f const& getEndLine)
{
    callAllHooks<hookLogicalParserOnCppCommentFound_f>(
        "hookLogicalParserOnCppCommentFound",
        PluginLogicalParserOnCppCommentFoundHandler{std::bind_front(&PluginManager::getPluginData, this),
                                                    getFilename,
                                                    getBriefText,
                                                    getStartLine,
                                                    getEndLine},
        libraries);
}

void PluginManager::callHooksOnParseCompleted(runQueryOnDatabase_f const& runQueryOnDatabase)
{
    callAllHooks<hookOnParseCompleted_f>(
        "hookOnParseCompleted",
        PluginParseCompletedHandler{std::bind_front(&PluginManager::getPluginData, this), runQueryOnDatabase},
        libraries);
}

void PluginManager::callHooksActiveSceneChanged(getSceneName_f const& getSceneName)
{
    callAllHooks<hookActiveSceneChanged_f>(
        "hookActiveSceneChanged",
        PluginActiveSceneChangedHandler{std::bind_front(&PluginManager::getPluginData, this), getSceneName},
        libraries);
}

void PluginManager::callHooksGraphChanged(graphChanged_getSceneName_f const& getSceneName,
                                          graphChanged_getVisibleEntities_f const& getVisibleEntities,
                                          graphChanged_getEdgeByQualifiedName_f const& getEdgeByQualifiedName,
                                          graphChanged_getProjectData_f const& getProjectData)
{
    callAllHooks<hookGraphChanged_f>("hookGraphChanged",
                                     PluginGraphChangedHandler{std::bind_front(&PluginManager::getPluginData, this),
                                                               getSceneName,
                                                               getVisibleEntities,
                                                               getEdgeByQualifiedName,
                                                               getProjectData},
                                     libraries);
}

void PluginManager::registerPluginData(std::string const& id, void *data)
{
    this->pluginData[id] = data;
}

void PluginManager::unregisterPluginData(std::string const& id)
{
    this->pluginData.erase(id);
}

void *PluginManager::getPluginData(std::string const& id) const
{
    try {
        return this->pluginData.at(id);
    } catch (std::out_of_range const&) {
        return nullptr;
    }
}

void PluginManager::registerPluginQObject(std::string const& id, QObject *object)
{
    this->pluginQObjects[id] = object;
}

QObject *PluginManager::getPluginQObject(std::string const& id) const
{
    return this->pluginQObjects.at(id);
}

} // namespace Codethink::lvtplg
