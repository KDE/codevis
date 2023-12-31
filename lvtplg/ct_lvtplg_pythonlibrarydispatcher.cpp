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
                     // Doesn't increment the internal counter. Lifetime must be ensured by caller.
                     auto *rawPtr = (void *) (pyObject.ptr());
                     self.registerPluginData(id, rawPtr);
                 })
            .def("getPluginData",
                 [](T const& self, std::string const& id) {
                     auto *rawPtr = (PyObject *) self.getPluginData(id);
                     return py::reinterpret_borrow<py::object>(rawPtr);
                 })
            .def_readonly("unregisterPluginData", &T::unregisterPluginData);
    }

    {
        using T = PluginContextMenuHandler;
        py::class_<T>(m, "PluginContextMenuHandler")
            .def("getPluginData",
                 [](T const& self, std::string const& id) {
                     auto *rawPtr = (PyObject *) self.getPluginData(id);
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
                     auto *rawPtr = (PyObject *) self.getPluginData(id);
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
                     auto *rawPtr = (PyObject *) self.getPluginData(id);
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
                     auto *rawPtr = (PyObject *) self.getPluginData(id);
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
                     auto *rawPtr = (PyObject *) self.getPluginData(id);
                     return py::reinterpret_borrow<py::object>(rawPtr);
                 })
            .def_readonly("getEntity", &T::getEntity)
            .def_readonly("setReportContents", &T::setReportContents);
    }
}
// NOLINTEND

namespace Codethink::lvtplg {

void *PythonLibraryDispatcher::getPluginData()
{
    return &this->pyModule;
}

functionPointer PythonLibraryDispatcher::resolve(std::string const& functionName)
{
    if (!py::hasattr(pyModule, functionName.c_str())) {
        return nullptr;
    }

    return CppToPythonBridge::get(functionName);
}

std::string PythonLibraryDispatcher::fileName()
{
    return "";
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
