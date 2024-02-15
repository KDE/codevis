// ct_lvtplg_pluginmanager.h                                         -*-C++-*-

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

#ifndef DIAGRAM_SERVER_CT_LVTPLG_PLUGINMANAGER_H
#define DIAGRAM_SERVER_CT_LVTPLG_PLUGINMANAGER_H

#include <lvtplg_export.h>

#include <ct_lvtplg_abstractlibrarydispatcher.h>
#include <ct_lvtplg_basicpluginhandlers.h>
#include <ct_lvtplg_basicpluginhooks.h>
#include <ct_lvtplg_pythonlibrarydispatcher.h>

#include <QDebug>
#include <QDir>
#include <QString>

#pragma push_macro("slots")
#undef slots
#include <pybind11/embed.h>
#pragma pop_macro("slots")

#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace py = pybind11;

namespace Codethink::lvtplg {

class LVTPLG_EXPORT PluginManager {
  public:
    PluginManager() = default;
    PluginManager(PluginManager const&) = delete;
    PluginManager(PluginManager&&) = delete;
    PluginManager operator=(PluginManager const&) = delete;
    PluginManager operator=(PluginManager&) = delete;
    void loadPlugins(const QList<QString>& paths);
    std::vector<std::string> getPluginsMetadataFilePaths() const;
    std::optional<std::reference_wrapper<AbstractLibraryDispatcher>> getPluginById(std::string const& id) const;

    void reloadPlugin(const QString& pluginfolder);
    void removePlugin(const QString& pluginFolder);

    void callHooksSetupPlugin();
    void callHooksMainWindowReady();
    void callHooksTeardownPlugin();

    using getAllEntitiesInCurrentView_f = decltype(PluginContextMenuHandler::getAllEntitiesInCurrentView);
    using getEntityByQualifiedName_f = decltype(PluginContextMenuHandler::getEntityByQualifiedName);
    using getEdgeByQualifiedName_f = decltype(PluginContextMenuHandler::getEdgeByQualifiedName);
    using registerContextMenu_f = decltype(PluginContextMenuHandler::registerContextMenu);
    void callHooksContextMenu(getAllEntitiesInCurrentView_f const& getAllEntitiesInCurrentView,
                              getEntityByQualifiedName_f const& getEntityByQualifiedName,
                              getEdgeByQualifiedName_f const& getEdgeByQualifiedName,
                              registerContextMenu_f const& registerContextMenu);

    using createPluginDock_f = decltype(PluginSetupDockWidgetHandler::createNewDock);
    void callHooksSetupDockWidget(createPluginDock_f const& createPluginDock);

    using getEntity_f = decltype(PluginEntityReportHandler::getEntity);
    using addReport_f = decltype(PluginEntityReportHandler::addReport);
    void callHooksSetupEntityReport(getEntity_f const& getEntity, addReport_f const& addReport);

    using getSourceFile_f = decltype(PluginPhysicalParserOnHeaderFoundHandler::getSourceFile);
    using getIncludedFile_f = decltype(PluginPhysicalParserOnHeaderFoundHandler::getIncludedFile);
    using getLineNo_f = decltype(PluginPhysicalParserOnHeaderFoundHandler::getLineNo);
    void callHooksPhysicalParserOnHeaderFound(getSourceFile_f const& getSourceFile,
                                              getIncludedFile_f const& getIncludedFile,
                                              getLineNo_f const& getLineNo);

    using getFilename_f = decltype(PluginLogicalParserOnCppCommentFoundHandler::getFilename);
    using getBriefText_f = decltype(PluginLogicalParserOnCppCommentFoundHandler::getBriefText);
    using getStartLine_f = decltype(PluginLogicalParserOnCppCommentFoundHandler::getStartLine);
    using getEndLine_f = decltype(PluginLogicalParserOnCppCommentFoundHandler::getEndLine);
    void callHooksPluginLogicalParserOnCppCommentFoundHandler(getFilename_f const& getFilename,
                                                              getBriefText_f const& getBriefText,
                                                              getStartLine_f const& getStartLine,
                                                              getEndLine_f const& getEndLine);

    using runQueryOnDatabase_f = decltype(PluginParseCompletedHandler::runQueryOnDatabase);
    void callHooksOnParseCompleted(runQueryOnDatabase_f const& runQueryOnDatabase);

    using getSceneName_f = decltype(PluginActiveSceneChangedHandler::getSceneName);
    void callHooksActiveSceneChanged(getSceneName_f const& getSceneName);

    using graphChanged_getSceneName_f = decltype(PluginGraphChangedHandler::getSceneName);
    using graphChanged_getVisibleEntities_f = decltype(PluginGraphChangedHandler::getVisibleEntities);
    using graphChanged_getEdgeByQualifiedName_f = decltype(PluginGraphChangedHandler::getEdgeByQualifiedName);
    using graphChanged_getProjectData_f = decltype(PluginGraphChangedHandler::getProjectData);
    void callHooksGraphChanged(graphChanged_getSceneName_f const& getSceneName,
                               graphChanged_getVisibleEntities_f const& getVisibleEntities,
                               graphChanged_getEdgeByQualifiedName_f const& getEdgeByQualifiedName,
                               graphChanged_getProjectData_f const& getProjectData);

    void registerPluginData(std::string const& id, void *data);
    void unregisterPluginData(std::string const& id);
    void *getPluginData(std::string const& id) const;

    void registerPluginQObject(std::string const& id, QObject *object);
    QObject *getPluginQObject(std::string const& id) const;
    PluginPythonInterpHandler getPyInterpHandler() const;

  private:
#ifdef ENABLE_PYTHON_PLUGINS
    py::scoped_interpreter py;
    py::gil_scoped_release defaultReleaseGIL;
#endif
    std::unordered_map<std::string, std::unique_ptr<AbstractLibraryDispatcher>> libraries;
    std::map<std::string, void *> pluginData;
    std::unordered_map<std::string, QObject *> pluginQObjects;
};
} // namespace Codethink::lvtplg

#endif // DIAGRAM_SERVER_CT_LVTPLG_PLUGINMANAGER_H
