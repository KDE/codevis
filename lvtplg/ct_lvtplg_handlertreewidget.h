// ct_lvtplg_handlertreewidget.h                                     -*-C++-*-

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

#ifndef DIAGRAM_SERVER_CT_LVTPLG_PLUGINTREEWIDGETHANDLER_H
#define DIAGRAM_SERVER_CT_LVTPLG_PLUGINTREEWIDGETHANDLER_H

#include <ct_lvtplg_handlergraphicsview.h>

#include <functional>
#include <string>

struct PluginTreeItemClickedActionHandler;
struct PluginTreeItemHandler {
    std::function<PluginTreeItemHandler(std::string const& label)> const addChild;
    std::function<void(std::string const& dataId, void *userData)> const addUserData;
    std::function<void *(std::string const& dataId)> const getUserData;

    using onClickItemAction_f = std::function<void(PluginTreeItemClickedActionHandler *selectedItem)>;
    std::function<void(onClickItemAction_f const& action)> const addOnClickAction;
};

struct PluginTreeItemClickedActionHandler {
    std::function<PluginTreeItemHandler()> getItem;
    std::function<PluginGraphicsViewHandler()> getGraphicsView;
};

struct PluginTreeWidgetHandler {
    std::function<PluginTreeItemHandler(std::string const& label)> const addRootItem;
    std::function<void()> const clear;
};

#endif
