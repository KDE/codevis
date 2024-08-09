#pragma once

#include "PluginManagerV2.h"
#include <QRectF>
#include <QtPlugin>

#include <pluginsystemheaders_export.h>

namespace Codevis::PluginSystem {

/* Use this interface to implement a number of layout algorithms. */
class PLUGINSYSTEMHEADERS_EXPORT IGraphicsLayoutPlugin {
  public:
    // The graph represetiation used by this plugin.
    struct Node {
        const Node *parent;
        const QList<Node *> children;
        const std::string qualifiedName;
        const QRect rect;

        // this point represents the center of the rect on the canvas.
        QPointF pos;
    };

    struct Graph {
        QRectF rect;
        const QList<Node *> topLevelNodes;
        const QHash<QString, QString> connection;
    };

    // The names of the algorithms that this plugin implements
    virtual QList<QString> layoutAlgorithms() = 0;

    // Receives a Graph and modifies it's bits to the new layout.
    // Note that the only thing it can modify is the ->pos of the nodes.
    // All pointers are guaranteed to exist, this function should not do any memory management.
    // This executes on a secondary thread.
    virtual void executeLayout(const QString& algorithmName, Graph& g) = 0;
};
} // namespace Codevis::PluginSystem

Q_DECLARE_INTERFACE(Codevis::PluginSystem::IGraphicsLayoutPlugin, "org.kde.codevis.IGraphicsLayoutPlugin");
