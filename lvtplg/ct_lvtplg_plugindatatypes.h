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

#include <functional>
#include <string>

struct Color {
    int r;
    int g;
    int b;
    int a = 255;
};

enum class EntityType { Unknown, PackageGroup, Package, Component };

struct Entity {
    std::function<std::string()> const getName;
    std::function<std::string()> const getQualifiedName;
    std::function<EntityType()> const getType;
    std::function<void(Color rgbColor)> const setColor;
    std::function<void(std::string info)> const addHoverInfo;
    std::function<std::vector<Entity>()> const getDependencies;
};

#endif
