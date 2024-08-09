#include "BasicLayoutPlugin.h"
#include "ICodevisPlugin.h"

#include <KPluginFactory>
#include <qnamespace.h>

BasicLayoutPlugin::BasicLayoutPlugin(QObject *parent, const QVariantList& args):
    Codevis::PluginSystem::ICodevisPlugin(parent, args)
{
}

QString BasicLayoutPlugin::pluginName()
{
    return "Basic Layout";
}

QString BasicLayoutPlugin::pluginDescription()
{
    return "Applies a basic layout on a given graph";
}

QList<QString> BasicLayoutPlugin::pluginAuthors()
{
    return {"Tomaz Canabrava - tcanabrava@kde.org"};
}

// IGraphicsLayoutPlugin
QList<QString> BasicLayoutPlugin::layoutAlgorithms()
{
    return {"Basic Vertical", "Basic Horizontal"};
}

void BasicLayoutPlugin::executeLayout(const QString& algorithmName,
                                      Codevis::PluginSystem::IGraphicsLayoutPlugin::Graph& g)
{
    std::ignore = algorithmName;
    for (auto node : g.topLevelNodes) {
        node->pos = QPointF(0, 0);
    }
}

K_PLUGIN_CLASS_WITH_JSON(BasicLayoutPlugin, "codevis_basiclayoutplugin.json")

#include "BasicLayoutPlugin.moc"
