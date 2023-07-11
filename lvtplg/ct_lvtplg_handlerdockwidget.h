// ct_lvtplg_handlerdockwidget.h                                     -*-C++-*-

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

#ifndef DIAGRAM_SERVER_CT_LVTPLG_PLUGINDOCKWIDGETHANDLER_H
#define DIAGRAM_SERVER_CT_LVTPLG_PLUGINDOCKWIDGETHANDLER_H

#include <ct_lvtplg_handlertreewidget.h>

#include <functional>
#include <string>

struct PluginDockWidgetHandler {
    /**
     * Returns the plugin data previously registered with `PluginSetupHandler::registerPluginData`.
     */
    std::function<void *(std::string const& id)> const getPluginData;

    /**
     * Creates a new dock in the GUI.
     */
    std::function<void(std::string const& dockId, std::string const& title)> const createNewDock;

    /**
     * Adds a text field in the dock widget. When the field is changed, the dataModel will be automatically updated.
     * Make sure to manage the lifetime of the dataModel to ensure it's available outside the hook function's scope.
     */
    std::function<void(std::string const& dockId, std::string const& title, std::string& dataModel)> const
        addDockWdgTextField;

    /**
     * Create a new tree in the dock widget.
     */
    std::function<void(std::string const& dockId, std::string const& treeId)> const addTree;
};

#endif
