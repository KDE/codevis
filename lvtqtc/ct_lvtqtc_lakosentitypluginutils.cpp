// ct_lvtqtc_lakosentitypluginutils.cpp                              -*-C++-*-

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

#include <ct_lvtldr_lakosiannode.h>
#include <ct_lvtqtc_lakosentitypluginutils.h>
#include <ct_lvtqtc_lakosrelation.h>

namespace Codethink::lvtqtc {

Entity createWrappedEntityFromLakosEntity(LakosEntity *e)
{
    auto getName = [e]() {
        return e->name();
    };
    auto getQualifiedName = [e]() {
        return e->qualifiedName();
    };
    auto getType = [e]() {
        switch (e->instanceType()) {
        case lvtshr::DiagramType::ClassType:
            return EntityType::Unknown;
        case lvtshr::DiagramType::ComponentType:
            return EntityType::Component;
        case lvtshr::DiagramType::PackageType:
            if (e->internalNode()->isPackageGroup()) {
                return EntityType::PackageGroup;
            }
            return EntityType::Package;
        case lvtshr::DiagramType::RepositoryType:
            return EntityType::Unknown;
        case lvtshr::DiagramType::FreeFunctionType:
            return EntityType::Unknown;
        case lvtshr::DiagramType::NoneType:
            return EntityType::Unknown;
        }
        return EntityType::Unknown;
    };
    auto setColor = [e](Color c) {
        e->setColor(QColor{c.r, c.g, c.b, c.a});
    };
    auto addHoverInfo = [e](std::string const& info) {
        e->setTooltipString(e->tooltipString() + "\n" + info);
    };
    auto getDependencies = [e]() {
        auto dependencies = std::vector<Entity>{};
        for (auto const& c : e->edgesCollection()) {
            for (auto const& r : c->relations()) {
                dependencies.push_back(createWrappedEntityFromLakosEntity(r->to()));
            }
        }
        return dependencies;
    };
    auto unloadFromScene = [e]() {
        Q_EMIT e->unloadThis();
    };
    auto getDbChildrenQualifiedNames = [e]() {
        auto childrenQNames = std::vector<std::string>{};
        for (auto const& c : e->internalNode()->children()) {
            childrenQNames.emplace_back(c->qualifiedName());
        }
        return childrenQNames;
    };
    auto getParent = [e]() -> std::optional<Entity> {
        auto parent = dynamic_cast<LakosEntity *>(e->parentItem());
        if (!parent) {
            return std::nullopt;
        }
        return createWrappedEntityFromLakosEntity(parent);
    };
    auto setSelected = [e](bool v) {
        e->setSelected(v);
    };
    auto isSelected = [e]() -> bool {
        return e->isSelected();
    };
    return Entity{getName,
                  getQualifiedName,
                  getType,
                  setColor,
                  addHoverInfo,
                  getDependencies,
                  unloadFromScene,
                  getDbChildrenQualifiedNames,
                  getParent,
                  setSelected,
                  isSelected};
}

Edge createWrappedEdgeFromLakosEntity(LakosEntity *from, LakosEntity *to)
{
    auto getRelation = [from, to]() -> LakosRelation * {
        if (!from || !to) {
            return nullptr;
        }
        auto& ecs = from->edgesCollection();
        auto foundToNode = std::find_if(ecs.begin(), ecs.end(), [to](auto const& ec) {
            return ec->to() == to;
        });
        if (foundToNode == ecs.end()) {
            return nullptr;
        }
        return (*foundToNode)->relations()[0];
    };
    auto setColor = [getRelation](Color c) {
        auto *relation = getRelation();
        if (!relation) {
            return;
        }
        relation->setColor(QColor{c.r, c.g, c.b, c.a});
    };
    auto setStyle = [getRelation](EdgeStyle s) {
        auto *relation = getRelation();
        if (!relation) {
            return;
        }
        auto qtStyle = [&]() {
            switch (s) {
            case EdgeStyle::SolidLine:
                return Qt::PenStyle::SolidLine;
            case EdgeStyle::DashLine:
                return Qt::PenStyle::DashLine;
            case EdgeStyle::DotLine:
                return Qt::PenStyle::DotLine;
            case EdgeStyle::DashDotLine:
                return Qt::PenStyle::DashDotLine;
            case EdgeStyle::DashDotDotLine:
                return Qt::PenStyle::DashDotDotLine;
            }
            return Qt::PenStyle::NoPen;
        }();
        relation->setStyle(qtStyle);
    };
    return Edge{setColor, setStyle};
}

} // namespace Codethink::lvtqtc
