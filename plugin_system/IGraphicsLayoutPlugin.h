#pragma once

#include <QRectF>
#include <QtPlugin>

#include <pluginsystemheaders_export.h>

// From Qt's QGraphicsView.
class QGraphicsScene;

// From Codevis - lvtqtc
namespace Codethink::lvtqtc {
class LakosEntity;
class GraphicsScene;
} // namespace Codethink::lvtqtc

namespace Codevis::PluginSystem {

/* Use this interface to implement a number of layout algorithms. */
class PLUGINSYSTEMHEADERS_EXPORT IGraphicsLayoutPlugin {
  public:
    // The names of the algorithms that this plugin implements
    virtual QList<QString> layoutAlgorithms() = 0;

    // Receives a Graph and modifies it's bits to the new layout.
    // Note that the only thing it can modify is the ->pos of the nodes.
    // This executes on a secondary thread.
    using ExecuteLayoutParams = std::variant<Codethink::lvtqtc::GraphicsScene *, Codethink::lvtqtc::LakosEntity *>;
    virtual void executeLayout(const QString& algorithmName, ExecuteLayoutParams params) = 0;

    virtual QWidget *configureWidget() = 0;
};
} // namespace Codevis::PluginSystem

Q_DECLARE_INTERFACE(Codevis::PluginSystem::IGraphicsLayoutPlugin, "org.kde.codevis.IGraphicsLayoutPlugin");
