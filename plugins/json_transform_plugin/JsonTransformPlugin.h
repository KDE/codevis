#pragma once

#include <ICodevisPlugin.h>
#include <IGraphicsSceneMenuPlugin.h>

#include <ct_lvtqtc_graphicsscene.h>
#include <ct_lvtqtc_lakosentity.h>
#include <jsontransformplugin_export.h>

using namespace Codevis::PluginSystem;
using namespace Codethink::lvtqtc;

class JSONTRANSFORMPLUGIN_EXPORT JsonTransformPlugin : public ICodevisPlugin, public IGraphicsSceneMenuPlugin {
    Q_OBJECT
  public:
    JsonTransformPlugin(QObject *parent);
    /* Provides a menu that have actions that will work work on the list of selected entities */
    QList<QMenu *> menuActions(const std::vector<LakosEntity *>& selectionForActions) override;

    /* Provides a menu that will work on the single entity */
    QList<QMenu *> menuActions(LakosEntity *entityForActions) override;

    /* Provides a menu that will work on the scene as a whole */
    QList<QMenu *> menuActions(GraphicsScene *sceneForActions) override;
};
