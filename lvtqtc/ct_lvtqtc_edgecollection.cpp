// ct_lvtqtc_edgecollection.cpp                                       -*-C++-*-

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

#include <ct_lvtqtc_edgecollection.h>
#include <ct_lvtqtc_lakosentity.h>

// TODO: circular component dependency
#include <ct_lvtqtc_lakosrelation.h>

#include <QDebug>

#include <cmath>

namespace Codethink::lvtqtc {

static constexpr double INTER_RELATION_SPACING = 8.0;

struct EdgeCollection::Private {
    /*! \brief weight of the Edge
     */
    double weight = 1.0;

    /*! \brief A list of LakosRelation instances
     */
    std::vector<LakosRelation *> relations;
    // All of the relations on this collection.

    LakosEntity *from = nullptr;
    LakosEntity *to = nullptr;

    PointFrom pointFrom = PointFrom::SOURCE;
    PointTo pointTo = PointTo::TARGET;

    bool isRedundant = false;
};

EdgeCollection::EdgeCollection(): d(std::make_unique<EdgeCollection::Private>())
{
}

EdgeCollection::~EdgeCollection() = default;

// TODO: This is a hack to make the filter correctly delete the memory if needed.
// try to come with a better algorithm for this.
void deleter(LakosRelation *relation)
{
    delete relation;
}

LakosEntity *EdgeCollection::from() const
{
    return d->from;
}

LakosEntity *EdgeCollection::to() const
{
    return d->to;
}

std::vector<LakosRelation *> EdgeCollection::relations() const
{
    return d->relations;
}

void EdgeCollection::setFrom(LakosEntity *from)
{
    d->from = from;
}

void EdgeCollection::setTo(LakosEntity *to)
{
    d->to = to;
}

LakosRelation *EdgeCollection::addRelation(LakosRelation *relation)
{
    auto *filtered_relation = lvtshr::filterEdgeFromCollection<LakosRelation *>(d->relations, relation, deleter);
    if (filtered_relation) {
        return filtered_relation;
    }

    d->relations.push_back(relation);
    // The edges with the most relations should be the shortest,
    // so reduce the weight each time a relation is added to the
    // edge.
    d->weight -= 0.15;
    return relation;
}

void EdgeCollection::layoutSingleEdge(LakosRelation *relation, double dx, double dy)
{
    if (relation->parentItem()) {
        // We need the item center point in scene coordinates. so first we get
        // the point, then we map to the view.
        // TODO: Check if we can do less transformations.
        auto relationSource = d->from->boundingRect().translated(d->from->pos()).translated(dx, dy).center();
        relationSource = d->from->parentItem()->mapToScene(relationSource);
        relationSource = relation->parentItem()->mapFromScene(relationSource);

        auto relationTarget = d->to->boundingRect().translated(d->to->pos()).translated(dx, dy).center();
        relationTarget = d->to->parentItem()->mapToScene(relationTarget);
        relationTarget = relation->parentItem()->mapFromScene(relationTarget);

        // TODO: Maybe we should set the lines always in scene coordinates?
        // This is in parent coordinates, *or* scene when parent is null.
        relation->setLine(QLineF(relationSource, relationTarget));
        return;
    }

    // No special case, we need to figure out if we can get the information
    // from the node or the parent, and we use scenePos instead of localPos
    QGraphicsItem *sourceEntity = nullptr;
    if (d->pointFrom == PointFrom::PARENT && d->from->parentItem()) {
        sourceEntity = d->from->parentItem();
    } else {
        sourceEntity = d->from;
    }

    QGraphicsItem *targetEntity = nullptr;
    if (d->pointTo == PointTo::PARENT && d->to->parentItem()) {
        targetEntity = d->to->parentItem();
    } else {
        targetEntity = d->to;
    }

    const QPointF relationSource = sourceEntity->mapToScene(sourceEntity->boundingRect().center());
    const QPointF relationTarget = targetEntity->mapToScene(targetEntity->boundingRect().center());

    relation->setLine(QLineF(relationSource, relationTarget));
}

void EdgeCollection::layoutRelations()
{
    if (d->relations.size() == 1) {
        layoutSingleEdge(d->relations[0], 0, 0);
        return;
    }

    QPointF sourcePoint = d->from->scenePos();
    QPointF targetPoint = d->to->scenePos();

    double x = sourcePoint.x() - targetPoint.x();
    double y = sourcePoint.y() - targetPoint.y();
    double length = std::sqrt((x * x) + (y * y));

    // Space out relations in the edge.
    // The spacings are at 90 degrees to the arrow,
    // and so the x and y coordinates are interchanged
    double ySpacing = (x / length) * INTER_RELATION_SPACING;
    double xSpacing = (y / length) * INTER_RELATION_SPACING;

    // assume that d->relations.size() is a smallish integer. Theoretically it
    // could be ULLONG_MAX, but in practice we shouldn't see that many relations
    // If for some reason it was a very large unsigned integer, then the double
    // approximation will only be approximate
    const auto num_relations = static_cast<double>(d->relations.size());

    double dy = -(((num_relations - 1) * ySpacing) / 2.0);
    double dx = -(((num_relations - 1) * xSpacing) / 2.0);

    for (auto *relation : d->relations) {
        layoutSingleEdge(relation, dx, dy);
        dy += ySpacing;
        dx += xSpacing;
    }
}

void EdgeCollection::setHighlighted(bool highlighted)
{
    toggleRelationFlags(RelationFlags::RelationIsParentHovered, highlighted);
}

void EdgeCollection::setVisible(bool v)
{
    for (auto *relation : d->relations) {
        relation->setVisible(v);
    }
}

void EdgeCollection::setPointFrom(PointFrom entity)
{
    d->pointFrom = entity;
    layoutRelations();
}

void EdgeCollection::setPointTo(PointTo entity)
{
    d->pointTo = entity;
    layoutRelations();
}

void EdgeCollection::toggleRelationFlags(RelationFlags flags, bool toggle)
{
    for (LakosRelation *relation : d->relations) {
        relation->toggleRelationFlags(flags, toggle);
    }
}

void EdgeCollection::removeEdge(LakosRelation *edge)
{
    auto it = std::find(std::begin(d->relations), std::end(d->relations), edge);
    if (it != std::end(d->relations)) {
        d->relations.erase(it);
    }
}

void EdgeCollection::setRedundant(bool redundant)
{
    d->isRedundant = redundant;

    // TODO: Change the visibility status of the redundant edges?
    // maybe a semi-translucent color so they are not as agressive as
    // the normal edges?
    for (auto *edge : d->relations) {
        edge->update();
    }
}

bool EdgeCollection::isRedundant() const
{
    return d->isRedundant;
}

} // end namespace Codethink::lvtqtc
