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
#include <ct_lvtplg_handlercontextmenuaction.h>
#include <ct_lvtplg_handlerdockwidget.h>

#include <iostream>
#include <vector>

using EntityUniqueId = std::string;
using HistoryList = std::vector<EntityUniqueId>;

static std::vector<HistoryList> d_cycles;
static auto const DOCK_WIDGET_TITLE = "Cycle detection";
static auto const DOCK_WIDGET_ID = "cyc_detection_plg";
static auto const DOCK_WIDGET_TREE_ID = "cyc_detection_tree";
static auto const ITEM_USER_DATA_CYCLE_ID = "cycle";

void highlightCycles(PluginContextMenuActionHandler *handler);
void hookGraphicsViewContextMenu(PluginContextMenuHandler *handler)
{
    handler->registerContextMenu("Highlight cycles", &highlightCycles);
}

void hookSetupDockWidget(PluginDockWidgetHandler *handler)
{
    handler->createNewDock(DOCK_WIDGET_ID, DOCK_WIDGET_TITLE);
    handler->addTree(DOCK_WIDGET_ID, DOCK_WIDGET_TREE_ID);
}

bool containsPermutation(const HistoryList& cycle, const std::vector<HistoryList>& cycles)
// returns true if cycles contains any permutation of cycle
// cycle must not contain the duplicated node
//
// This is a static method so we can re-use this logic elsewhere
{
    if (cycles.empty()) {
        return false;
    }

    // generate all permutations of the cycle
    std::vector<HistoryList> permutations;
    permutations.reserve(cycle.size());
    for (std::size_t shiftAmount = 0; shiftAmount < cycle.size(); ++shiftAmount) {
        HistoryList permutation;
        permutation.reserve(cycle.size());

        for (std::size_t i = shiftAmount; i < (cycle.size() + shiftAmount); ++i) {
            permutation.push_back(cycle[i % cycle.size()]);
        }
        permutations.push_back(std::move(permutation));
    }

    // check against existing cycles
    for (const HistoryList& oldCycle : cycles) {
        // -1 because oldCycle includes the duplicate node
        if ((oldCycle.size() - 1) != cycle.size()) {
            continue;
        }

        for (const HistoryList& permutation : permutations) {
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

void addCycle(const std::size_t first, const HistoryList& history)
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
    // copy form the first occurance of the duplicate in the history instead
    // of history.begin() so that we do not include any early nodes not
    // nesecarry for the cycle:
    // e.g. output "C B D C" not "A X Y Z C B D C"
    HistoryList cycle;
    cycle.reserve(history.size() - first);
    // don't include the duplicate node on the end yet because that throws
    // off the permutations (which would have different duplicate nodes)
    for (std::size_t i = first; i < history.size() - 1; ++i) {
        cycle.push_back(history[i]);
    }

    // now check if this is just a permutation of an existing cycle
    if (containsPermutation(cycle, d_cycles)) {
        // skip
        return;
    }

    // add duplicate node
    cycle.push_back(cycle.front());
    d_cycles.push_back(std::move(cycle));
}

void traverseDependencies(Entity& e, HistoryList const& history);
void traverse(Entity& node, HistoryList history)
// Recursive depth first search, keeping track of where we have been.
// If we encounter a node we have already seen in this path, that means
// there's a cycle (back edge).
// Passing history by value is delibirate: we want our own copy so that
// it acts like a stack.
{
    bool cycle = false;

    // we want an index instead of an iterator so that it remains valid after
    // appending node to history. Therefore we can't use std::find
    std::size_t i = 0;
    for (i = 0; i < history.size(); ++i) {
        if (history[i] == node.getQualifiedName()) {
            cycle = true;
            break;
        }
    }

    // add the current node to the history
    history.push_back(node.getQualifiedName());

    if (cycle) {
        addCycle(i, history);
        // skip iterating over dependencies because we have visited this
        // node before
        return;
    }

    // continue depth first search
    traverseDependencies(node, history);
}

void traverseDependencies(Entity& e, HistoryList const& history)
{
    for (auto& dependency : e.getDependencies()) {
        traverse(dependency, history);
    }
}

void onRootItemSelected(PluginTreeItemClickedActionHandler *handler);
void highlightCycles(PluginContextMenuActionHandler *handler)
{
    d_cycles.clear();

    for (auto&& e : handler->getAllEntitiesInCurrentView()) {
        auto history = HistoryList{};
        traverse(e, history);
    }

    auto tree = handler->getTree(DOCK_WIDGET_TREE_ID);
    tree.clear();
    for (auto&& cycle : d_cycles) {
        auto firstName = cycle[0];
        auto lastName = cycle[cycle.size() - 1];
        auto rootItem = tree.addRootItem("From " + firstName + " to " + lastName);
        rootItem.addUserData(ITEM_USER_DATA_CYCLE_ID, &cycle);
        rootItem.addOnClickAction(&onRootItemSelected);
        for (auto&& qualifiedName : cycle) {
            auto e = handler->getEntityByQualifiedName(qualifiedName);
            rootItem.addChild(qualifiedName);
        }
    }
}

HistoryList& extractHistoryListFrom(void *userData)
{
    return *static_cast<HistoryList *>(userData);
}

void onRootItemSelected(PluginTreeItemClickedActionHandler *handler)
{
    auto const SELECTED_COLOR = Color{200, 50, 50};
    auto const UNSELECTED_COLOR = Color{200, 200, 200};

    auto selectedItem = handler->getItem();
    auto gv = handler->getGraphicsView();
    auto& cycle = extractHistoryListFrom(selectedItem.getUserData(ITEM_USER_DATA_CYCLE_ID));
    for (auto&& e : gv.getVisibleEntities()) {
        e.setColor(UNSELECTED_COLOR);
    }
    for (auto&& qualifiedName : cycle) {
        auto e = gv.getEntityByQualifiedName(qualifiedName);
        if (e) {
            e->setColor(SELECTED_COLOR);
        }
    }
}
