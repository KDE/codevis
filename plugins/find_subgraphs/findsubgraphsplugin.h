#pragma once

#include <ICodevisPlugin.h>
#include <IGraphicsSceneManagementPlugin.h>
#include <IGraphicsSceneMenuPlugin.h>
#include <IMainWindowPlugin.h>
#include <subgraphtreeview.h>

#include <ct_lvtqtc_graphicsscene.h>
#include <ct_lvtqtc_lakosentity.h>
#include <findsubgraphs_export.h>

#include <QDockWidget>

using namespace Codevis::PluginSystem;

// End

class FINDSUBGRAPHS_EXPORT FindSubgraphsPlugin : public ICodevisPlugin,
                                                 public IGraphicsSceneMenuPlugin,
                                                 public IGraphicsSceneManagementPlugin,
                                                 public IMainWindowPlugin {
    Q_OBJECT
  public:
    FindSubgraphsPlugin(QObject *parent);

    // From IGraphicsSceneMenuPlugin
    QList<QMenu *> menuActions(const std::vector<Codethink::lvtqtc::LakosEntity *>& selectionForActions) override;
    QList<QMenu *> menuActions(Codethink::lvtqtc::LakosEntity *entityForActions) override;
    QList<QMenu *> menuActions(Codethink::lvtqtc::GraphicsScene *sceneForActions) override;

    // from IGraphicsSceneManagementPlugin
    void graphicsSceneChanged(Codethink::lvtqtc::GraphicsScene *currentScene) override;
    void graphicsSceneDestroyed(Codethink::lvtqtc::GraphicsScene *destroyedScene) override;

    // from IMainWindowPlugin
    void mainWindowReady(QMainWindow *mainWindow) override;

    void lookForSubgraphs(const std::vector<Codethink::lvtqtc::LakosEntity *>& entities);

  private:
    SubgraphListView *d_listView = nullptr;
    QDockWidget *d_dockWidget = nullptr;
    std::map<std::string, std::vector<Graph>> d_ourGraphs;
    Graph d_prevSelected;
    std::string d_currentGraphScene;
};
