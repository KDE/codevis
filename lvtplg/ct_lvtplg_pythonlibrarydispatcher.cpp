// ct_lvtplg_pythonlibrarydispatcher.cpp                             -*-C++-*-

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

#include <ct_lvtplg_pythonlibrarydispatcher.h>

namespace Codethink::lvtplg {

const std::string PythonLibraryDispatcher::pluginDataId = "private::pythonLibData";

}

// NOLINTBEGIN
PYBIND11_EMBEDDED_MODULE(pyLksPlugin, m)
{
    m.doc() = "Module containing entities exposed to python for code generation scripts";

    py::enum_<EntityType>(m, "EntityType")
        .value("Unknown", EntityType::Unknown)
        .value("PackageGroup", EntityType::PackageGroup)
        .value("Package", EntityType::Package)
        .value("Component", EntityType::Component)
        .export_values();

    py::class_<Color>(m, "Color")
        .def(py::init<int, int, int>())
        .def(py::init<int, int, int, int>())
        .def_readwrite("r", &Color::r)
        .def_readwrite("g", &Color::g)
        .def_readwrite("b", &Color::b)
        .def_readwrite("a", &Color::a);

    py::class_<Entity>(m, "Entity")
        .def_readonly("getName", &Entity::getName)
        .def_readonly("getQualifiedName", &Entity::getQualifiedName)
        .def_readonly("getType", &Entity::getType)
        .def_readonly("setColor", &Entity::setColor)
        .def_readonly("addHoverInfo", &Entity::addHoverInfo);

    {
        using T = PluginSetupHandler;
        py::class_<T>(m, "PluginSetupHandler")
            .def("registerPluginData",
                 [](T const& self, std::string const& id, py::object pyObject) {
                     auto *rawPtr = static_cast<void *>(pyObject.ptr());
                     Py_XINCREF(rawPtr);
                     self.registerPluginData(id, rawPtr);
                 })
            .def("getPluginData",
                 [](T const& self, std::string const& id) {
                     auto *rawPtr = static_cast<PyObject *>(self.getPluginData(id));
                     return py::reinterpret_borrow<py::object>(rawPtr);
                 })
            .def("unregisterPluginData", [](T const& self, std::string const& id) {
                auto *rawPtr = static_cast<PyObject *>(self.getPluginData(id));
                Py_XDECREF(rawPtr);
                self.unregisterPluginData(id);
            });
    }

    {
        using T = PluginContextMenuHandler;
        py::class_<T>(m, "PluginContextMenuHandler")
            .def("getPluginData",
                 [](T const& self, std::string const& id) {
                     auto *rawPtr = static_cast<PyObject *>(self.getPluginData(id));
                     return py::reinterpret_borrow<py::object>(rawPtr);
                 })
            .def_readonly("getAllEntitiesInCurrentView", &T::getAllEntitiesInCurrentView)
            .def_readonly("getEntityByQualifiedName", &T::getEntityByQualifiedName)
            .def_readonly("registerContextMenu", &T::registerContextMenu);
    }

    {
        using T = PluginContextMenuActionHandler;
        py::class_<T>(m, "PluginContextMenuActionHandler")
            .def("getPluginData",
                 [](T const& self, std::string const& id) {
                     auto *rawPtr = static_cast<PyObject *>(self.getPluginData(id));
                     return py::reinterpret_borrow<py::object>(rawPtr);
                 })
            .def_readonly("getAllEntitiesInCurrentView", &T::getAllEntitiesInCurrentView)
            .def_readonly("getEntityByQualifiedName", &T::getEntityByQualifiedName);
    }

    {
        using T = PluginDockWidgetHandler;
        py::class_<T>(m, "PluginDockWidgetHandler")
            .def("getPluginData",
                 [](T const& self, std::string const& id) {
                     auto *rawPtr = static_cast<PyObject *>(self.getPluginData(id));
                     return py::reinterpret_borrow<py::object>(rawPtr);
                 })
            .def_readonly("createNewDock", &T::createNewDock)
            .def("addDockWdgTextField",
                 [](T const& self, std::string const& dockId, std::string const& title, py::object dataModel) {
                     // TODO: Check how we can retrieve text field from python object
                 });
    }

    {
        using T = PluginEntityReportHandler;
        py::class_<T>(m, "PluginEntityReportHandler")
            .def("getPluginData",
                 [](T const& self, std::string const& id) {
                     auto *rawPtr = static_cast<PyObject *>(self.getPluginData(id));
                     return py::reinterpret_borrow<py::object>(rawPtr);
                 })
            .def_readonly("getEntity", &T::getEntity)
            .def_readonly("addReport", &T::addReport);
    }

    {
        using T = PluginEntityReportActionHandler;
        py::class_<T>(m, "PluginEntityReportActionHandler")
            .def("getPluginData",
                 [](T const& self, std::string const& id) {
                     auto *rawPtr = static_cast<PyObject *>(self.getPluginData(id));
                     return py::reinterpret_borrow<py::object>(rawPtr);
                 })
            .def_readonly("getEntity", &T::getEntity)
            .def_readonly("setReportContents", &T::setReportContents);
    }
}
// NOLINTEND

namespace Codethink::lvtplg {

#define REGISTER_PLUGIN_WRAPPER(_hookName, _handlerType)                                                               \
    static void _hookName##Wrapper(_handlerType *handler)                                                              \
    {                                                                                                                  \
        auto& module = *PythonLibraryDispatcher::PyResolveContext::activeModule;                                       \
        auto func = module.attr(#_hookName);                                                                           \
        auto pyLksPlugin = py::module_::import("pyLksPlugin");                                                         \
        (void) pyLksPlugin;                                                                                            \
        func(handler);                                                                                                 \
    }

REGISTER_PLUGIN_WRAPPER(hookSetupPlugin, PluginSetupHandler)
REGISTER_PLUGIN_WRAPPER(hookTeardownPlugin, PluginSetupHandler)
REGISTER_PLUGIN_WRAPPER(hookGraphicsViewContextMenu, PluginContextMenuHandler)
REGISTER_PLUGIN_WRAPPER(hookSetupDockWidget, PluginDockWidgetHandler)
REGISTER_PLUGIN_WRAPPER(hookSetupEntityReport, PluginEntityReportHandler)

#define REGISTER_PLUGIN_HOOK_SELECTOR(_hookName)                                                                       \
    if (hookName == #_hookName) {                                                                                      \
        this->hook = (functionPointer) (&_hookName##Wrapper);                                                          \
    }

py::module_ *PythonLibraryDispatcher::PyResolveContext::activeModule = nullptr;

PythonLibraryDispatcher::PyResolveContext::PyResolveContext(py::module_& module, const std::string& hookName):
    ILibraryDispatcher::ResolveContext(nullptr)
{
    PythonLibraryDispatcher::PyResolveContext::activeModule = &module;
    if (py::hasattr(module, hookName.c_str())) {
        REGISTER_PLUGIN_HOOK_SELECTOR(hookSetupPlugin)
        REGISTER_PLUGIN_HOOK_SELECTOR(hookTeardownPlugin)
        REGISTER_PLUGIN_HOOK_SELECTOR(hookGraphicsViewContextMenu)
        REGISTER_PLUGIN_HOOK_SELECTOR(hookSetupDockWidget)
        REGISTER_PLUGIN_HOOK_SELECTOR(hookSetupEntityReport)
    }
}

#undef REGISTER_PLUGIN_WRAPPER
#undef REGISTER_PLUGIN_HOOK_SELECTOR

PythonLibraryDispatcher::PyResolveContext::~PyResolveContext()
{
    PythonLibraryDispatcher::PyResolveContext::activeModule = nullptr;
}

std::unique_ptr<ILibraryDispatcher::ResolveContext> PythonLibraryDispatcher::resolve(std::string const& functionName)
{
    return std::make_unique<PyResolveContext>(this->pyModule, functionName);
}

std::string PythonLibraryDispatcher::fileName()
{
    py::gil_scoped_acquire _;
    return this->pyModule.attr("__file__").cast<std::string>();
}

bool PythonLibraryDispatcher::isValidPlugin(QDir const& pluginDir)
{
    auto pluginName = pluginDir.dirName();
    return (pluginDir.exists("README.md") && pluginDir.exists(pluginName + ".py"));
}

std::unique_ptr<ILibraryDispatcher> PythonLibraryDispatcher::loadSinglePlugin(QDir const& pluginDir)
{
    py::gil_scoped_acquire _;

    auto pluginName = pluginDir.dirName();
    auto pyLib = std::make_unique<PythonLibraryDispatcher>();

    auto pySys = py::module_::import("sys");
    pySys.attr("path").attr("append")(pluginDir.path().toStdString());
    try {
        pyLib->pyModule = py::module_::import(pluginName.toStdString().c_str());
    } catch (py::error_already_set const& e) {
        // Could not load python module - Cleanup sys path and early return.
        pySys.attr("path").attr("append")(pluginDir.path());
        return nullptr;
    }

    return pyLib;
}

} // namespace Codethink::lvtplg
