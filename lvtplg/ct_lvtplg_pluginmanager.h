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

#include <ct_lvtplg_basicpluginhooks.h>
#include <ct_lvtplg_handlercontextmenuaction.h>
#include <ct_lvtplg_handlerdockwidget.h>
#include <ct_lvtplg_handlerentityreport.h>
#include <ct_lvtplg_handlersetup.h>
#include <ct_lvtplg_librarydispatcherinterface.h>
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
    void loadPlugins();

    void callHooksSetupPlugin();
    void callHooksTeardownPlugin();

    using getAllEntitiesInCurrentView_f = decltype(PluginContextMenuHandler::getAllEntitiesInCurrentView);
    using getEntityByQualifiedName_f = decltype(PluginContextMenuHandler::getEntityByQualifiedName);
    using registerContextMenu_f = decltype(PluginContextMenuHandler::registerContextMenu);
    void callHooksContextMenu(getAllEntitiesInCurrentView_f const& getAllEntitiesInCurrentView,
                              getEntityByQualifiedName_f const& getEntityByQualifiedName,
                              registerContextMenu_f const& registerContextMenu);

    using createPluginDock_f = decltype(PluginDockWidgetHandler::createNewDock);
    using addDockWdgTextField_f = decltype(PluginDockWidgetHandler::addDockWdgTextField);
    using addTree_f = decltype(PluginDockWidgetHandler::addTree);
    void callHooksSetupDockWidget(createPluginDock_f const& createPluginDock,
                                  addDockWdgTextField_f const& addDockWdgTextField,
                                  addTree_f const& addTree);

    using getEntity_f = decltype(PluginEntityReportHandler::getEntity);
    using addReport_f = decltype(PluginEntityReportHandler::addReport);
    void callHooksSetupEntityReport(getEntity_f const& getEntity, addReport_f const& addReport);

    void *getPluginData(std::string const& id) const;

    void registerPluginQObject(std::string const& id, QObject *object);
    QObject *getPluginQObject(std::string const& id) const;

  private:
    static bool isValidPlugin(QDir const& pluginDir);
    static std::unique_ptr<ILibraryDispatcher> loadSinglePlugin(QDir const& pluginDir);

    template<typename HookFunctionType, typename HandlerType>
    void callAllHooks(std::string&& hookName, HandlerType&& handler)
    {
        auto findAndCallHook = [&hookName, &handler](auto&& pluginLib) {
            auto hook = reinterpret_cast<HookFunctionType>(pluginLib->resolve(hookName.c_str()));
            if (!hook) {
                // Plugin doesnt implement this hook
                return;
            }
            hook(&handler);
        };

        for (auto&& pluginLib : libraries) {
            if (dynamic_cast<PythonLibraryDispatcher *>(pluginLib.get())) {
                py::gil_scoped_acquire _;
                findAndCallHook(pluginLib);
            } else {
                findAndCallHook(pluginLib);
            }
        }
    }

    template<typename DispatcherType>
    void tryInstallPlugin(QString const& pluginDir)
    {
        if (!DispatcherType::isValidPlugin(pluginDir)) {
            return;
        }

        auto lib = DispatcherType::loadSinglePlugin(pluginDir);
        if (!lib) {
            qDebug() << "Could not load valid plugin: " << pluginDir;
            return;
        }

        pluginData[DispatcherType::pluginDataId] = lib->getPluginData();
        libraries.push_back(std::move(lib));
    }

#ifdef ENABLE_PYTHON_PLUGINS
    py::scoped_interpreter py;
    py::gil_scoped_release defaultReleaseGIL;
#endif
    std::vector<std::unique_ptr<ILibraryDispatcher>> libraries;
    std::map<std::string, void *> pluginData;
    std::unordered_map<std::string, QObject *> pluginQObjects;
};
} // namespace Codethink::lvtplg

#endif // DIAGRAM_SERVER_CT_LVTPLG_PLUGINMANAGER_H
