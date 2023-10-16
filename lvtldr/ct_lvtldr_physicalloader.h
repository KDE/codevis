// ct_lvtldr_physicalloader.h                                         -*-C++-*-

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

#ifndef INCLUDED_CT_LVTLDR_PHYSICALLOADER
#define INCLUDED_CT_LVTLDR_PHYSICALLOADER

//@PURPOSE: Load physical graphs from the database
//
//@CLASSES:
//  lvtldr::PhysicalLoader: Load physical graphs from the database
//      This class implements the policy of the loading: controlling what is and
//      is not loaded according to the graph settings. See LakosianNode for the
//      interface with lvtcdb and IGraphLoader for the interface with lvtshr.

#include <lvtldr_export.h>

#include <ct_lvtldr_igraphloader.h>

#include <ct_lvtshr_graphenums.h>
#include <ct_lvtshr_uniqueid.h>

#include <memory>
#include <result/result.hpp>
#include <string>

namespace Codethink::lvtshr {
class LoaderInfo;
}

namespace Codethink::lvtldr {

class NodeStorage;
class LakosianEdge;
class LakosianNode;

// ==========================
// class PhysicalLoader
// ==========================

struct GraphLoadError {
    std::string what;
};

class LVTLDR_EXPORT PhysicalLoader {
  public:
    // TYPES
    enum class ManualRuleKind { Children, Edges };

    void unvisitVertex(LakosianNode *node);
    // Sets a node as 'unvisited' so that the load algorithm can traverse the data again.

  private:
    // TYPES
    struct Private;

    // DATA
    std::unique_ptr<Private> d;

    // MANIPULATORS
    void addVertex(LakosianNode *node, bool withParent, lvtshr::LoaderInfo info);
    // Add a vertex to the diagram

    void visitVertex(LakosianNode *node, unsigned distance, lvtldr::NodeLoadFlags flags);
    // Recursive visitor for loading a graph

  public:
    // CREATORS
    explicit PhysicalLoader(NodeStorage& nodeStorage);

    ~PhysicalLoader() noexcept;

    // MANIPULATORS
    void setGraph(IGraphLoader *graph);
    // Really this should be in the constructor, but that isn't how
    // lvtqtc::GraphicsScene works

    void clear();

    void setMainNode(LakosianNode *node);
    // Set the main node of the graph

    void setExtDeps(bool extDeps);
    // Control if dependencies of dependencies (...of dependencies...) are
    // loaded

    bool isNodeFullyLoaded(LakosianNode *node, lvtldr::NodeLoadFlags flags) const;
    cpp::result<void, GraphLoadError> load(LakosianNode *node, lvtldr::NodeLoadFlags flags);
    // Load the graph from the code database
};

} // namespace Codethink::lvtldr

#endif // INCLUDED_CT_LVTLDR_PHYSICALLOADER
