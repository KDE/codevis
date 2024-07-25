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

#include <QElapsedTimer>
#include <iostream>
#include <map>
#include <string>
#include <vector>

using Cycle = std::vector<Codethink::lvtplg::Entity *>;

static auto const PLUGIN_DATA_ID = std::string{"cycle_detection_plugin"};
static auto const DOCK_WIDGET_TITLE = std::string{"Cycle detection"};
static auto const DOCK_WIDGET_ID = std::string{"cyc_detection_plg"};
static auto const DOCK_WIDGET_TREE_ID = std::string{"cyc_detection_tree"};
static auto const ITEM_USER_DATA_CYCLE_ID = std::string{"cycle"};
static auto const NODE_SELECTED_COLOR = Codethink::lvtplg::Color{200, 50, 50};
static auto const NODE_UNSELECTED_COLOR = Codethink::lvtplg::Color{200, 200, 200};
static auto const EDGE_SELECTED_COLOR = Codethink::lvtplg::Color{230, 40, 40};
static auto const EDGE_UNSELECTED_COLOR = Codethink::lvtplg::Color{230, 230, 230};
enum class SelectedState { Selected, NotSelected };

// Specific to a single GraphicsView
struct CycleDetectionData {
    Cycle selectedCycle;
    Cycle prevSelectedCycle;
    std::vector<Cycle> allCycles;
};

struct CycleDetectionPluginData {
    // The graph scene currently being used.
    std::string currentGraphScene;
    // Scene Name, Graph.
    std::map<std::string, CycleDetectionData> cyclesByScene;
};

template<typename Handler_t>
CycleDetectionPluginData *getPluginData(Handler_t *handler)
{
    return static_cast<CycleDetectionPluginData *>(handler->getPluginData(PLUGIN_DATA_ID));
}

void hookSetupPlugin(PluginSetupHandler *handler)
{
    handler->registerPluginData(PLUGIN_DATA_ID, new CycleDetectionPluginData{});
}

void hookTeardownPlugin(PluginSetupHandler *handler)
{
    auto *data = getPluginData(handler);
    handler->unregisterPluginData(PLUGIN_DATA_ID);
    delete data;
}

void highlightCycles(PluginContextMenuActionHandler *handler);
void hookGraphicsViewContextMenu(PluginContextMenuHandler *handler)
{
    handler->registerContextMenu("Highlight cycles", &highlightCycles);
}

void hookSetupDockWidget(PluginSetupDockWidgetHandler *handler)
{
    auto dockHandler = handler->createNewDock(DOCK_WIDGET_ID, DOCK_WIDGET_TITLE);
    dockHandler.addTree(DOCK_WIDGET_TREE_ID);
}

bool containsPermutation(const Cycle& cycle, const std::vector<Cycle>& allCycles)
// returns true if cycles contains any permutation of cycle
// cycle must not contain the duplicated node
//
// This is a static method so we can re-use this logic elsewhere
{
    if (allCycles.empty()) {
        return false;
    }

    // generate all permutations of the cycle
    std::vector<Cycle> permutations;
    permutations.reserve(cycle.size());
    for (std::size_t shiftAmount = 0; shiftAmount < cycle.size(); ++shiftAmount) {
        Cycle permutation;
        permutation.reserve(cycle.size());

        for (std::size_t i = shiftAmount; i < (cycle.size() + shiftAmount); ++i) {
            permutation.push_back(cycle[i % cycle.size()]);
        }
        permutations.push_back(std::move(permutation));
    }

    // check against existing cycles
    for (const Cycle& oldCycle : allCycles) {
        // -1 because oldCycle includes the duplicate node
        if ((oldCycle.size() - 1) != cycle.size()) {
            continue;
        }

        for (const Cycle& permutation : permutations) {
            // std::equal will only check the first oldCycle.size() - 1 nodes
            // because that is the size of permutation
            // (therefore skipping the duplicate node)
            if (std::equal(permutation.begin(), permutation.end(), oldCycle.begin())) {
                // a permutation of this cycle was already recorded
                return true;
            }
        }
    }

    return false;
}

void addCycle(const std::size_t first, const Cycle& history, std::vector<Cycle>& allCycles)
// Polishes up a history list to be the minimum possible to provide
// nice output of the cycles
//
// first is the index in history of the first instance of the duplicate
// node
//
// We are careful not to add different permutations of the same cycle.
// Doing this is slow but we only pay for it for each cycle permutation
// found in the graph.
{
    // copy form the first occurrence of the duplicate in the history instead
    // of history.begin() so that we do not include any early nodes not
    // necessary for the cycle:
    // e.g. output "C B D C" not "A X Y Z C B D C"
    Cycle maybeNewCycle;
    maybeNewCycle.reserve(history.size() - first);
    // don't include the duplicate node on the end yet because that throws
    // off the permutations (which would have different duplicate nodes)
    for (std::size_t i = first; i < history.size() - 1; ++i) {
        maybeNewCycle.push_back(history[i]);
    }

    // now check if this is just a permutation of an existing cycle
    if (containsPermutation(maybeNewCycle, allCycles)) {
        // skip
        return;
    }

    // add duplicate node
    maybeNewCycle.push_back(maybeNewCycle.front());
    allCycles.push_back(std::move(maybeNewCycle));
}

void traverseDependencies(std::shared_ptr<Codethink::lvtplg::Entity>& e,
                          Cycle& maybeCycle,
                          std::vector<Cycle>& allCycles);

void traverse(std::shared_ptr<Codethink::lvtplg::Entity>& node, Cycle& maybeCycle, std::vector<Cycle>& allCycles)
// Recursive depth first search, keeping track of where we have been.
// If we encounter a node we have already seen in this path, that means
// there's a cycle (back edge).
// Passing history by value is delibirate: we want our own copy so that
// it acts like a stack.
{
    bool hasCycle = false;

    // we want an index instead of an iterator so that it remains valid after
    // appending node to history. Therefore, we can't use std::find
    std::size_t i = 0;
    for (i = 0; i < maybeCycle.size(); ++i) {
        if (maybeCycle[i] == node.get()) {
            hasCycle = true;
            break;
        }
    }

    // add the current node to the history
    maybeCycle.push_back(node.get());

    if (hasCycle) {
        addCycle(i, maybeCycle, allCycles);
        // skip iterating over dependencies because we have visited this
        // node before
        maybeCycle.pop_back();
        return;
    }

    // continue depth first search
    traverseDependencies(node, maybeCycle, allCycles);
    maybeCycle.pop_back();
}

void traverseDependencies(std::shared_ptr<Codethink::lvtplg::Entity>& e,
                          Cycle& maybeCycle,
                          std::vector<Cycle>& allCycles)
{
    for (auto& dependency : e->getDependencies()) {
        traverse(dependency, maybeCycle, allCycles);
    }
}

void onRootItemSelected(PluginTreeItemClickedActionHandler *handler);

void updateTreeView(PluginTreeWidgetHandler tree, CycleDetectionPluginData *pluginData)
{
    tree.clear();
    auto& allCycles = pluginData->cyclesByScene[pluginData->currentGraphScene].allCycles;
    for (auto& cycle : allCycles) {
        auto firstName = cycle[0]->getQualifiedName();
        auto lastName = cycle[cycle.size() - 1]->getQualifiedName();
        auto rootItem = tree.addRootItem("From " + firstName + " to " + lastName);
        rootItem.addUserData(ITEM_USER_DATA_CYCLE_ID, &cycle);
        rootItem.addOnClickAction(&onRootItemSelected);
        for (auto&& entity : cycle) {
            rootItem.addChild(entity->getQualifiedName());
        }
    }
}

void highlightCycles(PluginContextMenuActionHandler *handler)
{
    auto *pluginData = getPluginData(handler);
    auto& cycleData = pluginData->cyclesByScene[pluginData->currentGraphScene];

    QElapsedTimer timer;
    timer.start();
    std::cout << "Starting to look for cycles" << std::endl;

    cycleData.allCycles.clear();
    cycleData.prevSelectedCycle.clear();
    cycleData.selectedCycle.clear();

    for (auto&& e : handler->getAllEntitiesInCurrentView()) {
        auto maybeCycle = Cycle{};
        traverse(e, maybeCycle, cycleData.allCycles);
    }

    if (cycleData.allCycles.empty()) {
        return;
    }

    std::cout << "Looking for cycles took" << timer.elapsed() << std::endl;

    auto treeWidget = handler->getTree(DOCK_WIDGET_TREE_ID);
    updateTreeView(treeWidget, pluginData);

    for (auto const& e0 : handler->getAllEntitiesInCurrentView()) {
        e0->setColor(NODE_UNSELECTED_COLOR);
        for (auto const& e1 : e0->getDependencies()) {
            auto edge = handler->getEdgeByQualifiedName(e0->getQualifiedName(), e1->getQualifiedName());
            if (edge.has_value()) {
                edge->setColor(EDGE_UNSELECTED_COLOR);
            }
        }
    }

    auto dock = handler->getDock(DOCK_WIDGET_ID);
    dock.setVisible(true);
}

Cycle& extractCycleFrom(void *userData)
{
    return *static_cast<Cycle *>(userData);
}

void onRootItemSelected(PluginTreeItemClickedActionHandler *handler)
{
    auto gv = handler->getGraphicsView();
    auto paintCycle = [&gv](const std::vector<Codethink::lvtplg::Entity *>& cycle, auto&& state) {
        auto prevQualifiedName = std::optional<std::string>();
        for (auto&& entity : cycle) {
            if (!entity) {
                continue;
            }
            entity->setColor(state == SelectedState::Selected ? NODE_SELECTED_COLOR : NODE_UNSELECTED_COLOR);

            if (prevQualifiedName) {
                auto fromQualifiedName = *prevQualifiedName;
                auto toQualifiedName = entity->getQualifiedName();
                auto edge = gv.getEdgeByQualifiedName(fromQualifiedName, toQualifiedName);
                if (edge) {
                    edge->setColor(state == SelectedState::Selected ? EDGE_SELECTED_COLOR : EDGE_UNSELECTED_COLOR);
                }
            }
            prevQualifiedName = entity->getQualifiedName();
        }
    };

    auto *pluginData = getPluginData(handler);
    auto& cycleData = pluginData->cyclesByScene[pluginData->currentGraphScene];

    auto selectedItem = handler->getItem();
    auto& selectedCycle = extractCycleFrom(selectedItem.getUserData(ITEM_USER_DATA_CYCLE_ID));
    paintCycle(cycleData.prevSelectedCycle, SelectedState::NotSelected);
    paintCycle(selectedCycle, SelectedState::Selected);
    cycleData.prevSelectedCycle = selectedCycle;
}

void hookActiveSceneChanged(PluginActiveSceneChangedHandler *handler)
{
    auto *pluginData = getPluginData(handler);
    auto treeWidget = handler->getTree(DOCK_WIDGET_TREE_ID);
    pluginData->currentGraphScene = handler->getSceneName();

#ifndef PLUGIN_TEST_CODE
    updateTreeView(treeWidget, pluginData);
#endif
}

void hookSceneDestroyed(PluginSceneDestroyedHandler *handler)
{
    auto *pluginData = getPluginData(handler);
    auto treeWidget = handler->getTree(DOCK_WIDGET_TREE_ID);
    std::string sceneName = handler->getSceneName();

    treeWidget.clear();
    pluginData->cyclesByScene.erase(sceneName);
}
