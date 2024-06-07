/*
 / /* Copyright 2024 Codethink Ltd <codethink@codethink.co.uk>
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

#include <ct_lvtplg_plugindatatypes.h>

namespace Codethink::lvtplg {

Entity::Entity(std::function<std::string()> const getName,
               std::function<std::string()> const getQualifiedName,
               std::function<EntityType()> const getType,
               std::function<void(Color rgbColor)> const setColor,
               std::function<void(std::string info)> const addHoverInfo,
               std::function<std::vector<std::shared_ptr<Entity>>()> const getDependencies,
               std::function<void()> const unloadFromScene,
               std::function<std::vector<std::string>()> const getDbChildrenQualifiedNames,
               std::function<std::shared_ptr<Entity>()> const getParent,
               std::function<void(bool v)> const setSelected,
               std::function<bool()> const isSelected):
    getName(getName),
    getQualifiedName(getQualifiedName),
    getType(getType),
    setColor(setColor),
    addHoverInfo(addHoverInfo),
    getDependencies(getDependencies),
    unloadFromScene(unloadFromScene),
    getDbChildrenQualifiedNames(getDbChildrenQualifiedNames),
    getParent(getParent),
    setSelected(setSelected),
    isSelected(isSelected)
{
}

Edge::Edge(std::function<void(Color rgbColor)> const setColor, std::function<void(EdgeStyle style)> const setStyle):
    setColor(setColor), setStyle(setStyle)
{
}

ProjectData::ProjectData(std::function<std::string()> const getSourceCodePath): getSourceCodePath(getSourceCodePath)
{
}

} // namespace Codethink::lvtplg
