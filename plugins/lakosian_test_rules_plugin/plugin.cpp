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

#include <ct_lvtplg_basicpluginhooks.h>
#include <ct_lvtplg_handlercodeanalysis.h>
#include <ct_lvtplg_handlercontextmenuaction.h>
#include <ct_lvtplg_handlersetup.h>

#include <utils.h>

enum class ToggleTestEntitiesState { VISIBLE, HIDDEN };

struct PluginData {
    static auto constexpr ID = "LKS_TEST_RULES_PLG";

    ToggleTestEntitiesState toggleState = ToggleTestEntitiesState::VISIBLE;
    std::vector<std::string> toggledTestEntities;
};

template<typename Handler_t>
PluginData *getPluginData(Handler_t *handler)
{
    return static_cast<PluginData *>(handler->getPluginData(PluginData::ID));
}

void hookSetupPlugin(PluginSetupHandler *handler)
{
    handler->registerPluginData(PluginData::ID, new PluginData{});
}

void hookTeardownPlugin(PluginSetupHandler *handler)
{
    auto *data = getPluginData(handler);
    handler->unregisterPluginData(PluginData::ID);
    delete data;
}

void toggleTestEntities(PluginContextMenuActionHandler *handler)
{
    auto *pluginData = getPluginData(handler);
    if (pluginData->toggleState == ToggleTestEntitiesState::VISIBLE) {
        pluginData->toggledTestEntities.clear();
        for (auto&& e : handler->getAllEntitiesInCurrentView()) {
            if (utils::string{e.getQualifiedName()}.endswith(".t")) {
                pluginData->toggledTestEntities.push_back(e.getQualifiedName());
                e.unloadFromScene();
            }
        }
        pluginData->toggleState = ToggleTestEntitiesState::HIDDEN;
    } else if (pluginData->toggleState == ToggleTestEntitiesState::HIDDEN) {
        for (auto&& qName : pluginData->toggledTestEntities) {
            handler->loadEntityByQualifiedName(qName);
        }
        pluginData->toggledTestEntities.clear();
        pluginData->toggleState = ToggleTestEntitiesState::VISIBLE;
    }
}

void hookGraphicsViewContextMenu(PluginContextMenuHandler *handler)
{
    handler->registerContextMenu("Toggle test entities", &toggleTestEntities);
}
