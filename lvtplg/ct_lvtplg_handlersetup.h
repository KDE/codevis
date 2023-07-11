// ct_lvtplg_handlersetup.h                                          -*-C++-*-

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

#ifndef DIAGRAM_SERVER_CT_LVTPLG_PLUGINSETUPHANDLER_H
#define DIAGRAM_SERVER_CT_LVTPLG_PLUGINSETUPHANDLER_H

#include <functional>
#include <string>

struct PluginSetupHandler {
    /**
     * Register a user-defined plugin data structure, that can be retrieved in other hooks.
     */
    std::function<void(std::string const& id, void *data)> const registerPluginData;

    /**
     * Returns the plugin data previously registered with `registerPluginData`.
     */
    std::function<void *(std::string const& id)> const getPluginData;

    /**
     * Unregister a plugin data. Please make sure you delete the data before calling this, or the resource will leak.
     */
    std::function<void(std::string const& id)> const unregisterPluginData;
};

#endif // DIAGRAM_SERVER_CT_LVTPLG_PLUGINSETUPHANDLER_H
