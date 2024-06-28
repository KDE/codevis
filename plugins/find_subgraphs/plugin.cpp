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
#include <string>
#include <vector>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/biconnected_components.hpp> // articulation_points
#include <boost/graph/connected_components.hpp> // connected_components
#include <boost/graph/copy.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/graph_utility.hpp>
#include <boost/graph/named_graph.hpp>
#include <boost/graph/properties.hpp>

static auto const PLUGIN_DATA_ID = std::string{"find_subgraph_plugin"};
static auto const DOCK_WIDGET_TITLE = std::string{"Find Subgraph"};
static auto const DOCK_WIDGET_ID = std::string{"find_subgraph_plugin_dock"};
static auto const DOCK_WIDGET_TREE_ID = std::string{"find_subgraph_plugin_tree"};

#if 0
static auto const ITEM_USER_DATA_CYCLE_ID = std::string{"find_subgraph"};
static auto const NODE_SELECTED_COLOR = Codethink::lvtplg::Color{200, 50, 50};
static auto const NODE_UNSELECTED_COLOR = Codethink::lvtplg::Color{200, 200, 200};
static auto const EDGE_SELECTED_COLOR = Codethink::lvtplg::Color{230, 40, 40};
static auto const EDGE_UNSELECTED_COLOR = Codethink::lvtplg::Color{230, 230, 230};
#endif

enum class SelectedState { Selected, NotSelected };

struct FindSubgraphPluginData { };

template<typename Handler_t>
FindSubgraphPluginData *getPluginData(Handler_t *handler)
{
    return static_cast<FindSubgraphPluginData *>(handler->getPluginData(PLUGIN_DATA_ID));
}

void hookSetupPlugin(PluginSetupHandler *handler)
{
    handler->registerPluginData(PLUGIN_DATA_ID, new FindSubgraphPluginData{});
}

void hookTeardownPlugin(PluginSetupHandler *handler)
{
    auto *data = getPluginData(handler);
    handler->unregisterPluginData(PLUGIN_DATA_ID);
    delete data;
}

void findSubgraphsToplevel(PluginContextMenuActionHandler *handler);
void hookGraphicsViewContextMenu(PluginContextMenuHandler *handler)
{
    handler->registerContextMenu("Find Subgraphs", &findSubgraphsToplevel);
}

void hookSetupDockWidget(PluginSetupDockWidgetHandler *handler)
{
    auto dockHandler = handler->createNewDock(DOCK_WIDGET_ID, DOCK_WIDGET_TITLE);
    dockHandler.addTree(DOCK_WIDGET_TREE_ID);
}

void onRootItemSelected(PluginTreeItemClickedActionHandler *handler);

using Graph = boost::adjacency_list<boost::vecS,
                                    boost::vecS,
                                    boost::bidirectionalS,
                                    boost::property<boost::vertex_name_t, Codethink::lvtplg::Entity *>>;
using Vertex = Graph::vertex_descriptor;
using Filtered = boost::filtered_graph<Graph, boost::keep_all, std::function<bool(Vertex)>>;
using ComponentId = int;
using Mappings = std::vector<ComponentId>;

/* This does not receives all nodes from the loaded graph, but only the nodes from
 * a closed set (be it a mouse selection, the topmost contents of a package, or the topmost contents of the
 * view.
 */
Graph buildBoostGraph(const std::vector<std::shared_ptr<Codethink::lvtplg::Entity>>& currentClusterNodes)
{
    Graph g;
    for (const auto& e : currentClusterNodes) {
        auto ee = add_vertex(e.get(), g);
        for (const auto& j : e->getDependencies()) {
            auto jj = add_vertex(j.get(), g);
            add_edge(ee, jj, g);
        }
    }

    return g;
}

Mappings map_components(Graph const& g)
{
    Mappings mappings(num_vertices(g));
    int num = boost::connected_components(g, mappings.data());
    std::cout << "found" << num << "subgraphs";
    return mappings;
}

std::vector<Graph> split(Graph const& g, Mappings const& components)
{
    if (components.empty()) {
        return {};
    }

    std::vector<Graph> results;

    auto highest = *std::max_element(components.begin(), components.end());
    for (int c = 0; c <= highest; ++c) {
        results.emplace_back();
        boost::copy_graph(Filtered(g,
                                   {},
                                   [c, &components](Vertex v) {
                                       return components.at(v) == c;
                                   }),
                          results.back());
    }

    return results;
}

void findSubgraphsToplevel(PluginContextMenuActionHandler *handler)
{
    //    auto *pluginData = getPluginData(handler);

    QElapsedTimer timer;
    timer.start();
    std::cout << "Starting to look for subgraphs" << std::endl;

    std::vector<Graph> subgraphs;

    auto entities = [handler]() -> std::vector<std::shared_ptr<Codethink::lvtplg::Entity>> {
        std::vector<std::shared_ptr<Codethink::lvtplg::Entity>> _entities;
        for (auto e : handler->getAllEntitiesInCurrentView()) {
            if (!e->getParent()) {
                _entities.push_back(e);
            }
        }
        return _entities;
    }();

    auto graph = buildBoostGraph(entities);
    auto map = map_components(graph);
    auto graphs = split(graph, map);
    subgraphs.insert(subgraphs.end(), graphs.begin(), graphs.end());

    std::cout << "Looking for subgraphs took" << timer.elapsed() << std::endl;

#if 0
    auto tree = handler->getTree(DOCK_WIDGET_TREE_ID);
    tree.clear();

    for (auto&& cycle : allCycles) {
        auto firstName = cycle[0]->getQualifiedName();
        auto lastName = cycle[cycle.size() - 1]->getQualifiedName();
        auto rootItem = tree.addRootItem("From " + firstName + " to " + lastName);
        rootItem.addUserData(ITEM_USER_DATA_CYCLE_ID, &cycle);
        rootItem.addOnClickAction(&onRootItemSelected);
        for (auto&& entity : cycle) {
            rootItem.addChild(entity->getQualifiedName());
        }
    }

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
#endif
}

void onRootItemSelected(PluginTreeItemClickedActionHandler *handler)
{
#if 0
    auto gv = handler->getGraphicsView();
    auto paintCycle = [&gv](auto&& cycle, auto&& state) {
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
    auto selectedItem = handler->getItem();
    auto& selectedCycle = extractCycleFrom(selectedItem.getUserData(ITEM_USER_DATA_CYCLE_ID));
    paintCycle(pluginData->prevSelectedCycle, SelectedState::NotSelected);
    paintCycle(selectedCycle, SelectedState::Selected);
    pluginData->prevSelectedCycle = selectedCycle;
#endif
}
