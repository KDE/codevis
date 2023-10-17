// ct_lvtplg_pythonmodule.cpp                                        -*-C++-*-

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
#include <ct_lvtplg_handlercodeanalysis.h>
#include <ct_lvtplg_handlercontextmenuaction.h>
#include <ct_lvtplg_handlerdockwidget.h>
#include <ct_lvtplg_handlerentityreport.h>
#include <ct_lvtplg_handlergraphicsview.h>
#include <ct_lvtplg_handlersetup.h>
#include <ct_lvtplg_handlertreewidget.h>

#pragma push_macro("slots")
#undef slots
#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#pragma pop_macro("slots")

#include <optional>
#include <string>

#if defined AS_EMBEDDED
#include <pybind11/embed.h>
#define PYMODULE_TYPE PYBIND11_EMBEDDED_MODULE
#elif defined AS_MODULE
#define PYMODULE_TYPE PYBIND11_MODULE
#else
#error "Please select compilation macro option: AS_EMBEDDED or AS_MODULE"
#endif

// #define PYMODULE_TYPE PYBIND11_MODULE

namespace py = pybind11;

namespace Codethink::lvtplg {

template<typename T>
auto pyRegisterPluginData(T const& self, std::string const& id, py::object pyObject)
{
    auto *rawPtr = static_cast<void *>(pyObject.ptr());
    Py_XINCREF(rawPtr);
    return self.registerPluginData(id, rawPtr);
}

template<typename T>
auto pyGetPluginData(T const& self, std::string const& id)
{
    auto *rawPtr = static_cast<PyObject *>(self.getPluginData(id));
    return py::reinterpret_borrow<py::object>(rawPtr);
}

template<typename T>
auto pyUnregisterPluginData(T const& self, std::string const& id)
{
    auto *rawPtr = static_cast<PyObject *>(self.getPluginData(id));
    Py_XDECREF(rawPtr);
    return self.unregisterPluginData(id);
}
} // namespace Codethink::lvtplg

// NOLINTBEGIN
PYMODULE_TYPE(pyLksPlugin, m)
{
    using namespace Codethink::lvtplg;

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
        .def_readonly("addHoverInfo", &Entity::addHoverInfo)
        .def_readonly("getDependencies", &Entity::getDependencies)
        .def_readonly("unloadFromScene", &Entity::unloadFromScene)
        .def_readonly("getDbChildrenQualifiedNames", &Entity::getDbChildrenQualifiedNames)
        .def_readonly("getParent", &Entity::getParent);

    py::class_<Edge>(m, "Edge").def_readonly("setColor", &Edge::setColor).def_readonly("setStyle", &Edge::setStyle);

    py::class_<ProjectData>(m, "ProjectData").def_readonly("getSourceCodePath", &ProjectData::getSourceCodePath);

    {
        using T = PluginSetupHandler;
        py::class_<T>(m, "PluginSetupHandler")
            .def("registerPluginData", &pyRegisterPluginData<T>)
            .def("getPluginData", &pyGetPluginData<T>)
            .def("unregisterPluginData", &pyUnregisterPluginData<T>);
    }

    {
        using T = PluginContextMenuHandler;
        py::class_<T>(m, "PluginContextMenuHandler")
            .def("getPluginData", &pyGetPluginData<T>)
            .def_readonly("getAllEntitiesInCurrentView", &T::getAllEntitiesInCurrentView)
            .def_readonly("getEntityByQualifiedName", &T::getEntityByQualifiedName)
            .def_readonly("registerContextMenu", &T::registerContextMenu);
    }

    {
        using T = PluginContextMenuActionHandler;
        py::class_<T>(m, "PluginContextMenuActionHandler")
            .def("getPluginData", &pyGetPluginData<T>)
            .def_readonly("getAllEntitiesInCurrentView", &T::getAllEntitiesInCurrentView)
            .def_readonly("getEntityByQualifiedName", &T::getEntityByQualifiedName);
    }

    {
        using T = PluginDockWidgetHandler;
        py::class_<T>(m, "PluginDockWidgetHandler")
            .def("getPluginData", &pyGetPluginData<T>)
            .def_readonly("createNewDock", &T::createNewDock)
            .def("addDockWdgTextField",
                 [](T const& self, std::string const& dockId, std::string const& title, py::object dataModel) {
                     // TODO: Check how we can retrieve text field from python object
                 });
    }

    {
        using T = PluginEntityReportHandler;
        py::class_<T>(m, "PluginEntityReportHandler")
            .def("getPluginData", &pyGetPluginData<T>)
            .def_readonly("getEntity", &T::getEntity)
            .def_readonly("addReport", &T::addReport);
    }

    {
        using T = PluginEntityReportActionHandler;
        py::class_<T>(m, "PluginEntityReportActionHandler")
            .def("getPluginData", &pyGetPluginData<T>)
            .def_readonly("getEntity", &T::getEntity)
            .def_readonly("setReportContents", &T::setReportContents);
    }

    {
        using T = PluginMainNodeChangedHandler;
        py::class_<T>(m, "PluginMainNodeChangedHandler")
            .def("getPluginData", &pyGetPluginData<T>)
            .def_readonly("getEntity", &T::getEntity)
            .def_readonly("getVisibleEntities", &T::getVisibleEntities)
            .def_readonly("getEdgeByQualifiedName", &T::getEdgeByQualifiedName)
            .def_readonly("getProjectData", &T::getProjectData);
    }
}
// NOLINTEND
