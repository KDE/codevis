// ct_lvtplg_handlercontextmenuaction.h                              -*-C++-*-

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

#ifndef DIAGRAM_SERVER_CT_LVTPLG_PLUGINDATATYPES_H
#define DIAGRAM_SERVER_CT_LVTPLG_PLUGINDATATYPES_H

#include <any>
#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace Codethink::lvtplg {
struct Color {
    int r = 0;
    int g = 0;
    int b = 0;
    int a = 255;
};

enum class EdgeStyle { SolidLine, DashLine, DotLine, DashDotLine, DashDotDotLine };

enum class EntityType { Unknown, PackageGroup, Package, Component };

enum class PluginFieldType { TextInput, TextArea };

using RawDBData = std::optional<std::any>;
using RawDBCols = std::vector<RawDBData>;
using RawDBRows = std::vector<RawDBCols>;

struct Entity {
    std::function<std::string()> getName;
    std::function<std::string()> getQualifiedName;
    std::function<EntityType()> getType;
    std::function<void(Color rgbColor)> setColor;
    std::function<void(std::string info)> addHoverInfo;
    std::function<std::vector<std::shared_ptr<Entity>>&()> getDependencies;
    std::function<std::vector<std::shared_ptr<Entity>>&()> getChildren;

    /**
     * Unloads the entity from the current scene.
     * Warning: The Entity instance becomes invalid after method this is called, and must *not* be used.
     */
    std::function<void()> unloadFromScene;

    /**
     * Will return all the qualified names from all children from the database. Not only those loaded in the scene.
     */
    std::function<std::vector<std::string>()> getDbChildrenQualifiedNames;

    std::function<std::shared_ptr<Entity>()> getParent;

    std::function<void(bool v)> setSelected;
    std::function<bool()> isSelected;
};

struct Edge {
    std::function<void(Color rgbColor)> setColor;
    std::function<void(EdgeStyle style)> setStyle;
};

struct ProjectData {
    std::function<std::string()> getSourceCodePath;
};

} // namespace Codethink::lvtplg

#endif
