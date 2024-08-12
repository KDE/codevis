#pragma once

#include <ICodevisPlugin.h>
#include <IGraphicsLayoutPlugin.h>

struct BasicLayoutPluginConfig {
    enum class LevelizationLayoutType : short { Horizontal, Vertical };
    LevelizationLayoutType type;
    int direction;

    double spaceBetweenLevels = 40.;
    double spaceBetweenSublevels = 10.;
    double spaceBetweenEntities = 10.;
    int maxEntitiesPerLevel = 8;
};

class BasicLayoutPlugin : public Codevis::PluginSystem::ICodevisPlugin,
                          public Codevis::PluginSystem::IGraphicsLayoutPlugin {
  public:
    BasicLayoutPlugin(QObject *parent, const QVariantList& args);

    // ICodevisPlugin
    QString pluginName() override;
    QString pluginDescription() override;
    QList<QString> pluginAuthors() override;

    // IGraphicsLayoutPlugin
    QList<QString> layoutAlgorithms() override;
    void executeLayout(const QString& algorithmName, Codevis::PluginSystem::IGraphicsLayoutPlugin::Graph& g) override;
    QWidget *configureWidget() override;
};
