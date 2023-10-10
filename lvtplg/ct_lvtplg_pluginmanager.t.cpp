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

auto constexpr CPP_TEST_PLUGIN_ID = "cppTestPlugin";
auto constexpr PY_TEST_PLUGIN_ID = "pyTestPlugin";

TEST_CASE("Plugin manager")
{
    auto const pluginsPath = std::string{TEST_PLG_PATH};

    auto pm = PluginManager{};
    pm.loadPlugins({QString::fromStdString(pluginsPath)});

    // Make sure all plugins are enabled for this test
    pm.getPluginById(CPP_TEST_PLUGIN_ID)->get().setEnabled(true);
    pm.getPluginById(PY_TEST_PLUGIN_ID)->get().setEnabled(true);

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

    // Check workflow with disabled plugin
    pm.getPluginById(CPP_TEST_PLUGIN_ID)->get().setEnabled(false);
    pm.getPluginById(PY_TEST_PLUGIN_ID)->get().setEnabled(true);
    pm.callHooksSetupPlugin();
    REQUIRE(pm.getPluginData(CPP_TEST_PLUGIN_ID) == nullptr);
    REQUIRE(pm.getPluginData(PY_TEST_PLUGIN_ID) != nullptr);
    pm.callHooksTeardownPlugin();
    REQUIRE(pm.getPluginData(PY_TEST_PLUGIN_ID) == nullptr);

    pm.getPluginById(CPP_TEST_PLUGIN_ID)->get().setEnabled(true);
    pm.getPluginById(PY_TEST_PLUGIN_ID)->get().setEnabled(true);
    pm.callHooksSetupPlugin();
    REQUIRE(pm.getPluginData(CPP_TEST_PLUGIN_ID) != nullptr);
    REQUIRE(pm.getPluginData(PY_TEST_PLUGIN_ID) != nullptr);
    pm.callHooksTeardownPlugin();
    REQUIRE(pm.getPluginData(CPP_TEST_PLUGIN_ID) == nullptr);
    REQUIRE(pm.getPluginData(PY_TEST_PLUGIN_ID) == nullptr);
}

TEST_CASE("Unload plugin")
{
    auto const pluginsPath = std::string{TEST_PLG_PATH};
    auto pm = PluginManager{};
    pm.loadPlugins({QString::fromStdString(pluginsPath)});

    // TODO: Unload C++ plugins
    // pm.getPluginById(CPP_TEST_PLUGIN_ID)->get().setEnabled(true);
    pm.getPluginById(PY_TEST_PLUGIN_ID)->get().setEnabled(true);
    pm.callHooksSetupPlugin();
    REQUIRE(pm.getPluginData(PY_TEST_PLUGIN_ID) != nullptr);

    const QString basicPythonPluginPath =
        QString::fromStdString(pluginsPath) + QDir::separator() + QStringLiteral("basicpythonplugin");
    pm.removePlugin(basicPythonPluginPath);
    REQUIRE(pm.getPluginData(PY_TEST_PLUGIN_ID) == nullptr);
}
