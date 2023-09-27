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
#include <unordered_set>
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

void handleVisibleCase(PluginContextMenuActionHandler *handler, MergeTestDependenciesOnCUT keepTestEntitiesOnCUT)
{
    auto *pluginData = getPluginData(handler);
    auto& activeToggleDataState = pluginData->toggleDataState[pluginData->activeSceneId];
    auto& toggleState = activeToggleDataState.toggleState;
    auto& toggledTestEntities = activeToggleDataState.toggledTestEntities;
    auto& testOnlyEdges = activeToggleDataState.testOnlyEdges;

    toggledTestEntities.clear();
    for (auto&& e : handler->getAllEntitiesInCurrentView()) {
        if (!utils::string{e.getQualifiedName()}.endswith(".t")) {
            continue;
        }
        auto& testDriver = e;

        // Create test-only dependencies coming from the component
        auto component = handler->getEntityByQualifiedName(utils::string{testDriver.getQualifiedName()}.split('.')[0]);
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
    toggleState = ToggleTestEntitiesState::HIDDEN;
}

void handleHiddenCase(PluginContextMenuActionHandler *handler)
{
    auto *pluginData = getPluginData(handler);
    auto& activeToggleDataState = pluginData->toggleDataState[pluginData->activeSceneId];
    auto& toggleState = activeToggleDataState.toggleState;
    auto& toggledTestEntities = activeToggleDataState.toggledTestEntities;
    auto& testOnlyEdges = activeToggleDataState.testOnlyEdges;

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

void toggleMergeTestEntitiesHelper(PluginContextMenuActionHandler *handler,
                                   MergeTestDependenciesOnCUT keepTestEntitiesOnCUT)
{
    auto *pluginData = getPluginData(handler);
    auto& activeToggleDataState = pluginData->toggleDataState[pluginData->activeSceneId];
    auto& toggleState = activeToggleDataState.toggleState;
    auto& toggledTestEntities = activeToggleDataState.toggledTestEntities;
    auto& testOnlyEdges = activeToggleDataState.testOnlyEdges;

    switch (toggleState) {
    case ToggleTestEntitiesState::VISIBLE:
        handleVisibleCase(handler, keepTestEntitiesOnCUT);
        break;
    case ToggleTestEntitiesState::HIDDEN:
        handleHiddenCase(handler);
        break;
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
    for (auto const& e : handler->getAllEntitiesInCurrentView()) {
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
        for (auto const& dependency : testDriver.getDependencies()) {
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

void ignoreTestOnlyDependenciesPkgLvl(PluginContextMenuActionHandler *handler)
{
    auto getPkgId = [handler](Entity const& pkg) -> std::optional<long long> {
        auto result = handler->runQueryOnDatabase("SELECT id FROM source_package WHERE qualified_name = \""
                                                  + pkg.getQualifiedName() + "\"");
        if (!result.empty() && !result[0].empty() && result[0][0].has_value()) {
            return std::any_cast<int>(result[0][0].value());
        }
        return std::nullopt;
    };

    for (auto const& e : handler->getAllEntitiesInCurrentView()) {
        if (e.getType() != EntityType::Package) {
            continue;
        }

        auto& srcPkg = e;
        auto maybeSrcPkgId = getPkgId(srcPkg);
        if (!maybeSrcPkgId) {
            continue;
        }
        auto srcPkgId = *maybeSrcPkgId;
        for (auto const& trgPkg : srcPkg.getDependencies()) {
            // There is a package-level dependency between srcPkg and trgPkg. The code below finds out if it is a
            // test-only dependency (meaning, only *.t packages on srcPkg depends on a package on trgPkg).
            // Test-only dependencies will then be removed at package level (for visualization only).
            auto maybeTrgPkgId = getPkgId(trgPkg);
            if (!maybeTrgPkgId) {
                continue;
            }
            auto trgPkgId = *maybeTrgPkgId;
            auto query = R"(
    SELECT COUNT(*)
    FROM source_component c0
    JOIN source_component c1
    INNER JOIN dependencies pkg_d ON pkg_d.source_id = c0.package_id AND pkg_d.target_id = c1.package_id
    INNER JOIN component_relation cmp_d ON cmp_d.source_id = c0.id AND cmp_d.target_id = c1.id
    WHERE 1
    AND c0.name NOT LIKE "%.t"
    AND pkg_d.source_id = ")"
                + std::to_string(srcPkgId) + R"("
    AND pkg_d.target_id = ")"
                + std::to_string(trgPkgId) + R"("
)";
            auto result = handler->runQueryOnDatabase(query);
            if (!result.empty() && !result[0].empty() && result[0][0].has_value()) {
                auto nNonTestDependencies = std::stoi(std::any_cast<std::string>(result[0][0].value()));
                if (nNonTestDependencies == 0) {
                    // There are only test dependencies between packages.
                    handler->removeEdgeByQualifiedName(srcPkg.getQualifiedName(), trgPkg.getQualifiedName());
                }
            }
        }
    }
}

enum class ContextMenuType { ComponentScene, PackageScene };

void hookGraphicsViewContextMenu(PluginContextMenuHandler *handler)
{
    auto ctxMenuType = ContextMenuType::PackageScene;
    for (auto const& e : handler->getAllEntitiesInCurrentView()) {
        if (e.getType() == EntityType::Component) {
            ctxMenuType = ContextMenuType::ComponentScene;
            break;
        }
    }

    if (ctxMenuType == ContextMenuType::PackageScene) {
        handler->registerContextMenu("Ignore test only dependencies", &ignoreTestOnlyDependenciesPkgLvl);
    } else {
        handler->registerContextMenu("Toggle test entities", &toggleTestEntities);
        handler->registerContextMenu("Toggle merge test entities with their components", &toggleMergeTestEntities);
        handler->registerContextMenu("Mark invalid test dependencies", &paintBadTestComponents);
    }
}
