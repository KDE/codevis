// ct_lvtldr_graphloader.cpp                                          -*-C++-*-

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

#include <ct_lvtldr_graphloader.h>

#include <ct_lvtldr_lakosiannode.h>

#include <ct_lvtshr_stringhelpers.h>

#include <cassert>
#include <iostream>
#include <unordered_map>
#include <unordered_set>

// TODO: Have a way to enable debugs from application
//       While removing the dependency from ldr to QtGui and QtWidgets, we needed to avoid touching the Preferences
//       module directly, since they depend on those things. We need a way to enable/disable debug messages from this
//       module without touching the Preferences global directly to avoid unwanted dependencies.
//       Old code for reference:
//       Preferences::self()->debug()->enableDebugOutput() || Preferences::self()->debug()->storeDebugOutput();
//
static const bool showDebug = false;

namespace Codethink::lvtldr {

// ==========================
// class IGraphLoader
// ==========================

struct GraphLoader::LoaderVertex {
    LakosianNode *node;

    LakosianNode *parent = nullptr;
    // If this is a nullptr then we don't load the parent, otherwise use the
    // parent pointed to here. This will normally be node->parent(), but
    // for some manual layout interventions it could be
    // node->parent()->parent(), etc

    bool expanded = false;

    lvtshr::LoaderInfo info;

    LoaderVertex(LakosianNode *node, bool withParent, lvtshr::LoaderInfo info): node(node), info(info)
    {
        assert(node);
        if (withParent) {
            parent = node->parent();
        } else {
            parent = nullptr;
        }
    }

    bool operator==(const LoaderVertex& other) const
    // see VertexHash::operator()
    {
        return node == other.node;
    }
};

struct GraphLoader::VertexHash {
    std::size_t operator()(const LoaderVertex& vertex) const
    {
        // One vertex per LakosianNode. The other fields are just metadata
        return std::hash<LakosianNode *>{}(vertex.node);
    }
};

struct GraphLoader::LoaderEdge {
    LakosianNode *source;
    LakosianNode *target;
    lvtshr::LakosRelationType type;

    LoaderEdge(LakosianNode *source, LakosianNode *target, lvtshr::LakosRelationType type):
        source(source), target(target), type(type)
    {
    }

    bool operator==(const LoaderEdge& other) const
    {
        return source == other.source && target == other.target && type == other.type;
    }
};

struct GraphLoader::EdgeHash {
    std::size_t operator()(const LoaderEdge& edge) const
    {
        auto h = std::hash<decltype(edge.source)>{}(edge.source);
        auto h2 = std::hash<decltype(edge.target)>{}(edge.target);
        h = h ^ (h2 << 1);
        auto h3 = std::hash<decltype(edge.type)>{}(edge.type);
        h = h ^ (h3 << 1);
        return h;
    }
};

struct GraphLoader::Private {
    IGraphLoader *graph = nullptr;
    NodeStorage *store = nullptr;

    std::unordered_set<LoaderVertex, VertexHash> vertices;
    std::unordered_set<LoaderEdge, EdgeHash> edges;
    std::unordered_map<LakosianNode *, lvtqtc::LakosEntity *> entityMap;
};

GraphLoader::GraphLoader(): d(std::make_unique<GraphLoader::Private>())
{
}

GraphLoader::~GraphLoader() noexcept = default;

void GraphLoader::unload(LakosianNode *node)
{
    auto mapIt = d->entityMap.find(node);
    if (mapIt != std::end(d->entityMap)) {
        d->entityMap.erase(mapIt);
    }

    auto verticeIt = std::find_if(std::begin(d->vertices), std::end(d->vertices), [node](const LoaderVertex& v) {
        return v.node == node;
    });
    if (verticeIt != std::end(d->vertices)) {
        d->vertices.erase(verticeIt);
    }

    // C++17 lacks a std::remove_if for maps and sets
    std::unordered_set<LoaderEdge, EdgeHash> temp_edges;
    for (const auto& edge : d->edges) {
        if (edge.source == node || edge.target == node) {
            continue;
        }
        temp_edges.insert(edge);
    }
    d->edges = temp_edges;
}

void GraphLoader::setGraph(IGraphLoader *graph)
{
    d->graph = graph;
}

lvtqtc::LakosEntity *GraphLoader::load(LakosianNode *node)
// loop up the LoaderVertex for node then call load() on that
{
    assert(node);

    auto [it, _] = d->vertices.emplace(LoaderVertex(node, false, lvtshr::LoaderInfo(false, false, false)));
    return load(*it);
}

lvtqtc::LakosEntity *GraphLoader::load(const GraphLoader::LoaderVertex& vertex)
{
    if (d->entityMap.count(vertex.node)) {
        return d->entityMap[vertex.node];
    }

    lvtqtc::LakosEntity *parent = nullptr;

    // make sure our parents are added first
    if (vertex.parent) {
        parent = load(vertex.parent);
    }

    qDebug() << "lvtldr::IGraphLoader: adding vertex " << vertex.node->qualifiedName();

    if (parent) {
        qDebug() << " parent: " << vertex.node->parent()->qualifiedName();
    }
    qDebug() << " expanded: " << vertex.expanded;

    lvtqtc::LakosEntity *entity = nullptr;
    if (d->graph) {
        switch (vertex.node->type()) {
        case lvtshr::DiagramType::ClassType:
            entity = d->graph->addUdtVertex(vertex.node, vertex.expanded, parent, vertex.info);
            break;
        case lvtshr::DiagramType::PackageType:
            entity = d->graph->addPkgVertex(vertex.node, vertex.expanded, parent, vertex.info);
            break;
        case lvtshr::DiagramType::RepositoryType:
            entity = d->graph->addRepositoryVertex(vertex.node, vertex.expanded, parent, vertex.info);
            break;
        case lvtshr::DiagramType::ComponentType:
            entity = d->graph->addCompVertex(vertex.node, vertex.expanded, parent, vertex.info);
            break;
        case lvtshr::DiagramType::NoneType:
            break;
        }
    }

    d->entityMap.insert({vertex.node, entity});
    return entity;
}

void GraphLoader::load(const LoaderEdge& edge)
{
    if (!d->graph) {
        return;
    }

    lvtqtc::LakosEntity *source = load(edge.source);
    lvtqtc::LakosEntity *target = load(edge.target);
    assert(source && "Missing source pointer");
    assert(target && "Missing target pointer");

    switch (edge.type) {
    case lvtshr::IsA:
        d->graph->addIsARelation(source, target);
        break;
    case lvtshr::PackageDependency:
        d->graph->addPackageDependencyRelation(source, target);
        break;
    case lvtshr::AllowedDependency:
        d->graph->addAllowedPackageDependencyRelation(source, target);
        break;
    case lvtshr::UsesInTheImplementation:
        d->graph->addUsesInTheImplementationRelation(source, target);
        break;
    case lvtshr::UsesInTheInterface:
        d->graph->addUsesInTheInterfaceRelation(source, target);
        break;
    case lvtshr::UsesInNameOnly:
    case lvtshr::None:
        assert(false && "Unhandled edge type");
    }
}

void GraphLoader::clear()
{
    if (showDebug) {
        qDebug() << "Clearing Inner Loader";
    }
    d->vertices.clear();
    d->edges.clear();
    d->entityMap.clear();
}

void GraphLoader::addVertex(LakosianNode *node, bool withParent, lvtshr::LoaderInfo info)
{
    assert(node);

    auto [it, inserted] = d->vertices.emplace(node, withParent, info);
    if (inserted && showDebug) {
        qDebug() << "GraphLoader: Added node " << QString::fromStdString(node->qualifiedName());
    }

    if (!inserted && withParent) {
        // const_cast is safe here because this does not effect the hash
        const_cast<LoaderVertex&>(*it).info = info;
        if (!it->parent) {
            const_cast<LoaderVertex&>(*it).parent = node->parent();
            const_cast<LoaderVertex&>(*it).info.setHasParent(true);
        }
    }

    if (it->parent) {
        // make sure parent knows it needs to be a subgraph
        auto [parentIt, _] = d->vertices.emplace(node->parent(), false, lvtshr::LoaderInfo(false, false, false));
        // const_cast is safe here because expanded does not effect the hash value
        const_cast<LoaderVertex&>(*parentIt).expanded = true;
        // don't set hasChildren in the LoaderInfo because we might not have
        // *all* of the children
    }
}

void GraphLoader::addEdge(LakosianNode *source, const LakosianEdge& edge, bool reverse)
{
    LakosianNode *dest = edge.other();
    if (reverse) {
        std::swap(source, dest);
    }

    d->edges.emplace(source, dest, edge.type());
}

void GraphLoader::loadForwardEdges()
{
    for (const LoaderVertex& vertex : d->vertices) {
        for (const LakosianEdge& edge : vertex.node->providers()) {
            // if we are drawing the other side of the edge anyway, we should
            // draw the edge
            if (d->vertices.count({edge.other(), false, {}})) {
                addEdge(vertex.node, edge, false);
            }
        }
    }
}

void GraphLoader::loadReverseEdges()
{
    for (const LoaderVertex& vertex : d->vertices) {
        for (const LakosianEdge& edge : vertex.node->clients()) {
            // if we are drawing the other side of the edge anyway, we should
            // draw the edge
            if (d->vertices.count({edge.other(), false, {}})) {
                addEdge(vertex.node, edge, true);
            }
        }
    }
}

void GraphLoader::load()
{
    // set parent for any vertices which have their parent loaded but don't
    // know about it
    for (const LoaderVertex& vertex : d->vertices) {
        if (vertex.parent) {
            // parent already set
            continue;
        }

        LakosianNode *parent = vertex.node->parent();
        while (parent) {
            // if the node's parent has already been added, set withParent
            auto parentIt = d->vertices.find(LoaderVertex(parent, false, {}));
            if (parentIt != d->vertices.end()) {
                // const_cast safe because it doesn't change the hash value
                const_cast<LoaderVertex&>(vertex).parent = parent;
                const_cast<LoaderVertex&>(*parentIt).expanded = true;
                break;
            }

            // try grandparent's etc
            parent = parent->parent();
        }
    }

    for (const LoaderVertex& vertex : d->vertices) {
        // the vertex will add its own parents if these were not already added
        load(vertex);
    }

    for (const LoaderEdge& edge : d->edges) {
        load(edge);
    }
}

} // namespace Codethink::lvtldr
