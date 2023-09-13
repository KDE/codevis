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
#include <ct_lvtplg_handlersetup.h>

#include <algorithm>
#include <utils.h>

enum class ToggleTestEntitiesState { VISIBLE, HIDDEN };
static auto const BAD_TEST_DEPENDENCY_COLOR = Color{255, 20, 20};

struct PluginData {
    static auto constexpr ID = "LKS_TEST_RULES_PLG";

    ToggleTestEntitiesState toggleState = ToggleTestEntitiesState::VISIBLE;
    std::vector<std::string> toggledTestEntities;
};

template<typename Handler_t>
PluginData *getPluginData(Handler_t *handler)
{
    return static_cast<PluginData *>(handler->getPluginData(PluginData::ID));
}

void hookSetupPlugin(PluginSetupHandler *handler)
{
    handler->registerPluginData(PluginData::ID, new PluginData{});
}

void hookTeardownPlugin(PluginSetupHandler *handler)
{
    auto *data = getPluginData(handler);
    handler->unregisterPluginData(PluginData::ID);
    delete data;
}

void toggleTestEntities(PluginContextMenuActionHandler *handler)
{
    auto *pluginData = getPluginData(handler);
    if (pluginData->toggleState == ToggleTestEntitiesState::VISIBLE) {
        pluginData->toggledTestEntities.clear();
        for (auto&& e : handler->getAllEntitiesInCurrentView()) {
            if (utils::string{e.getQualifiedName()}.endswith(".t")) {
                pluginData->toggledTestEntities.push_back(e.getQualifiedName());
                e.unloadFromScene();
            }
        }
        pluginData->toggleState = ToggleTestEntitiesState::HIDDEN;
    } else if (pluginData->toggleState == ToggleTestEntitiesState::HIDDEN) {
        for (auto&& qName : pluginData->toggledTestEntities) {
            handler->loadEntityByQualifiedName(qName);
        }
        pluginData->toggledTestEntities.clear();
        pluginData->toggleState = ToggleTestEntitiesState::VISIBLE;
    }
}

void paintBadTestComponents(PluginContextMenuActionHandler *handler)
{
    for (auto&& e : handler->getAllEntitiesInCurrentView()) {
        if (!utils::string{e.getQualifiedName()}.endswith(".t")) {
            continue;
        }

        auto& testDriver = e;
        auto component = handler->getEntityByQualifiedName(utils::string{testDriver.getQualifiedName()}.split('.')[0]);
        if (!component) {
            // If we can't find the test driver's CUT (Component-Under-Test), then skip.
            continue;
        }

        // CUT (Component-Under-Test)
        auto& cut = *component;
        for (auto&& dependency : testDriver.getDependencies()) {
            // It is ok for the test driver to depend on the CUT
            if (dependency.getName() == cut.getName()) {
                continue;
            }

            // It is ok for the test driver to depend on things that the CUT
            // also depends on ("redundant dependencies").
            if (std::ranges::any_of(cut.getDependencies(), [&](auto&& cutDependency) {
                    return cutDependency.getName() == dependency.getName();
                })) {
                continue;
            }

            // All other dependencies are marked as "invalid"
            auto edge = handler->getEdgeByQualifiedName(testDriver.getQualifiedName(), dependency.getQualifiedName());
            if (edge) {
                edge->setColor(BAD_TEST_DEPENDENCY_COLOR);
            }
        }
    }
}

void hookGraphicsViewContextMenu(PluginContextMenuHandler *handler)
{
    handler->registerContextMenu("Toggle test entities", &toggleTestEntities);
    handler->registerContextMenu("Search invalid test dependencies", &paintBadTestComponents);
}
