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

#include <ct_lvtplg_basicpluginhooks.h>
#include <ct_lvtplg_handlercontextmenuaction.h>
#include <ct_lvtplg_handlerdockwidget.h>
#include <ct_lvtplg_handlerentityreport.h>
#include <ct_lvtplg_handlersetup.h>
#include <ct_lvtplg_pluginmanager.h>
#ifdef ENABLE_PYTHON_PLUGINS
#include <ct_lvtplg_pythonlibrarydispatcher.h>
#endif
#include <ct_lvtplg_sharedlibrarydispatcher.h>

#include <QCoreApplication>

namespace Codethink::lvtplg {

void PluginManager::loadPlugins(std::optional<QDir> preferredPath)
{
    auto homePath = QDir(QDir::homePath() + "/lks-plugins");
    auto appPath = QDir(QCoreApplication::applicationDirPath() + "/lks-plugins");

    auto searchPaths = std::vector<QDir>{homePath, appPath};
    if (preferredPath) {
        searchPaths.insert(searchPaths.begin(), *preferredPath);
    }

    for (auto pluginsPath : searchPaths) {
        qDebug() << "Loading plugins from " << pluginsPath.path() << "...";
        if (!pluginsPath.exists() || !pluginsPath.isReadable()) {
            qDebug() << "Couldn't find any plugin on path " << pluginsPath.path();
            continue;
        }

        pluginsPath.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
        pluginsPath.setSorting(QDir::Name);
        auto entryInfoList = pluginsPath.entryInfoList();
        for (auto const& fileInfo : qAsConst(entryInfoList)) {
            auto pluginDir = fileInfo.absoluteFilePath();
            tryInstallPlugin<SharedLibraryDispatcher>(pluginDir);
#ifdef ENABLE_PYTHON_PLUGINS
            tryInstallPlugin<PythonLibraryDispatcher>(pluginDir);
#endif
        }
    }

    qDebug() << "Loaded plugins:";
    for (auto&& p : this->libraries) {
        qDebug() << "+ " << QString::fromStdString(p->fileName());
    }
}

void PluginManager::callHooksSetupPlugin()
{
    callAllHooks<hookSetupPlugin_f>("hookSetupPlugin",
                                    PluginSetupHandler{/* registerPluginData */
                                                       [this](std::string const& id, void *data) {
                                                           pluginData[id] = data;
                                                       },
                                                       /* getPluginData */
                                                       [this](std::string const& id) {
                                                           return pluginData.at(id);
                                                       },
                                                       /* unregisterPluginData */
                                                       [this](std::string const& id) {
                                                           pluginData.erase(id);
                                                       }});
}

void PluginManager::callHooksTeardownPlugin()
{
    callAllHooks<hookTeardownPlugin_f>("hookTeardownPlugin",
                                       PluginSetupHandler{/* registerPluginData */
                                                          [this](std::string const& id, void *data) {
                                                              pluginData[id] = data;
                                                          },
                                                          /* getPluginData */
                                                          [this](std::string const& id) {
                                                              return pluginData.at(id);
                                                          },
                                                          /* unregisterPluginData */
                                                          [this](std::string const& id) {
                                                              pluginData.erase(id);
                                                          }});
}

void PluginManager::callHooksContextMenu(getAllEntitiesInCurrentView_f const& getAllEntitiesInCurrentView,
                                         getEntityByQualifiedName_f const& getEntityByQualifiedName,
                                         registerContextMenu_f const& registerContextMenu)
{
    auto getPluginData = [this](auto&& id) {
        return this->getPluginData(id);
    };
    callAllHooks<hookGraphicsViewContextMenu_f>("hookGraphicsViewContextMenu",
                                                PluginContextMenuHandler{getPluginData,
                                                                         getAllEntitiesInCurrentView,
                                                                         getEntityByQualifiedName,
                                                                         registerContextMenu});
}

void PluginManager::callHooksSetupDockWidget(createPluginDock_f const& createPluginDock,
                                             addDockWdgTextField_f const& addDockWdgTextField,
                                             addTree_f const& addTree)
{
    auto getPluginData = [this](auto&& id) {
        return this->getPluginData(id);
    };
    callAllHooks<hookSetupDockWidget_f>(
        "hookSetupDockWidget",
        PluginDockWidgetHandler{getPluginData, createPluginDock, addDockWdgTextField, addTree});
}

void PluginManager::callHooksSetupEntityReport(getEntity_f const& getEntity, addReport_f const& addReport)
{
    auto getPluginData = [this](auto&& id) {
        return this->getPluginData(id);
    };
    callAllHooks<hookSetupEntityReport_f>("hookSetupEntityReport",
                                          PluginEntityReportHandler{getPluginData, getEntity, addReport});
}

void PluginManager::callHooksPhysicalParserOnHeaderFound(getSourceFile_f const& getSourceFile,
                                                         getIncludedFile_f const& getIncludedFile,
                                                         getLineNo_f const& getLineNo)
{
    auto getPluginData = [this](auto&& id) {
        return this->getPluginData(id);
    };
    callAllHooks<hookPhysicalParserOnHeaderFound_f>(
        "hookPhysicalParserOnHeaderFound",
        PluginPhysicalParserOnHeaderFoundHandler{getPluginData, getSourceFile, getIncludedFile, getLineNo});
}

void PluginManager::callHooksPluginLogicalParserOnCppCommentFoundHandler(getFilename_f const& getFilename,
                                                                         getBriefText_f const& getBriefText,
                                                                         getStartLine_f const& getStartLine,
                                                                         getEndLine_f const& getEndLine)
{
    auto getPluginData = [this](auto&& id) {
        return this->getPluginData(id);
    };
    callAllHooks<hookLogicalParserOnCppCommentFound_f>("hookLogicalParserOnCppCommentFound",
                                                       PluginLogicalParserOnCppCommentFoundHandler{getPluginData,
                                                                                                   getFilename,
                                                                                                   getBriefText,
                                                                                                   getStartLine,
                                                                                                   getEndLine});
}

void PluginManager::callHooksOnParseCompleted(runQueryOnDatabase_f const& runQueryOnDatabase)
{
    auto getPluginData = [this](auto&& id) {
        return this->getPluginData(id);
    };
    callAllHooks<hookOnParseCompleted_f>("hookOnParseCompleted",
                                         PluginParseCompletedHandler{getPluginData, runQueryOnDatabase});
}

void *PluginManager::getPluginData(std::string const& id) const
{
    try {
        return pluginData.at(id);
    } catch (std::out_of_range const&) {
        return nullptr;
    }
}

void PluginManager::registerPluginQObject(std::string const& id, QObject *object)
{
    pluginQObjects[id] = object;
}

QObject *PluginManager::getPluginQObject(std::string const& id) const
{
    return pluginQObjects.at(id);
}

} // namespace Codethink::lvtplg
