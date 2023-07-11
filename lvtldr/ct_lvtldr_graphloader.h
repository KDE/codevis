// ct_lvtldr_graphloader.h                                            -*-C++-*-

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

#ifndef INCLUDED_CT_LVTLDR_GRAPHLOADER
#define INCLUDED_CT_LVTLDR_GRAPHLOADER

//@PURPOSE: Load data into an lvtshr::IGraphLoader such that parents are always
//          loaded before children and things are not added multiple times

#include <lvtldr_export.h>

#include <ct_lvtldr_igraphloader.h>
#include <ct_lvtshr_loaderinfo.h>

#include <memory>

namespace Codethink::lvtldr {

class NodeStorage;
class LakosianEdge;
class LakosianNode;

// ==========================
// class IGraphLoader
// ==========================

class LVTLDR_EXPORT GraphLoader {
    // Loads data into an lvtshr::IGraphLoader

    // TYPES
    struct Private;
    struct LoaderVertex;
    struct LoaderEdge;
    struct VertexHash;
    struct EdgeHash;

    // DATA
    std::unique_ptr<Private> d;

    // PRIVATE MODIFIERS
    lvtqtc::LakosEntity *load(LakosianNode *node);
    lvtqtc::LakosEntity *load(const LoaderVertex& vertex);
    void load(const LoaderEdge& edge);

    void addEdge(LakosianNode *source, const LakosianEdge& edge, bool reverse);
    // Add an edge to the internal store. reverse should be true if this is
    // an edge for a reverse dependency

  public:
    explicit GraphLoader();
    ~GraphLoader() noexcept;

    // MODIFIERS
    void setGraph(IGraphLoader *graph);
    // Really this should be in the constructor, but that isn't how
    // lvtqtc::GraphicsScene works. Graph must live at least as long as this
    // object.

    void clear();
    // Empties the internal store of vertices and edges

    void addVertex(LakosianNode *node, bool withParent, lvtshr::LoaderInfo info);
    // add a vertex to our internal store. These may be added in any order
    // and it is safe to call this multiple times with the same node.
    // If we are ever called withParent = true, that node's parent will be
    // loaded no matter if other calls set withParent = true.

    void loadForwardEdges();
    // load all forward edges between the added vertices

    void loadReverseEdges();
    // load all reverse edges between the added vertices

    void unload(LakosianNode *node);
    // we need to be able to unload those when removing children from the view.

    // ACCESSORS
    void load();
    // Load everything into the lvtshr::IGraphLoader
};

} // namespace Codethink::lvtldr

#endif // INCLUDED_CT_LVTLDR_GRAPHLOADER
