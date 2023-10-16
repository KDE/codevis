// ct_lvtldr_physicalloader.cpp                                       -*-C++-*-

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

#include <ct_lvtldr_physicalloader.h>

#include <ct_lvtldr_graphloader.h>
#include <ct_lvtldr_lakosiannode.h>
#include <ct_lvtldr_nodestorage.h>

#include <ct_lvtshr_stringhelpers.h>

#include <cassert>
#include <optional>
#include <unordered_set>

// TODO: Have a way to enable debugs from application
//       While removing the dependency from ldr to QtGui and QtWidgets, we needed to avoid touching the Preferences
//       module directly, since they depend on those things. We need a way to enable/disable debug messages from this
//       module without touching the Preferences global directly to avoid unwanted dependencies.
//       Old code for reference:
//       Preferences::self()->debug()->enableDebugOutput() || Preferences::self()->debug()->storeDebugOutput();
//
static const bool showDebug = false;

namespace {

using namespace Codethink::lvtldr;

struct VisitLog {
    // logs which calls to visitVertex we have already processed
    LakosianNode *node;

    // needed for std::find.
    explicit VisitLog(LakosianNode *node): node(node)
    {
    }
};

struct VisitLogHash {
    std::size_t operator()(const VisitLog& log) const
    {
        return std::hash<LakosianNode *>{}(log.node);
        // one log per LakosianNode
    }
};

inline bool operator==(const VisitLog& lhs, const VisitLog& rhs)
{
    // only check nodes - see VisitLogHash
    return lhs.node == rhs.node;
}

} // namespace

namespace Codethink::lvtldr {

struct PhysicalLoader::Private {
    NodeStorage& nodeStorage;

    lvtshr::DiagramType type = lvtshr::DiagramType::NoneType;
    std::string qualifiedName;
    LakosianNode *mainNode = nullptr;

    bool extDeps = false;

    GraphLoader loader;
    std::unordered_set<VisitLog, VisitLogHash> visited;

    IGraphLoader *graphLoader = nullptr;

    explicit Private(NodeStorage& nodeStorage): nodeStorage(nodeStorage)
    {
    }
};

PhysicalLoader::PhysicalLoader(NodeStorage& nodeStorage): d(std::make_unique<Private>(nodeStorage))
{
}

PhysicalLoader::~PhysicalLoader() noexcept = default;

void PhysicalLoader::addVertex(LakosianNode *node, bool withParent, lvtshr::LoaderInfo info)
{
    assert(node);

    d->loader.addVertex(node, withParent, info);

    if (showDebug) {
        qDebug() << "lvtldr::PhysicalLoader::addVertex: " << node->qualifiedName() << ": " << withParent;
    }
}

void PhysicalLoader::setGraph(IGraphLoader *graph)
{
    d->loader.setGraph(graph);
    d->graphLoader = graph;
}

void PhysicalLoader::clear()
{
    d->visited.clear();
    d->loader.clear();
}

void PhysicalLoader::setMainNode(LakosianNode *node)
{
    d->mainNode = node;
    if (d->mainNode) {
        d->type = d->mainNode->type();
        d->qualifiedName = d->mainNode->qualifiedName();
    }
}

void PhysicalLoader::setExtDeps(bool extDeps)
{
    d->extDeps = extDeps;
}

cpp::result<void, GraphLoadError> PhysicalLoader::load(LakosianNode *node, lvtldr::NodeLoadFlags flags)
{
    // Each call to load() will *not* clean the previous graph - this will be used as a cache during the scene
    // visualization. When the user selects a *completely* new graph then, we clean the view.

    if (!node) {
        return cpp::fail(GraphLoadError{"Trying to load a null node"});
    }

    visitVertex(node, 0, flags);

    d->loader.loadReverseEdges();
    d->loader.loadForwardEdges();

    d->loader.load();

    return {};
}

void PhysicalLoader::unvisitVertex(LakosianNode *node)
{
    d->loader.unload(node);

    auto it = d->visited.find(VisitLog{node});
    if (it == std::end(d->visited)) {
        return;
    }

    d->visited.erase(it);
}

bool PhysicalLoader::isNodeFullyLoaded(LakosianNode *node, lvtldr::NodeLoadFlags flags) const
{
    if (flags.loadChildren) {
        for (LakosianNode *child : node->children()) {
            if (d->visited.find(VisitLog{child}) == d->visited.end()) {
                return false;
            }
        }
    }

    if (flags.traverseProviders) {
        for (const LakosianEdge& edge : node->providers()) {
            if (d->visited.find(VisitLog{edge.other()}) == d->visited.end()) {
                return false;
            }
        }
    }

    if (flags.traverseClients) {
        for (const LakosianEdge& edge : node->clients()) {
            if (d->visited.find(VisitLog{edge.other()}) == d->visited.end()) {
                return false;
            }
        }
    }

    return true;
}

void PhysicalLoader::visitVertex(LakosianNode *node, const unsigned distance, lvtldr::NodeLoadFlags flags)
{
    assert(node);

    auto [it, inserted] = d->visited.emplace(node);
    if (!inserted) {
        if (isNodeFullyLoaded(node, flags)) {
            if (showDebug) {
                qDebug() << "Node " << node->qualifiedName() << " is fully loaded, exiting";
            }
            return;
        }
    }

    if (showDebug) {
        qDebug() << "Loading node " << node->qualifiedName();
    }

    bool loadParent = node->parent();
    if (loadParent) {
        // 99: distance parameter so high that we won't even think about loading
        // edges
        const bool hasParent = d->visited.find(VisitLog{node->parent()}) != std::end(d->visited);
        if (!hasParent) {
            constexpr unsigned MAX_DISTANCE = 99;
            auto visitFlags = d->graphLoader->loadFlagsFor(node);
            visitVertex(node->parent(), MAX_DISTANCE, visitFlags);
        }
    }

    const bool loadChildren = flags.loadChildren;
    const lvtshr::LoaderInfo loaderInfo(loadChildren || node->children().empty(), loadParent || !node->parent(), false);

    addVertex(node, loadParent, loaderInfo);

    // load dependencies
    if (flags.traverseProviders) {
        for (const LakosianEdge& edge : node->providers()) {
            auto visitFlags = d->graphLoader->loadFlagsFor(edge.other());
            visitVertex(edge.other(), distance + 1, visitFlags);
        }
    }

    if (flags.traverseClients) {
        for (const LakosianEdge& edge : node->clients()) {
            auto visitFlags = d->graphLoader->loadFlagsFor(edge.other());
            visitVertex(edge.other(), distance + 1, visitFlags);
        }
    }

    // load children
    if (loadChildren) {
        for (LakosianNode *child : node->children()) {
            auto visitFlags = d->graphLoader->loadFlagsFor(child);
            visitVertex(child, distance, visitFlags);
        }
    }
}

} // namespace Codethink::lvtldr
