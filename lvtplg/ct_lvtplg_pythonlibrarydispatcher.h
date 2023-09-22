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
#include <ct_lvtplg_handlercodeanalysis.h>
#include <ct_lvtplg_handlercontextmenuaction.h>
#include <ct_lvtplg_handlerdockwidget.h>
#include <ct_lvtplg_handlerentityreport.h>
#include <ct_lvtplg_handlergraphicsview.h>
#include <ct_lvtplg_handlersetup.h>
#include <ct_lvtplg_handlertreewidget.h>
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
    class PyResolveContext : public ILibraryDispatcher::ResolveContext {
      public:
        PyResolveContext(py::module_& pyModule, std::string const& functionName);
        PyResolveContext(PyResolveContext&) = delete;
        PyResolveContext(PyResolveContext&&) = delete;
        ~PyResolveContext();

        static py::module_ *activeModule;
        py::gil_scoped_acquire acquiredGil;
    };

    static constexpr auto name = "PythonLib";
    static const std::string pluginDataId;

    std::unique_ptr<ResolveContext> resolve(std::string const& functionName) override;
    std::string fileName() override;
    void reload() override;

    static bool isValidPlugin(QDir const& pluginDir);
    static std::unique_ptr<ILibraryDispatcher> loadSinglePlugin(QDir const& pluginDir);
    static bool reloadSinglePlugin(QDir const& pluginDir);

  private:
    py::module_ pyModule;
};

} // namespace Codethink::lvtplg

#endif // DIAGRAM_SERVER_CT_LVTPLG_PYTHONLIBRARYDISPATCHER_H
