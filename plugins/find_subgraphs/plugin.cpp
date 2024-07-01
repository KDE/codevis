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

#include <boost/graph/graph_selectors.hpp>
#include <ct_lvtplg_basicpluginhandlers.h>
#include <ct_lvtplg_basicpluginhooks.h>

#include <QElapsedTimer>
#include <QMap>

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

static auto const ITEM_USER_DATA_CYCLE_ID = std::string{"find_subgraph"};
static auto const NODE_SELECTED_COLOR = Codethink::lvtplg::Color{200, 50, 50};
static auto const NODE_UNSELECTED_COLOR = Codethink::lvtplg::Color{200, 200, 200};
static auto const EDGE_SELECTED_COLOR = Codethink::lvtplg::Color{230, 40, 40};
static auto const EDGE_UNSELECTED_COLOR = Codethink::lvtplg::Color{230, 230, 230};

enum class SelectedState { Selected, NotSelected };

struct VertexProps {
    Codethink::lvtplg::Entity *ptr;
};

using Graph = boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS, VertexProps>;

using Vertex = Graph::vertex_descriptor;
using Filtered = boost::filtered_graph<Graph, boost::keep_all, std::function<bool(Vertex)>>;

struct FindSubgraphPluginData {
    Graph prevSelected;
    std::vector<Graph> ourGraphs;
};

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

/* This does not receives all nodes from the loaded graph, but only the nodes from
 * a closed set (be it a mouse selection, the topmost contents of a package, or the topmost contents of the
 * view.
 */
Graph buildBoostGraph(const std::vector<std::shared_ptr<Codethink::lvtplg::Entity>>& currentClusterNodes)
{
    Graph g;
    QMap<Codethink::lvtplg::Entity *, long> vertices;

    for (const auto& e : currentClusterNodes) {
        if (!vertices.contains(e.get())) {
            vertices.insert(e.get(), add_vertex(VertexProps{e.get()}, g));
        }

        auto ee = vertices.value(e.get());
        for (const auto& j : e->getDependencies()) {
            if (!vertices.contains(j.get())) {
                vertices.insert(j.get(), add_vertex(VertexProps{j.get()}, g));
            }
            auto jj = vertices.value(j.get());

            add_edge(ee, jj, g);
        }
    }

    return g;
}

std::vector<int> map_components(Graph const& g)
{
    std::vector<int> mappings(num_vertices(g));
    size_t res = boost::connected_components(g, &mappings[0]);
    std::cout << "found " << res << " subgraphs";
    return mappings;
}

std::vector<Graph> split(Graph const& g, std::vector<int> const& components)
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
    auto *pluginData = getPluginData(handler);

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
    pluginData->ourGraphs = split(graph, map);

    std::cout << "Looking for subgraphs took " << timer.elapsed() << std::endl;
    std::cout << "Number of subgraphs " << pluginData->ourGraphs.size() << "\n";

    auto tree = handler->getTree(DOCK_WIDGET_TREE_ID);
    tree.clear();

    int curr = 0;
    for (Graph& g : pluginData->ourGraphs) {
        auto rootItem = tree.addRootItem("Subgraph " + std::to_string(curr));
        rootItem.addUserData(ITEM_USER_DATA_CYCLE_ID, &g);
        rootItem.addOnClickAction(&onRootItemSelected);
#if 0
        for (const auto vd : boost::make_iterator_range(boost::vertices(g))) {
            auto entity = g[vd];
            rootItem.addChild(entity.ptr->getQualifiedName());
        }
#endif
        curr += 1;
    }

    auto dock = handler->getDock(DOCK_WIDGET_ID);
    dock.setVisible(true);
}

void onRootItemSelected(PluginTreeItemClickedActionHandler *handler)
{
    auto gv = handler->getGraphicsView();
    auto paintCycle = [&gv](Graph& graph, auto&& state) {
        auto prevQualifiedName = std::optional<std::string>();

        for (const auto vd : boost::make_iterator_range(boost::vertices(graph))) {
            auto entity = graph[vd];
            entity.ptr->setColor(state == SelectedState::Selected ? NODE_SELECTED_COLOR : NODE_UNSELECTED_COLOR);

            if (prevQualifiedName) {
                auto fromQualifiedName = *prevQualifiedName;
                auto toQualifiedName = entity.ptr->getQualifiedName();
                auto edge = gv.getEdgeByQualifiedName(fromQualifiedName, toQualifiedName);
                if (edge) {
                    edge->setColor(state == SelectedState::Selected ? EDGE_SELECTED_COLOR : EDGE_UNSELECTED_COLOR);
                }
            }
            prevQualifiedName = entity.ptr->getQualifiedName();
        }
    };

    auto *pluginData = getPluginData(handler);
    auto selectedItem = handler->getItem();
    auto *selectedGraph = static_cast<Graph *>(selectedItem.getUserData(ITEM_USER_DATA_CYCLE_ID));

    paintCycle(pluginData->prevSelected, SelectedState::NotSelected);
    paintCycle(*selectedGraph, SelectedState::Selected);
    pluginData->prevSelected = *selectedGraph;
}
