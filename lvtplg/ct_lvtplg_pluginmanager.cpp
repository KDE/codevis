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

void PluginManager::loadPlugins()
{
    auto homePath = QDir(QDir::homePath() + "/lks-plugins");
    auto appPath = QDir(QCoreApplication::applicationDirPath() + "/lks-plugins");
    auto appPathStr = appPath.path().toStdString();
    for (auto pluginsPath : {homePath, appPath}) {
        if (!pluginsPath.exists() || !pluginsPath.isReadable()) {
            qDebug() << "Could not find plugins on path " << pluginsPath << ". Will not use plugins.";
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

void *PluginManager::getPluginData(std::string const& id) const
{
    return pluginData.at(id);
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
