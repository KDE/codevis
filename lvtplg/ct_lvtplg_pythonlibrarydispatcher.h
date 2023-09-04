// ct_lvtplg_pythonlibrarydispatcher.h                               -*-C++-*-

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

#ifndef DIAGRAM_SERVER_CT_LVTPLG_PYTHONLIBRARYDISPATCHER_H
#define DIAGRAM_SERVER_CT_LVTPLG_PYTHONLIBRARYDISPATCHER_H

#include <ct_lvtplg_basicpluginhooks.h>
#include <ct_lvtplg_handlercontextmenuaction.h>
#include <ct_lvtplg_handlerdockwidget.h>
#include <ct_lvtplg_handlerentityreport.h>
#include <ct_lvtplg_handlersetup.h>
#include <ct_lvtplg_librarydispatcherinterface.h>

#include <lvtplg_export.h>

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QLibrary>
#include <QString>

#pragma push_macro("slots")
#undef slots
#include <pybind11/embed.h>
#include <pybind11/functional.h>
#include <pybind11/stl.h>
#pragma pop_macro("slots")

#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace py = pybind11;

namespace Codethink::lvtplg {

class LVTPLG_EXPORT PythonLibraryDispatcher : public ILibraryDispatcher {
  public:
    static const std::string pluginDataId;

    void *getPluginData() override;
    functionPointer resolve(std::string const& functionName) override;
    std::string fileName() override;
    static bool isValidPlugin(QDir const& pluginDir);
    static std::unique_ptr<ILibraryDispatcher> loadSinglePlugin(QDir const& pluginDir);

  private:
    struct CppToPythonBridge {
#define REGISTER_PLUGIN_WRAPPER(_hookName, _handlerType)                                                               \
    static void _hookName##Wrapper(_handlerType *handler)                                                              \
    {                                                                                                                  \
        auto module = *static_cast<py::module_ *>(handler->getPluginData(PythonLibraryDispatcher::pluginDataId));      \
        auto func = module.attr(#_hookName);                                                                           \
        auto pyLksPlugin = py::module_::import("pyLksPlugin");                                                         \
        (void) pyLksPlugin;                                                                                            \
        func(handler);                                                                                                 \
    }

#define REGISTER_PLUGIN_WRAPPER_GETTER(_hookName)                                                                      \
    if (hookName == #_hookName) {                                                                                      \
        return (functionPointer) (&CppToPythonBridge::_hookName##Wrapper);                                             \
    }

        REGISTER_PLUGIN_WRAPPER(hookSetupPlugin, PluginSetupHandler)
        REGISTER_PLUGIN_WRAPPER(hookTeardownPlugin, PluginSetupHandler)
        REGISTER_PLUGIN_WRAPPER(hookGraphicsViewContextMenu, PluginContextMenuHandler)
        REGISTER_PLUGIN_WRAPPER(hookSetupDockWidget, PluginDockWidgetHandler)
        REGISTER_PLUGIN_WRAPPER(hookSetupEntityReport, PluginEntityReportHandler)

        static functionPointer get(std::string const& hookName)
        {
            REGISTER_PLUGIN_WRAPPER_GETTER(hookSetupPlugin)
            REGISTER_PLUGIN_WRAPPER_GETTER(hookTeardownPlugin)
            REGISTER_PLUGIN_WRAPPER_GETTER(hookGraphicsViewContextMenu)
            REGISTER_PLUGIN_WRAPPER_GETTER(hookSetupDockWidget)
            REGISTER_PLUGIN_WRAPPER_GETTER(hookSetupEntityReport)

            return nullptr;
        }
    };

    py::module_ pyModule;

#undef REGISTER_PLUGIN_WRAPPER
#undef REGISTER_PLUGIN_WRAPPER_GETTER
};

} // namespace Codethink::lvtplg

#endif // DIAGRAM_SERVER_CT_LVTPLG_PYTHONLIBRARYDISPATCHER_H
