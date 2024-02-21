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

#include <ct_lvtplg_basicpluginhandlers.h>
#include <ct_lvtplg_basicpluginhooks.h>

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

// clang-format off
template<typename ModuleType>
void exportDataTypes(ModuleType& m)
{
    {
        using T = EntityType;
        py::enum_<T>(m, "EntityType")
            .value("Unknown", T::Unknown)
            .value("PackageGroup", T::PackageGroup)
            .value("Package", T::Package)
            .value("Component", T::Component)
            .export_values();
    }

    {
        using T = PluginFieldType;
        py::enum_<T>(m, "PluginFieldType")
            .value("TextInput", T::TextInput)
            .value("TextArea", T::TextArea)
            .export_values();
    }

    {
        using T = Color;
        py::class_<T>(m, "Color")
            .def(py::init<int, int, int>())
            .def(py::init<int, int, int, int>())
            .def_readwrite("r", &T::r)
            .def_readwrite("g", &T::g)
            .def_readwrite("b", &T::b)
            .def_readwrite("a", &T::a);
    }

    {
        using T = Entity;
        py::class_<T>(m, "Entity")
            .def("getName", [](T const& self) { return self.getName(); })
            .def("getQualifiedName", [](T const& self) { return self.getQualifiedName(); })
            .def("getType", [](T const& self) { return self.getType(); })
            .def("setColor", [](T const& self, Color rgbColor) { return self.setColor(rgbColor); }, py::arg("rgbColor"))
            .def("addHoverInfo", [](T const& self, std::string info) { return self.addHoverInfo(std::move(info)); }, py::arg("info"))
            .def("getDependencies", [](T const& self) { return self.getDependencies(); })
            .def("unloadFromScene", [](T const& self) { return self.unloadFromScene(); })
            .def("getDbChildrenQualifiedNames", [](T const& self) { return self.getDbChildrenQualifiedNames(); })
            .def("getParent", [](T const& self) { return self.getParent(); })
            .def("setSelected", [](T const& self, bool v) { return self.setSelected(v); })
            .def("isSelected", [](T const& self) -> bool { return self.isSelected(); });
    }

    {
        using T = Edge;
        py::class_<T>(m, "Edge")
            .def("setColor", [](T const& self, Color rgbColor) { return self.setColor(rgbColor); }, py::arg("color"))
            .def("setStyle", [](T const& self, EdgeStyle style) { return self.setStyle(style); }, py::arg("style"));
    }

    {
        using T = ProjectData;
        py::class_<T>(m, "ProjectData").def("getSourceCodePath", [](T const& self) { self.getSourceCodePath(); });
    }
}

#include <ct_lvtplg_handlerbindings.inc.cpp>
// clang-format on

// NOLINTBEGIN
PYMODULE_TYPE(pyLksPlugin, m)
{
    m.doc() = "Codevis plugins module.";
    exportDataTypes(m);
    exportHandlers(m);
}
// NOLINTEND
