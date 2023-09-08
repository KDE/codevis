// ct_lvtplg_pluginmanager.t.cpp                                     -*-C++-*-

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

#include <ct_lvtplg_pluginmanager.h>

#include <QDir>
#include <string>

#include <catch2-local-includes.h>
#include <test-project-paths.h>

using namespace Codethink::lvtplg;
namespace py = pybind11;

auto constexpr CPP_TEST_PLUGIN_ID = "cppTestPlugin";
auto constexpr PY_TEST_PLUGIN_ID = "pyTestPlugin";

TEST_CASE("Plugin manager")
{
    auto const pluginsPath = std::string{TEST_PLG_PATH};

    auto pm = PluginManager{};
    pm.loadPlugins(QDir{QString::fromStdString(pluginsPath)});
    REQUIRE(pm.getPluginData(CPP_TEST_PLUGIN_ID) == nullptr);
    REQUIRE(pm.getPluginData(PY_TEST_PLUGIN_ID) == nullptr);
    pm.callHooksSetupPlugin();
    REQUIRE(pm.getPluginData(CPP_TEST_PLUGIN_ID) != nullptr);
    REQUIRE(pm.getPluginData(PY_TEST_PLUGIN_ID) != nullptr);
    REQUIRE(pm.getPluginData("some_other_id") == nullptr);
    pm.callHooksTeardownPlugin();
    REQUIRE(pm.getPluginData(CPP_TEST_PLUGIN_ID) == nullptr);
    REQUIRE(pm.getPluginData(PY_TEST_PLUGIN_ID) == nullptr);

    const QString basicPythonPluginPath =
        QString::fromStdString(pluginsPath) + QDir::separator() + QStringLiteral("basicpythonplugin");

    // Make sure it doesn't crashes reloading the plugin.'
    pm.reloadPlugin(basicPythonPluginPath);
}
