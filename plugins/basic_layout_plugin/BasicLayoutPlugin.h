#pragma once

#include <ICodevisPlugin.h>
#include <IGraphicsLayoutPlugin.h>
#include <ct_lvtqtc_graphicsscene.h>
#include <ct_lvtqtc_lakosentity.h>

using namespace Codethink::lvtqtc;
using namespace Codevis::PluginSystem;

struct BasicLayoutPluginConfig {
    enum class LevelizationLayoutType : short { Horizontal, Vertical };
    LevelizationLayoutType type;
    int direction;

    double spaceBetweenLevels = 40.;
    double spaceBetweenSublevels = 10.;
    double spaceBetweenEntities = 10.;
    int maxEntitiesPerLevel = 8;
};

class BasicLayoutPlugin : public ICodevisPlugin, public IGraphicsLayoutPlugin {
    Q_OBJECT
  public:
    BasicLayoutPlugin(QObject *parent);

    // IGraphicsLayoutPlugin
    QList<QString> layoutAlgorithms() override;
    void executeLayout(const QString& algorithmName, IGraphicsLayoutPlugin::ExecuteLayoutParams params) override;
    QWidget *configureWidget() override;
};
