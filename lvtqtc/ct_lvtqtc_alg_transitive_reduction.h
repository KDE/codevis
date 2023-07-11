// ct_lvtqtc_alg_transitive_reduction.h                             -*-C++-*-

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

#ifndef INCLUDED_CT_LVTQTC_ALG_TRANSITIVE_REDUCTION
#define INCLUDED_CT_LVTQTC_ALG_TRANSITIVE_REDUCTION

#include <QElapsedTimer>
#include <QObject>
#include <QString>
#include <QThreadPool>

#include <memory>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "lvtqtc_export.h"

namespace Codethink::lvtldr {
class LakosianNode;
}

namespace Codethink::lvtqtc {
class LakosRelation;
class LakosEntity;
struct EdgeCollection;

class LVTQTC_EXPORT AlgorithmTransitiveReduction : public QObject {
    // Inherits QObject for QObject::tr()
    Q_OBJECT
  public:
    void setVertices(std::vector<LakosEntity *> entities);
    // the list of the vertices we will run the algorithm on.

    void run();

    bool hasRun() const;
    bool hasError() const;
    long long totalRunTime() const;

    QString errorMessage() const;

    std::unordered_map<LakosEntity *, std::vector<std::shared_ptr<EdgeCollection>>>& redundantEdgesByNode();
    // Returns a reference so we don't need to copy it all the time.
    // be careful not to hold to the reference.

    void reset();
    // reset the result of the algorithm and make it usable again.

  private:
    // Algorithm Steps.

    void startLookingForRedundantEdgesOnNode(LakosEntity *entity);
    // for each direct connection from `entity` to any other entity,
    // we have a redundant relation if we find a longer path that also
    // connects `entity` to its direct connection.
    // if so, the direct connecction is a redundant connection.

    bool
    recursiveTransitiveSearch(LakosEntity *target, LakosEntity *current, std::unordered_set<LakosEntity *>& visited);
    // target is the direct connection from the original entity
    // current is where we are right now on the graph tree.
    // if current == target, that means we found a redundant edge.

    class RunnableThread;

    std::shared_mutex d_collectionSerialiser;
    std::unordered_map<LakosEntity *, std::vector<std::shared_ptr<EdgeCollection>>> d_redundantEdgesByNode;
    std::unordered_set<std::shared_ptr<EdgeCollection>> d_redundantEdges;
    std::vector<LakosEntity *> d_entities;

    using InputHashKey = unsigned long long;
    using CachedOutput = std::vector<std::tuple<lvtldr::LakosianNode *, lvtldr::LakosianNode *>>;
    std::unordered_map<InputHashKey, CachedOutput> d_redundantEdgesCache;

    QElapsedTimer d_totalTimer;
    // total time it took to run the algorithm.

    QString d_errorMessage;

    QThreadPool d_pool;

    bool d_hasRun = false;
    bool d_hasError = false;

    long long d_totalTime = 0;
};

} // namespace Codethink::lvtqtc

#endif
