// ct_lvtcgn_generatecode.cpp                                       -*-C++-*-

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

#include <ct_lvtcgn_generatecode.h>

#include <filesystem>
#include <iostream>
#include <pybind11/embed.h>
#include <pybind11/stl.h>

namespace py = pybind11;

namespace Codethink::lvtcgn::mdl {

IPhysicalEntityInfo::~IPhysicalEntityInfo() = default;
ICodeGenerationDataProvider::~ICodeGenerationDataProvider() = default;

// TODO [#437]: Check if we can't use enums from lvtshr directly after architecture review.
enum class pyDiagramType { UnknownEntityType = 0, PackageGroupType = 1, PackageType = 10, ComponentType = 100 };

class pyPhysicalEntityInfoWrapper {
  public:
    pyPhysicalEntityInfoWrapper(std::reference_wrapper<IPhysicalEntityInfo> impl): impl(impl)
    {
    }

    [[nodiscard]] std::string name() const
    {
        return impl.get().name();
    }

    [[nodiscard]] pyDiagramType type() const
    {
        auto type = impl.get().type();
        if (type == "Component") {
            return pyDiagramType::ComponentType;
        }
        if (type == "Package") {
            return pyDiagramType::PackageType;
        }
        if (type == "PackageGroup") {
            return pyDiagramType::PackageGroupType;
        }
        return pyDiagramType::UnknownEntityType;
    }

    [[nodiscard]] std::shared_ptr<pyPhysicalEntityInfoWrapper> parent()
    {
        auto parent = impl.get().parent();
        if (!parent) {
            return nullptr;
        }
        return std::make_shared<pyPhysicalEntityInfoWrapper>(*parent);
    }

    [[nodiscard]] std::vector<std::shared_ptr<pyPhysicalEntityInfoWrapper>> forwardDependencies() const
    {
        auto deps = impl.get().fwdDependencies();
        auto pyFwdDeps = std::vector<std::shared_ptr<pyPhysicalEntityInfoWrapper>>();
        for (auto dep : deps) {
            pyFwdDeps.push_back(std::make_shared<pyPhysicalEntityInfoWrapper>(dep));
        }
        return pyFwdDeps;
    }

    [[nodiscard]] std::vector<std::shared_ptr<pyPhysicalEntityInfoWrapper>> children() const
    {
        auto children = impl.get().children();
        auto pyChildren = std::vector<std::shared_ptr<pyPhysicalEntityInfoWrapper>>();
        for (auto c : children) {
            pyChildren.push_back(std::make_shared<pyPhysicalEntityInfoWrapper>(c));
        }
        return pyChildren;
    }

  private:
    std::reference_wrapper<IPhysicalEntityInfo> impl;
};

// NOLINTBEGIN
PYBIND11_EMBEDDED_MODULE(pycgn, m)
{
    m.doc() = "Module containing entities exposed to python for code generation scripts";

    py::class_<pyPhysicalEntityInfoWrapper, std::shared_ptr<pyPhysicalEntityInfoWrapper>>(m, "PhysicalEntity")
        .def("name", &pyPhysicalEntityInfoWrapper::name, "Physical entity name without namespaces.")
        .def("type",
             &pyPhysicalEntityInfoWrapper::type,
             "Physical entity type (Component, Package, etc.) See DiagramType.")
        .def("parent",
             &pyPhysicalEntityInfoWrapper::parent,
             "Returns the Physical entity in which this entity is contained. If there's no parent, returns None.")
        .def("forwardDependencies",
             &pyPhysicalEntityInfoWrapper::forwardDependencies,
             "Returns a list of entities that this Physical entity depends on.")
        .def("children",
             &pyPhysicalEntityInfoWrapper::children,
             "Returns a list of entities that are children of this entity.");

    py::enum_<pyDiagramType>(m, "DiagramType")
        .value("Component", pyDiagramType::ComponentType)
        .value("Package", pyDiagramType::PackageType)
        .value("PackageGroup", pyDiagramType::PackageGroupType)
        .value("UnknownEntity", pyDiagramType::UnknownEntityType)
        .export_values();
}
// NOLINTEND

cpp::result<void, CodeGenerationError>
CodeGeneration::generateCodeFromScript(const std::string& scriptPath,
                                       const std::string& outputDir,
                                       ICodeGenerationDataProvider& dataProvider,
                                       std::optional<std::function<void(CodeGenerationStep const&)>> callback)
{
    std::filesystem::path path(scriptPath);
    auto modulePath = path.parent_path().string();
    auto moduleName = path.stem().string();

    py::gil_scoped_acquire _;

    auto pySys = py::module_::import("sys");
    pySys.attr("path").attr("append")(modulePath);

    auto pyCgn = py::module_::import("pycgn");
    auto pyUserModuleResult = [&moduleName]() -> cpp::result<py::module_, CodeGenerationError> {
        try {
            return py::module_::import(moduleName.c_str());
        } catch (py::error_already_set const& e) {
            return cpp::fail(CodeGenerationError{CodeGenerationError::Kind::PythonError, e.what()});
        }
    }();
    if (pyUserModuleResult.has_error()) {
        return cpp::fail(pyUserModuleResult.error());
    }
    auto pyUserModule = pyUserModuleResult.value();
    if (!py::hasattr(pyUserModule, "buildPhysicalEntity")) {
        return cpp::fail(CodeGenerationError{CodeGenerationError::Kind::ScriptDefinitionError,
                                             "Expected function named buildPhysicalEntity"});
    }

    try {
        using InfoVec = std::vector<std::reference_wrapper<IPhysicalEntityInfo>>;

        auto user_ctx = py::dict();

        if (py::hasattr(pyUserModule, "beforeProcessEntities")) {
            if (callback) {
                (*callback)(BeforeProcessEntitiesStep{});
            }
            auto const& beforeProcessEntities = pyUserModule.attr("beforeProcessEntities");
            beforeProcessEntities(outputDir, user_ctx);
        }

        auto const& buildPhysicalEntity = pyUserModule.attr("buildPhysicalEntity");
        std::function<void(InfoVec const&)> recursiveBuild = [&](InfoVec const& entities) -> void {
            for (auto refWrapEntity : entities) {
                auto& entity = refWrapEntity.get();
                if (!entity.selectedForCodeGeneration()) {
                    continue;
                }
                if (callback) {
                    (*callback)(ProcessEntityStep{entity.name()});
                }

                buildPhysicalEntity(pyCgn, pyPhysicalEntityInfoWrapper{entity}, outputDir, user_ctx);
                auto children = entity.children();
                recursiveBuild(children);
            }
        };
        recursiveBuild(dataProvider.topLevelEntities());

        if (py::hasattr(pyUserModule, "afterProcessEntities")) {
            if (callback) {
                (*callback)(AfterProcessEntitiesStep{});
            }
            auto const& afterProcessEntities = pyUserModule.attr("afterProcessEntities");
            afterProcessEntities(outputDir, user_ctx);
        }
    } catch (std::runtime_error const& e) {
        return cpp::fail(CodeGenerationError{CodeGenerationError::Kind::PythonError, e.what()});
    }

    return {};
}

} // namespace Codethink::lvtcgn::mdl
