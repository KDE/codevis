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

#ifndef DIAGRAM_SERVER_CT_LVTPLG_PLUGINCONTEXTMENUACTIONHANDLER_H
#define DIAGRAM_SERVER_CT_LVTPLG_PLUGINCONTEXTMENUACTIONHANDLER_H

#include <ct_lvtplg_handlertreewidget.h>
#include <ct_lvtplg_plugindatatypes.h>

#include <functional>
#include <optional>
#include <string>
#include <vector>

struct PluginContextMenuActionHandler;

struct PluginContextMenuHandler {
    /**
     * Returns the plugin data previously registered with `PluginSetupHandler::registerPluginData`.
     */
    std::function<void *(std::string const& id)> const getPluginData;

    /**
     * Returns a vector of a wrapper for the entities in the current view. It is not guarantee that such entities will
     * be available outside the caller hook scope (So it is adviseable that you do not keep references or copies of
     * such entities).
     */
    std::function<std::vector<Entity>()> const getAllEntitiesInCurrentView;

    /**
     * Returns one instance of a wrapper for one entity in the current view. It is not guarantee that such entities will
     * be available outside the caller hook scope (So it is adviseable that you do not keep references or copies of
     * such entities).
     */
    std::function<std::optional<Entity>(std::string const& qualifiedName)> const getEntityByQualifiedName;

    /**
     * Call this function to register a new menu action on the current view.
     */
    using ctxMenuAction_f = std::function<void(PluginContextMenuActionHandler *)>;
    std::function<void(std::string const& title, ctxMenuAction_f const& action)> const registerContextMenu;

    std::function<std::optional<Edge>(std::string const& fromQualifiedName, std::string const& toQualifiedName)> const
        getEdgeByQualifiedName;
};

struct PluginContextMenuActionHandler {
    /**
     * Returns the plugin data previously registered with `PluginSetupHandler::registerPluginData`.
     */
    std::function<void *(std::string const& id)> const getPluginData;

    /**
     * Returns a vector of a wrapper for the entities in the current view. It is not guarantee that such entities will
     * be available outside the caller hook scope (So it is adviseable that you do not keep references or copies of
     * such entities).
     */
    std::function<std::vector<Entity>()> const getAllEntitiesInCurrentView;

    /**
     * Returns one instance of a wrapper for one entity in the current view. It is not guarantee that such entities will
     * be available outside the caller hook scope (So it is adviseable that you do not keep references or copies of
     * such entities).
     */
    std::function<std::optional<Entity>(std::string const& qualifiedName)> const getEntityByQualifiedName;

    std::function<PluginTreeWidgetHandler(std::string const& id)> const getTree;

    std::function<std::optional<Edge>(std::string const& fromQualifiedName, std::string const& toQualifiedName)> const
        getEdgeByQualifiedName;

    /**
     * Loads an entity into the current graphics scene by qualified name. If the entity doesn't exists, nothing is
     * loaded.
     */
    std::function<void(std::string const& qualifiedName)> const loadEntityByQualifiedName;

    /**
     * Creates a new edge connecting the entities with the respective qualified names. If any of those entities is not
     * found, will not create the edge. The edge won't be persisted.
     */
    std::function<std::optional<Edge>(std::string const& fromQualifiedName, std::string const& toQualifiedName)>
        addEdgeByQualifiedName;

    /**
     * Removes an edge connecting the entities with the respective qualified names. If connection or entities are not
     * found, nothing is done. This action is not persisted.
     */
    std::function<void(std::string const& fromQualifiedName, std::string const& toQualifiedName)>
        removeEdgeByQualifiedName;

    /**
     * Check if there's an edge between the entities with the respective qualified name currently on the scene.
     * Note that this doesn't necessarily mean that they have or they have not an actual dependency, since it could be
     * a plugin dependency.
     */
    std::function<bool(std::string const& fromQualifiedName, std::string const& toQualifiedName)>
        hasEdgeByQualifiedName;
};

#endif // DIAGRAM_SERVER_CT_LVTPLG_PLUGINCONTEXTMENUACTIONHANDLER_H
