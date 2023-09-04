// ct_lvtplg_pythonlibrarydispatcher.t.cpp                           -*-C++-*-

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

#include <QDir>
#include <catch2-local-includes.h>
#include <ct_lvttst_tmpdir.h>
#include <string>

using namespace Codethink::lvtplg;
namespace py = pybind11;

static auto constexpr PLUGIN_CONTENTS =
    ""
    "def hookSetupPlugin(handler):"
    "    pass"
    "";

TEST_CASE("Python dispatcher")
{
    py::scoped_interpreter py;
    py::gil_scoped_release defaultReleaseGIL;

    auto pluginDir = TmpDir{std::string{"myplugin"}};
    auto plgDirAsQDir = QDir{QString::fromStdString(pluginDir.path().string())};

    REQUIRE_FALSE(PythonLibraryDispatcher::isValidPlugin(plgDirAsQDir));
    (void) pluginDir.createTextFile("myplugin.py", PLUGIN_CONTENTS);
    (void) pluginDir.createTextFile("README.md", "");
    REQUIRE(PythonLibraryDispatcher::isValidPlugin(plgDirAsQDir));
    auto pluginLib = PythonLibraryDispatcher::loadSinglePlugin(plgDirAsQDir);
    {
        py::gil_scoped_acquire _;

        auto *setupHook = reinterpret_cast<hookSetupPlugin_f>(pluginLib->resolve("hookSetupPlugin"));
        REQUIRE(setupHook != nullptr);

        auto handler = PluginSetupHandler{
            [](auto _1, auto _2) {},
            [&pluginLib](std::string const& id) {
                return pluginLib->getPluginData();
            },
            [](auto _1) {},
        };
        setupHook(&handler);

        auto *notImplementedHook = reinterpret_cast<hookTeardownPlugin_f>(pluginLib->resolve("hookTeardownPlugin"));
        REQUIRE(notImplementedHook == nullptr);
    }
}
