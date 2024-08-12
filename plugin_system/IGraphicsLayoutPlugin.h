#pragma once

#include "PluginManagerV2.h"
#include <QRectF>
#include <QtPlugin>

#include <optional>
#include <pluginsystemheaders_export.h>

namespace Codevis::PluginSystem {

/* Use this interface to implement a number of layout algorithms. */
class PLUGINSYSTEMHEADERS_EXPORT IGraphicsLayoutPlugin {
  public:
    // The graph representation used by this plugin.
    struct Node {
        intptr_t id; // Maps to the pointer of the actual object
        intptr_t parentId; // Maps to the pointer of the Parent
        int lakosianLevel;
        std::vector<Node> children;
        std::string qualifiedName;
        QRectF rect;

        // this point represents the center of the rect on the canvas.
        QPointF pos;
    };

    struct Graph {
        QRectF rect;
        std::vector<Node> topLevelNodes;
        QMultiHash<intptr_t, intptr_t> connections; // Table of connections between each node identified by id.
        std::optional<QString> topLevelQualifiedName;
    };

    // The names of the algorithms that this plugin implements
    virtual QList<QString> layoutAlgorithms() = 0;

    // Receives a Graph and modifies it's bits to the new layout.
    // Note that the only thing it can modify is the ->pos of the nodes.
    // This executes on a secondary thread.
    virtual void executeLayout(const QString& algorithmName, Graph& g) = 0;

    virtual QWidget *configureWidget() = 0;
};
} // namespace Codevis::PluginSystem

Q_DECLARE_INTERFACE(Codevis::PluginSystem::IGraphicsLayoutPlugin, "org.kde.codevis.IGraphicsLayoutPlugin");
