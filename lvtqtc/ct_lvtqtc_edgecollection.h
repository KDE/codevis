// ct_lvtqtc_edgecollection.h                                        -*-C++-*-

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

#ifndef INCLUDED_CT_LVTQTC_EDGECOLLECTION
#define INCLUDED_CT_LVTQTC_EDGECOLLECTION

#include <lvtqtc_export.h>

#include <memory>
#include <vector>

#include <QPointF>

#include <ct_lvtshr_graphstorage.h>

namespace Codethink::lvtqtc {

struct Vertex;
class LakosRelation;

/*! \struct Edge edge.cpp  edge.h
 *  \brief Represents an Edge in the LakosGraph
 *
 * %Edge contains one or more LakosRelation and the
 * center points of the source and target vertices
 * which are Lakos Entities.
 */
struct LVTQTC_EXPORT EdgeCollection {
    // This enum describes how the edge will be painted on screen
    enum RelationFlags {
        RelationFlagsNone = 0x0,
        RelationIsSelected = 0x1, /* the edge is selected */
        RelationIsCyclic = 0x2, /* the edge belongs to a cycle */
        RelationIsParentHovered = 0x4, /* the parent of this edge is hovered */
        RelationIsHighlighted = 0x8, /* an algorithm is highlightning the relation */
    };

    enum class PointFrom { SOURCE, PARENT };
    enum class PointTo { TARGET, PARENT };

    EdgeCollection();

    ~EdgeCollection();

    LakosRelation *addRelation(LakosRelation *relation);
    // Adds a relation to the relations vector
    //
    // A relation is not adding of one of the same type and
    // direction already exists in the vector.
    //

    void layoutRelations();
    // Position the relations on the screen.

    void setPointFrom(PointFrom entity);
    void setPointTo(PointTo entity);
    // Those to functions changes how the edge is displayed
    // if the edge belongs to a node inside of a package, making
    // it "toParent", means that the edge now will look like as if
    // it belongs to the pacakge, and not to the internal node.
    // those functions are important to the expand / shrink mechanism,
    // as we need to change where it's pointing to.

    void setHighlighted(bool highlighted);
    // changes the color of the edges on the collection to a setHighlighted color.

    void setVisible(bool v);

    [[nodiscard]] std::vector<LakosRelation *> relations() const;
    // all the relations from this Collection

    [[nodiscard]] LakosEntity *from() const;
    [[nodiscard]] LakosEntity *to() const;

    void setFrom(LakosEntity *from);
    void setTo(LakosEntity *to);

    void toggleRelationFlags(RelationFlags flags, bool toggle);

    void removeEdge(LakosRelation *edge);

    void setRedundant(bool redundant);
    [[nodiscard]] bool isRedundant() const;
    // a redundant edge is an edge that if removed, does not changes
    // the meaning of the graph. but it's still important to be able to
    // access them in runtime since we could potentially be trying to
    // look for redundancy and ways to remove those in code.

  private:
    struct Private;
    std::unique_ptr<Private> d;

    void layoutSingleEdge(LakosRelation *relation, double dx, double dy);
    // Applies the layout algorithm on the edge.
};

} // end namespace Codethink::lvtqtc

#endif
