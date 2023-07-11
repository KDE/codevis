// ct_lvtldr_igraphloader.h                                       -//-C++-//-

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

#ifndef INCLUDED_CT_LVTSHR_IGRAPHLOADER
#define INCLUDED_CT_LVTSHR_IGRAPHLOADER

#include <lvtldr_export.h>

#include <ct_lvtshr_graphenums.h>
#include <ct_lvtshr_graphstorage.h>
#include <ct_lvtshr_loaderinfo.h>

#include <optional>
#include <vector>

namespace Codethink::lvtqtc {
class LakosEntity;
class LakosRelation;
} // namespace Codethink::lvtqtc

namespace Codethink::lvtldr {
class LakosianNode;

struct NodeLoadFlags {
    /* Each visible node loaded has this member, controlling what the loader
       will try to load. The default load is just "load yourself", no childrens
       and no dependencies. When we want more data from the loader, we flip the bit,
       and request the loader to reload this.
       It's important to note that this is *only* about *graphical* representation,
       so we can't save this on the PhysicalNode *nor* on the NodeStorage.
       This should be used *only* on the GraphicsScene or Scene Items.
    */

    // For all kinds of packages and logical entities.
    bool loadChildren = false;

    // for packages
    bool traverseProviders = false;
    bool traverseClients = false;

    // for logical entities
    bool loadIsARelationships = false;
    bool loadUsesInTheImplementationRelationships = false;
    bool loadUsesInTheInterfaceRelationships = false;
};

class LVTLDR_EXPORT IGraphLoader {
    // This class defines what we need to implement on classes that load graphs visually
  public:
    virtual ~IGraphLoader();

    virtual void clearGraph() = 0;
    // removes all nodes and edges from the current representation

    virtual lvtqtc::LakosEntity *addUdtVertex(lvtldr::LakosianNode *node,
                                              bool selected = false,
                                              lvtqtc::LakosEntity *parent = nullptr,
                                              lvtshr::LoaderInfo info = {}) = 0;
    // Adds a Lakos Logical Entity to the graph
    //
    // Takes a UserDefinedType instance and adds a LogicalEntity
    // for it to the graph if not already present. Returns a
    // descriptor to the Vertex in the graph.

    virtual lvtqtc::LakosEntity *addRepositoryVertex(lvtldr::LakosianNode *node,
                                                     bool selected = false,
                                                     lvtqtc::LakosEntity *parent = nullptr,
                                                     lvtshr::LoaderInfo info = {}) = 0;

    virtual lvtqtc::LakosEntity *addPkgVertex(lvtldr::LakosianNode *node,
                                              bool selected = false,
                                              lvtqtc::LakosEntity *parent = nullptr,
                                              lvtshr::LoaderInfo info = {}) = 0;
    // Adds a Lakos Package Entity to the graph
    //
    // Takes a SourcePackage instance and adds a PackageEntity
    // for it to the graph if not already present. Returns a
    // descriptor to the Vertex in the graph.

    virtual lvtqtc::LakosEntity *addCompVertex(lvtldr::LakosianNode *node,
                                               bool selected = false,
                                               lvtqtc::LakosEntity *parent = nullptr,
                                               lvtshr::LoaderInfo info = {}) = 0;
    // Adds a Lakos Compnent Entity to the graph
    //
    // Takes a SourceComponent instance and adds a ComponentEntity
    // for it to the graph if not already present. Returns a
    // descriptor to the Vertex in the graph.

    virtual lvtqtc::LakosRelation *addIsARelation(lvtqtc::LakosEntity *parentDescriptor,
                                                  lvtqtc::LakosEntity *childDescriptor) = 0;
    // Adds a IsA Relation to the graph.
    //
    // If no Edge exists for the start and end vertices and
    // new Edge is created. An EdgeDescriptor to the Edge is
    // returned

    virtual lvtqtc::LakosRelation *addUsesInTheInterfaceRelation(lvtqtc::LakosEntity *source,
                                                                 lvtqtc::LakosEntity *target) = 0;
    // Adds a UsesInTheInterfaceRelation to the graph.
    //
    // If no Edge exists for the start and end vertices and
    // new Edge is created. An EdgeDescriptor to the Edge is
    // returned

    virtual lvtqtc::LakosRelation *addUsesInTheImplementationRelation(lvtqtc::LakosEntity *source,
                                                                      lvtqtc::LakosEntity *target) = 0;
    // Adds a UsesInTheImplementation relation to the graph.
    //
    // If no Edge exists for the start and end vertices and
    // new Edge is created. An EdgeDescriptor to the Edge is
    // returned

    virtual lvtqtc::LakosRelation *addPackageDependencyRelation(lvtqtc::LakosEntity *source,
                                                                lvtqtc::LakosEntity *target) = 0;
    // Adds a PackageDependency Relation to the graph.
    //
    // If no Edge exists for the start and end vertices and
    // new Edge is created. An EdgeDescriptor to the Edge is
    // returned

    virtual lvtqtc::LakosRelation *addAllowedPackageDependencyRelation(lvtqtc::LakosEntity *source,
                                                                       lvtqtc::LakosEntity *target) = 0;

    virtual NodeLoadFlags loadFlagsFor(LakosianNode *node) const = 0;
};

} // end namespace Codethink::lvtldr

#endif
