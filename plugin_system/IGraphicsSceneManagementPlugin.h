#pragma once

#include <QList>
#include <vector>

#include <pluginsystemheaders_export.h>

class QMenu;

namespace Codethink::lvtqtc {
class LakosEntity;
class GraphicsScene;
}; // namespace Codethink::lvtqtc

namespace Codevis::PluginSystem {
/* This interface should be used when the plugin needs to react to changes
 * on the graphics scene.
 */
class IGraphicsSceneManagementPlugin {
  public:
    virtual void graphicsSceneChanged(Codethink::lvtqtc::GraphicsScene *currentScene) = 0;
    virtual void graphicsSceneDestroyed(Codethink::lvtqtc::GraphicsScene *destroyedScene) = 0;
    // TODO: Maybe add things related about node selection here?
};
} // namespace Codevis::PluginSystem
