// ct_lvtqtc_alg_transitive_reduction.cpp                             -*-C++-*-

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

#include <ct_lvtqtc_alg_transitive_reduction.h>

#include <ct_lvtqtc_edgecollection.h>
#include <ct_lvtqtc_lakosentity.h>
#include <ct_lvtqtc_lakosrelation.h>

#include <memory>

#include <QDebug>
#include <QRunnable>

namespace {
constexpr bool debugMode = false;
// start to spit information on the command line.

QString S(const std::string& string)
{
    return QString::fromStdString(string);
}
} // namespace

using namespace Codethink::lvtqtc;

class AlgorithmTransitiveReduction::RunnableThread : public QRunnable {
    // The ancient version of Qt in appimage (5.11) doesn't support creating a
    // QRunnable from a lambda so we have to write a lambda by hand here :(
  private:
    // PRIVATE DATA
    AlgorithmTransitiveReduction& d_parent;
    LakosEntity *d_entity;

  public:
    RunnableThread(AlgorithmTransitiveReduction& parent, LakosEntity *entity): d_parent(parent), d_entity(entity)
    {
        setAutoDelete(true);
    }

    ~RunnableThread() override = default;

    // MUTATORS
    void run() override
    {
        d_parent.startLookingForRedundantEdgesOnNode(d_entity);
    }
};

void AlgorithmTransitiveReduction::setVertices(std::vector<LakosEntity *> entities)
{
    d_entities = std::move(entities);
}

bool AlgorithmTransitiveReduction::hasRun() const
{
    return d_hasRun;
}

bool AlgorithmTransitiveReduction::hasError() const
{
    return d_hasError;
}

QString AlgorithmTransitiveReduction::errorMessage() const
{
    return d_errorMessage;
}

std::unordered_map<LakosEntity *, std::vector<std::shared_ptr<EdgeCollection>>>&
AlgorithmTransitiveReduction::redundantEdgesByNode()
{
    return d_redundantEdgesByNode;
}

long long AlgorithmTransitiveReduction::totalRunTime() const
{
    return d_totalTime;
}

void AlgorithmTransitiveReduction::reset()
{
    d_hasRun = false;
    d_hasError = false;
    d_redundantEdgesByNode.clear();
    d_redundantEdges.clear();
    d_redundantEdgesCache.clear();
}

void AlgorithmTransitiveReduction::run()
{
    if (d_entities.empty()) {
        return;
    }

    reset();

    d_hasRun = true;
    d_totalTimer.start();

    auto cacheKey = std::uintptr_t{0};
    for (auto *e : d_entities) {
        cacheKey += reinterpret_cast<std::uintptr_t>(e->internalNode());
    }
    if (d_redundantEdgesCache.count(cacheKey) > 0) {
        for (auto const& entity : d_entities) {
            for (const std::shared_ptr<EdgeCollection>& maybeRedundant : entity->edgesCollection()) {
                auto& cache = d_redundantEdgesCache[cacheKey];
                auto edge =
                    std::tuple<lvtldr::LakosianNode *, lvtldr::LakosianNode *>{maybeRedundant->from()->internalNode(),
                                                                               maybeRedundant->to()->internalNode()};
                auto it = std::find_if(cache.cbegin(), cache.cend(), [&edge](auto const& e) {
                    return edge == e;
                });
                if (it == cache.cend()) {
                    continue;
                }

                d_redundantEdgesByNode[entity].push_back(maybeRedundant);
                d_redundantEdges.insert(maybeRedundant);
            }
        }
    } else {
        for (LakosEntity *entity : d_entities) {
            // if the current entity has zero edges, it's impossible
            // that it has redundant edges. If it has just one edge,
            // the edge can't be redundant as there's no alternative
            // way to go to the node.
            const auto eSize = entity->edgesCollection().size();
            if (eSize == 0 || eSize == 1) {
                if (debugMode) {
                    qDebug() << "entity" << S(entity->name()) << "can't have redundant edges";
                }
                continue;
            }

            // if we have more than one edge, one of them could be a redundant
            // edge. so let's search.
            QRunnable *runnable = new RunnableThread(*this, entity);
            d_pool.start(runnable);
        }
        d_pool.waitForDone();

        // Save to cache
        for (auto const& e : d_redundantEdges) {
            d_redundantEdgesCache[cacheKey].push_back({e->from()->internalNode(), e->to()->internalNode()});
        }
    }

    for (const std::shared_ptr<EdgeCollection>& redundant : d_redundantEdges) {
        redundant->setRedundant(true);
    }

    d_totalTime = d_totalTimer.elapsed();
    if (debugMode) {
        qDebug() << "Transitive Reduction took" << d_totalTime;
    }
}

void AlgorithmTransitiveReduction::startLookingForRedundantEdgesOnNode(LakosEntity *entity)
{
    // here we iterate over all edges, twice.
    // the outer loop is where we will have the target entity, the
    // value we are looking for, the inner loop is the start of the
    // lookup.
    // the edgesCollection is necessarily the edges that are going from
    // the entity to other entities.
    if (debugMode) {
        qDebug() << "Starting to look for reduntant edges on node" << S(entity->name());
    }

    for (const std::shared_ptr<EdgeCollection>& maybeRedundant : entity->edgesCollection()) {
        if (debugMode) {
            qDebug() << "Testing to see if the edge " << S(entity->name()) << S(maybeRedundant->to()->name())
                     << "is redundant";
        }
        for (const std::shared_ptr<EdgeCollection>& lookupStart : entity->edgesCollection()) {
            // ignore the lookupStart if it's the same edge as the `maybeRedundant`.
            if (maybeRedundant == lookupStart) {
                continue;
            }
            // This edge is already on the redundant set, ignore.
            {
                std::shared_lock<std::shared_mutex> guard(d_collectionSerialiser);
                if (d_redundantEdges.find(lookupStart) != std::end(d_redundantEdges)) {
                    continue;
                }
            }

            // We are at the start of the search of transitiveness for the edge
            // `maybeRedundant`. Setup the initial data here, and start to walk
            // the graph. If we find a cycle, abort.
            if (debugMode) {
                qDebug() << "Reseting visited";
            }

            std::unordered_set<LakosEntity *> visited;
            LakosEntity *maybeReduntantNode = maybeRedundant->to();
            LakosEntity *currentToNode = lookupStart->to();
            visited.insert(lookupStart->from());
            const bool found = recursiveTransitiveSearch(maybeReduntantNode, currentToNode, visited);
            if (found) {
                if (debugMode) {
                    qDebug() << "Found redundant edge between" << QString::fromStdString(entity->name()) << "and"
                             << QString::fromStdString(maybeReduntantNode->name());
                }

                // An edge collection has 0 - N edges between the same two
                // nodes. It's usually 1 though.
                {
                    std::unique_lock<std::shared_mutex> guard(d_collectionSerialiser);
                    d_redundantEdgesByNode[entity].push_back(maybeRedundant);
                    d_redundantEdges.insert(maybeRedundant);
                }
            }
        }
    }
}

bool AlgorithmTransitiveReduction::recursiveTransitiveSearch(LakosEntity *target,
                                                             LakosEntity *current,
                                                             std::unordered_set<LakosEntity *>& visited)
{
    if (target == current) {
        return true;
    }

    // if it's already visited, we are on a loop, and we need to break away from it.
    if (visited.find(current) != std::end(visited)) {
        return false;
    }

    if (debugMode) {
        qDebug() << "Adding" << S(current->name()) << "To visited";
    }

    visited.insert(current);
    for (const std::shared_ptr<EdgeCollection>& edge : current->edgesCollection()) {
        LakosEntity *nextNode = edge->to();

        // This edge is already on the redundant set, so just abort here.
        {
            std::shared_lock<std::shared_mutex> guard(d_collectionSerialiser);
            if (d_redundantEdges.find(edge) != std::end(d_redundantEdges)) {
                continue;
            }
        }

        if (debugMode) {
            if (!nextNode) {
                qDebug() << "Entity" << S(current->name()) << " lacks a `to` node on the edge collection";
            } else {
                qDebug() << "The next node from" << S(current->name()) << "is" << S(nextNode->name());
            }
        }

        const bool found = recursiveTransitiveSearch(target, nextNode, visited);
        if (found) {
            return true;
        }
    }

    return false;
}
