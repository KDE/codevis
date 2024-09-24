/*  This file was part of the KDE libraries
 *
 *    SPDX-FileCopyrightText: 2021 Tomaz Canabrava <tcanabrava@kde.org>
 *
 *    SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include "IGraphicsSceneManagementPlugin.h"
#include <QObject>

#include <pluginsystem_export.h>

namespace Codevis::PluginSystem {
// Plugin Interfaces.
class ICodevisPlugin;
class IGraphicsLayoutPlugin;
class IGraphicsSceneMenuPlugin;
class IGraphicsSceneManagementPlugin;
class IMainWindowPlugin;

struct PluginManagerPrivate;

class PLUGINSYSTEM_EXPORT PluginManagerV2 : public QObject {
    Q_OBJECT
  public:
    static PluginManagerV2& self();
    ~PluginManagerV2() override;
    void loadAllPlugins();

    const std::vector<ICodevisPlugin *>& plugins() const;
    const std::vector<IGraphicsLayoutPlugin *>& graphicsLayoutPlugins() const;
    const std::vector<IGraphicsSceneMenuPlugin *>& graphicsSceneMenuPlugins() const;
    const std::vector<IGraphicsSceneManagementPlugin *>& graphicsSceneManagementPlugins() const;
    const std::vector<IMainWindowPlugin *>& mainWindowPlugins() const;

  private:
    PluginManagerV2();

    std::vector<ICodevisPlugin *> d_plugins;
    std::vector<IGraphicsLayoutPlugin *> d_graphicsLayoutPlugins;
    std::vector<IGraphicsSceneMenuPlugin *> d_graphicsSceneMenuPlugins;
    std::vector<IGraphicsSceneManagementPlugin *> d_graphicsSceneManagementPlugins;
    std::vector<IMainWindowPlugin *> d_mainWindowPlugins;
};

} // namespace Codevis::PluginSystem
#endif
