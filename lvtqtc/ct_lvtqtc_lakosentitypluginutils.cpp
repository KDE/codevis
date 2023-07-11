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
            return EntityType::Package;
        case lvtshr::DiagramType::RepositoryType:
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
        for (auto&& c : e->edgesCollection()) {
            for (auto&& r : c->relations()) {
                dependencies.push_back(createWrappedEntityFromLakosEntity(r->to()));
            }
        }
        return dependencies;
    };
    return Entity{getName, getQualifiedName, getType, setColor, addHoverInfo, getDependencies};
}

} // namespace Codethink::lvtqtc
