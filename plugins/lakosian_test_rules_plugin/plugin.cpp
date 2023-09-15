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
#include <map>
#include <utils.h>

enum class ToggleTestEntitiesState { VISIBLE, HIDDEN };
enum class MergeTestDependenciesOnCUT { YES, NO };
static auto const BAD_TEST_DEPENDENCY_COLOR = Color{225, 225, 0};

using SceneId = std::string;

struct ToggleDataState {
    std::string mainEntityQualifiedName;
    ToggleTestEntitiesState toggleState = ToggleTestEntitiesState::VISIBLE;
    std::vector<std::string> toggledTestEntities;
    std::vector<std::tuple<std::string, std::string>> testOnlyEdges;
};

struct PluginData {
    static auto constexpr ID = "LKS_TEST_RULES_PLG";

    std::map<SceneId, ToggleDataState> toggleDataState;
    SceneId activeSceneId;
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

void hookActiveSceneChanged(PluginActiveSceneChangedHandler *handler)
{
    auto *pluginData = getPluginData(handler);
    pluginData->activeSceneId = handler->getSceneName();
}

void hookMainNodeChanged(PluginMainNodeChangedHandler *handler)
{
    auto *pluginData = getPluginData(handler);

    auto isSameEntitySelected = pluginData->toggleDataState[handler->getSceneName()].mainEntityQualifiedName
        == handler->getEntity().getQualifiedName();
    if (isSameEntitySelected) {
        return;
    }

    pluginData->activeSceneId = handler->getSceneName();
    pluginData->toggleDataState[pluginData->activeSceneId] = {};
}

void toggleMergeTestEntitiesHelper(PluginContextMenuActionHandler *handler,
                                   MergeTestDependenciesOnCUT keepTestEntitiesOnCUT)
{
    auto *pluginData = getPluginData(handler);
    auto& activeToggleDataState = pluginData->toggleDataState[pluginData->activeSceneId];
    auto& toggleState = activeToggleDataState.toggleState;
    auto& toggledTestEntities = activeToggleDataState.toggledTestEntities;
    auto& testOnlyEdges = activeToggleDataState.testOnlyEdges;

    if (toggleState == ToggleTestEntitiesState::VISIBLE) {
        toggledTestEntities.clear();
        for (auto&& e : handler->getAllEntitiesInCurrentView()) {
            if (utils::string{e.getQualifiedName()}.endswith(".t")) {
                auto& testDriver = e;

                // Create test-only dependencies coming from the component
                auto component =
                    handler->getEntityByQualifiedName(utils::string{testDriver.getQualifiedName()}.split('.')[0]);
                if (component) {
                    for (auto&& dependency : testDriver.getDependencies()) {
                        auto from = component->getQualifiedName();
                        auto to = dependency.getQualifiedName();

                        if (!handler->hasEdgeByQualifiedName(from, to)) {
                            testOnlyEdges.emplace_back(from, to);
                        }
                        if (keepTestEntitiesOnCUT == MergeTestDependenciesOnCUT::YES) {
                            auto newEdge = handler->addEdgeByQualifiedName(from, to);
                            if (newEdge) {
                                newEdge->setColor(BAD_TEST_DEPENDENCY_COLOR);
                                newEdge->setStyle(EdgeStyle::DotLine);
                            }
                        }
                    }
                }

                toggledTestEntities.push_back(e.getQualifiedName());
                e.unloadFromScene();
            }
        }
        toggleState = ToggleTestEntitiesState::HIDDEN;
    } else if (toggleState == ToggleTestEntitiesState::HIDDEN) {
        for (auto&& [e0, e1] : testOnlyEdges) {
            handler->removeEdgeByQualifiedName(e0, e1);
        }
        testOnlyEdges.clear();

        for (auto&& qName : toggledTestEntities) {
            handler->loadEntityByQualifiedName(qName);
        }
        toggledTestEntities.clear();

        toggleState = ToggleTestEntitiesState::VISIBLE;
    }
}

void toggleTestEntities(PluginContextMenuActionHandler *handler)
{
    toggleMergeTestEntitiesHelper(handler, MergeTestDependenciesOnCUT::NO);
}

void toggleMergeTestEntities(PluginContextMenuActionHandler *handler)
{
    toggleMergeTestEntitiesHelper(handler, MergeTestDependenciesOnCUT::YES);
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
            auto deps = cut.getDependencies();
            if (std::any_of(deps.begin(), deps.end(), [&](auto&& cutDependency) {
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
    handler->registerContextMenu("Toggle merge test entities with their components", &toggleMergeTestEntities);
    handler->registerContextMenu("Mark invalid test dependencies", &paintBadTestComponents);
}
