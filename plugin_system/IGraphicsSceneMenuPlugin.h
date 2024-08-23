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
class IGraphicsSceneMenuPlugin {
  public:
    /* Provides a menu that have actions that will work work on the list of selected entities */
    virtual QList<QMenu *> menuActions(const std::vector<Codethink::lvtqtc::LakosEntity *>& selectionForActions) = 0;

    /* Provides a menu that will work on the single entity */
    virtual QList<QMenu *> menuActions(Codethink::lvtqtc::LakosEntity *entityForActions) = 0;

    /* Provides a menu that will work on the scene as a whole */
    virtual QList<QMenu *> menuActions(Codethink::lvtqtc::GraphicsScene *sceneForActions) = 0;
};
} // namespace Codevis::PluginSystem
