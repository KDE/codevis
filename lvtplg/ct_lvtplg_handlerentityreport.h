// ct_lvtplg_handlerentityreport.h                                   -*-C++-*-

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

#ifndef DIAGRAM_SERVER_CT_LVTPLG_PLUGINENTITYREPORTHANDLER_H
#define DIAGRAM_SERVER_CT_LVTPLG_PLUGINENTITYREPORTHANDLER_H

#include <ct_lvtplg_plugindatatypes.h>
#include <functional>
#include <optional>
#include <string>
#include <vector>

struct PluginEntityReportActionHandler;

struct PluginEntityReportHandler {
    /**
     * Returns the plugin data previously registered with `PluginSetupHandler::registerPluginData`.
     */
    std::function<void *(std::string const& id)> const getPluginData;

    /**
     * Returns the active entity.
     */
    std::function<Entity()> const getEntity;

    /**
     * Setup and add a new report action in the entity context menu.
     */
    using ctxMenuAction_f = std::function<void(PluginEntityReportActionHandler *)>;
    std::function<void(
        std::string const& contextMenuTitle, std::string const& reportTitle, ctxMenuAction_f const& action)> const
        addReport;
};

struct PluginEntityReportActionHandler {
    /**
     * Returns the plugin data previously registered with `PluginSetupHandler::registerPluginData`.
     */
    std::function<void *(std::string const& id)> const getPluginData;

    /**
     * Returns the active entity.
     */
    std::function<Entity()> const getEntity;

    /**
     * Set the contents of the generated report after the user clicks in the context menu action.
     */
    std::function<void(std::string const& contentsHTML)> const setReportContents;
};

#endif // DIAGRAM_SERVER_CT_LVTPLG_PLUGINCONTEXTMENUACTIONHANDLER_H
