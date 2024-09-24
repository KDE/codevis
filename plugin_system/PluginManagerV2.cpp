/*  This file was part of the KDE libraries
 *
 *    SPDX-FileCopyrightText: 2021 Tomaz Canabrava <tcanabrava@kde.org>
 *
 *    SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "IGraphicsSceneManagementPlugin.h"
#include "IMainWindowPlugin.h"
#include <PluginManagerV2.h>

#include <KPluginFactory>
#include <KPluginMetaData>

#include <ICodevisPlugin.h>
#include <IGraphicsLayoutPlugin.h>
#include <IGraphicsSceneManagementPlugin.h>
#include <IGraphicsSceneMenuPlugin.h>

#include <QDebug>
#include <QLoggingCategory>

#include <iostream>

namespace Codevis::PluginSystem {
PluginManagerV2& PluginManagerV2::self()
{
    static PluginManagerV2 us;
    return us;
}

PluginManagerV2::PluginManagerV2()
{
}

PluginManagerV2::~PluginManagerV2()
{
    qDeleteAll(d_plugins);
}

void PluginManagerV2::loadAllPlugins()
{
    QVector<KPluginMetaData> pluginMetaData =
        KPluginMetaData::findPlugins(QStringLiteral("codevis_plugins_v2"), [](const KPluginMetaData& data) {
#if 0
            // Compare RELEASE_SERVICE_VERSION MAJOR and MINOR only: XX.YY
            auto plugin_version = QString(data.version()).left(5);
            auto release_version = QLatin1String(RELEASE_SERVICE_VERSION).left(5);
            if (plugin_version == release_version) {
                return true;
            } else {
                qWarning() << "Ignoring" << data.name() << "plugin version (" << plugin_version << ") doesn't match release version ("
                << release_version << ")";
                return false;
            }
#endif
            std::cout << "Found Plugin " << data.name().toStdString() << std::endl;
            return true;
        });

    for (const auto& metaData : pluginMetaData) {
        const KPluginFactory::Result result = KPluginFactory::instantiatePlugin<ICodevisPlugin>(metaData);
        if (!result) {
            continue;
        }

        if (auto *layoutPlugin = dynamic_cast<IGraphicsLayoutPlugin *>(result.plugin)) {
            d_graphicsLayoutPlugins.push_back(layoutPlugin);
        }

        if (auto *menuPlugin = dynamic_cast<IGraphicsSceneMenuPlugin *>(result.plugin)) {
            d_graphicsSceneMenuPlugins.push_back(menuPlugin);
        }

        if (auto *plugin = dynamic_cast<IGraphicsSceneManagementPlugin *>(result.plugin)) {
            d_graphicsSceneManagementPlugins.push_back(plugin);
        }

        if (auto *plugin = dynamic_cast<IMainWindowPlugin *>(result.plugin)) {
            d_mainWindowPlugins.push_back(plugin);
        }

        d_plugins.push_back(result.plugin);
    }
}

const std::vector<ICodevisPlugin *>& PluginManagerV2::plugins() const
{
    return d_plugins;
}

const std::vector<IGraphicsLayoutPlugin *>& PluginManagerV2::graphicsLayoutPlugins() const
{
    return d_graphicsLayoutPlugins;
}

const std::vector<IGraphicsSceneMenuPlugin *>& PluginManagerV2::graphicsSceneMenuPlugins() const
{
    return d_graphicsSceneMenuPlugins;
}

const std::vector<IGraphicsSceneManagementPlugin *>& PluginManagerV2::graphicsSceneManagementPlugins() const
{
    return d_graphicsSceneManagementPlugins;
}

const std::vector<IMainWindowPlugin *>& PluginManagerV2::mainWindowPlugins() const
{
    return d_mainWindowPlugins;
}

} // namespace Codevis::PluginSystem

#include "moc_PluginManagerV2.cpp"
