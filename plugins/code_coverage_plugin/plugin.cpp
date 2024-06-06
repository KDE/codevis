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

#include <ct_lvtplg_basicpluginhandlers.h>
#include <ct_lvtplg_basicpluginhooks.h>

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <fstream>
#include <string>
#include <sstream>
#include <iomanip>

static auto const CTX_MENU_TITLE = std::string{"Load code coverage information"};
static auto const PLUGIN_DATA_ID = std::string{"CodeCovPluginData"};

struct CodeCoveragePluginData {
    std::string jsonFilePath;
    std::string htmlFilePath;
};

struct CodeCovMetric {
    int hits = 0;
    int total = 0;

    double percentage() const
    {
        if (total == 0) {
            return 100.;
        }

        return 100. * (static_cast<double>(hits) / total);
    }
};

void hookSetupPlugin(PluginSetupHandler *handler)
{
    handler->registerPluginData(PLUGIN_DATA_ID, new CodeCoveragePluginData{});
}

void hookTeardownPlugin(PluginSetupHandler *handler)
{
    auto *data = static_cast<CodeCoveragePluginData *>(handler->getPluginData(PLUGIN_DATA_ID));
    handler->unregisterPluginData(PLUGIN_DATA_ID);
    delete data;
}

void graphicsViewContextMenuAction(PluginContextMenuActionHandler *handler);
void hookGraphicsViewContextMenu(PluginContextMenuHandler *handler)
{
    handler->registerContextMenu(CTX_MENU_TITLE, &graphicsViewContextMenuAction);
}

void graphicsViewContextMenuAction(PluginContextMenuActionHandler *handler)
{
    auto *data = static_cast<CodeCoveragePluginData *>(handler->getPluginData(PLUGIN_DATA_ID));

    // TODO: Error handling
    auto file = QFile{};
    file.setFileName(QString::fromStdString(data->jsonFilePath));
    if (!file.exists()) {
        return;
    }

    file.open(QIODevice::ReadOnly | QIODevice::Text);
    auto val = QString{file.readAll()};
    file.close();

    auto jsonData = QJsonDocument::fromJson(val.toUtf8()).object();
    auto dataMap = std::map<std::string, CodeCovMetric>{};
    for (auto jsonFileRef : jsonData["files"].toArray()) {
        auto jsonFile = jsonFileRef.toObject();

        auto selectedEntity = [&]() -> std::shared_ptr<Codethink::lvtplg::Entity> {
            for (auto entity : handler->getAllEntitiesInCurrentView()) {
                if (entity->getType() != Codethink::lvtplg::EntityType::Component) {
                    continue;
                }

                auto entityName = QString::fromStdString(entity->getName());
                if (jsonFile["file"].toString().contains(entityName)) {
                    return entity;
                }
            }
            return {};
        }();
        if (!selectedEntity) {
            continue;
        }
        auto& metrics = dataMap[selectedEntity->getQualifiedName()];

        for (auto jsonFileLineRef : jsonFile["lines"].toArray()) {
            auto jsonFileLine = jsonFileLineRef.toObject();
            if (jsonFileLine["gcovr/noncode"].toBool()) {
                /* Line marked as ignored by code coverage tool */
                continue;
            }

            if (jsonFileLine["count"].toInt() > 0) {
                metrics.hits += 1;
            }
            metrics.total += 1;
        }

        // TODO: User definable color mapping
        auto color = [metrics]() -> Codethink::lvtplg::Color {
            auto pct = metrics.percentage();
            if (pct >= 99.99) {
                return {0, 220, 0};
            } else if (pct < 99.99 && pct >= 49.99) {
                return {250, 165, 0};
            } else {
                return {220, 0, 0};
            }
        }();
        selectedEntity->setColor(color);
    }

    for (auto&& [qualifiedName, metrics] : dataMap) {
        auto e = handler->getEntityByQualifiedName(qualifiedName);
        if (!e) {
            continue;
        }

        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << metrics.percentage();
        auto info = "Code coverage: " + ss.str() + "%\n";
        e->addHoverInfo(info);
    }
}

void hookSetupDockWidget(PluginSetupDockWidgetHandler *handler)
{
    auto const DOCK_WIDGET_TITLE = "Code coverage plugin";
    auto const DOCK_WIDGET_ID = "cov_plugin_dock";

    auto *data = static_cast<CodeCoveragePluginData *>(handler->getPluginData(PLUGIN_DATA_ID));

    // TODO: Persist user data on project file
    // TODO: Change text fields with proper field types (Color picker and file picker)
    auto dock = handler->createNewDock(DOCK_WIDGET_ID, DOCK_WIDGET_TITLE);
    dock.addDockWdgTextField("Code coverage json file:", data->jsonFilePath);
    dock.addDockWdgTextField("Code coverage HTML directory:", data->htmlFilePath);
}

void entityReportAction(PluginEntityReportActionHandler *handler);
void hookSetupEntityReport(PluginEntityReportHandler *handler)
{
    auto entityName = handler->getEntity()->getName();
    handler->addReport("Code coverage report", "Code Coverage: " + entityName, &entityReportAction);
}

void entityReportAction(PluginEntityReportActionHandler *handler)
{
    auto *data = static_cast<CodeCoveragePluginData *>(handler->getPluginData(PLUGIN_DATA_ID));
    auto entityName = handler->getEntity()->getName();
    auto covHtmlPath = QString::fromStdString(data->htmlFilePath);

    auto contents = QString{};

    auto cssIfs = std::ifstream(covHtmlPath.toStdString() + "index.css");
    auto cssCcontents = std::string(std::istreambuf_iterator<char>(cssIfs), std::istreambuf_iterator<char>());
    contents += QString::fromStdString("<style>" + cssCcontents + "</style>");

    auto it = QDirIterator(covHtmlPath,
                           QStringList() << QString::fromStdString("*" + entityName + "*.html"),
                           QDir::Files,
                           QDirIterator::Subdirectories);
    while (it.hasNext()) {
        auto filename = it.next();
        auto file = QFile(filename);
        if (!file.open(QIODevice::ReadOnly)) {
            continue;
        }
        auto htmlContents = file.readAll();
        contents += htmlContents;
    }

    handler->setReportContents(contents.toStdString());
}
