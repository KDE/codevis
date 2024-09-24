#include "subgraphtreeview.h"
#include <KPluginFactory>
#include <QApplication>
#include <QMainWindow>
#include <ct_lvtqtc_lakosentity.h>
#include <findsubgraphsplugin.h>

#include <graphdefinition.h>

FindSubgraphsPlugin::FindSubgraphsPlugin(QObject *parent): ICodevisPlugin(parent)
{
    d_dockWidget = new QDockWidget(tr("Subgraph List"));
    d_listView = new SubgraphListView(d_dockWidget);
    d_dockWidget->setWidget(d_listView);
    d_dockWidget->setVisible(false);
}

QList<QMenu *>
FindSubgraphsPlugin::menuActions(const std::vector<Codethink::lvtqtc::LakosEntity *>& selectionForActions)
{
    return {};
}

/* Provides a menu that will work on the single entity */
QList<QMenu *> FindSubgraphsPlugin::menuActions(Codethink::lvtqtc::LakosEntity *entityForActions)
{
    const QString entityName = QString::fromStdString(entityForActions->name());
    auto menu = new QMenu(QStringLiteral("Find Subgraphs on %1").arg(entityName));
    menu->setObjectName("find-subgraph-menu-for-entity");
    auto action = menu->addAction("Execute");

    connect(action, &QAction::triggered, this, [this, entityForActions] {
        lookForSubgraphs(entityForActions->lakosEntities());
    });

    return {menu};
}

QList<QMenu *> FindSubgraphsPlugin::menuActions(Codethink::lvtqtc::GraphicsScene *sceneForActions)
{
    auto menu = new QMenu(QStringLiteral("Find Subgraphs on the scene"));
    menu->setObjectName("find-subgraph-menu-for-scene");

    auto action = menu->addAction("Execute");

    connect(action, &QAction::triggered, this, [this, sceneForActions] {
        std::vector<Codethink::lvtqtc::LakosEntity *> entities = {};
        for (auto entity : sceneForActions->allEntities()) {
            if (!entity->parentItem()) {
                entities.push_back(entity);
            }
        }
        lookForSubgraphs(entities);
    });

    return {menu};
}

void FindSubgraphsPlugin::graphicsSceneChanged(Codethink::lvtqtc::GraphicsScene *currentScene)
{
    d_currentGraphScene = currentScene->objectName().toStdString();
    d_listView->setGraphs(d_ourGraphs[d_currentGraphScene]);
}

void FindSubgraphsPlugin::graphicsSceneDestroyed(Codethink::lvtqtc::GraphicsScene *destroyedScene)
{
    d_listView->setGraphs({});
}

void FindSubgraphsPlugin::mainWindowReady(QMainWindow *mainWindow)
{
    mainWindow->addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, d_dockWidget);
}

Graph buildBoostGraph(const std::vector<Codethink::lvtqtc::LakosEntity *>& currentClusterNodes)
{
    Graph g;
    QMap<Codethink::lvtqtc::LakosEntity *, long> vertices;

    for (const auto& vertex : currentClusterNodes) {
        if (!vertices.contains(vertex)) {
            vertices.insert(vertex, add_vertex(VertexProps{vertex}, g));
        }

        auto ee = vertices.value(vertex);
        for (const auto& edge : vertex->edgesCollection()) {
            if (!vertices.contains(edge->to())) {
                vertices.insert(edge->to(), add_vertex(VertexProps{edge->to()}, g));
            }
            auto jj = vertices.value(edge->to());

            add_edge(ee, jj, g);
        }
    }

    return g;
}

std::vector<int> map_components(Graph const& g)
{
    std::vector<int> mappings(num_vertices(g));
    boost::connected_components(g, &mappings[0]);
    return mappings;
}

std::vector<Graph> split(Graph const& g, std::vector<int> const& components)
{
    if (components.empty()) {
        return {};
    }

    std::vector<Graph> results;

    auto highest = *std::max_element(components.begin(), components.end());
    for (int c = 0; c <= highest; ++c) {
        results.emplace_back();
        boost::copy_graph(Filtered(g,
                                   {},
                                   [c, &components](Vertex v) {
                                       return components.at(v) == c;
                                   }),
                          results.back());
    }

    return results;
}

void FindSubgraphsPlugin::lookForSubgraphs(const std::vector<Codethink::lvtqtc::LakosEntity *>& entities)
{
    auto graph = buildBoostGraph(entities);
    auto map = map_components(graph);

    d_ourGraphs[d_currentGraphScene] = split(graph, map);
    d_prevSelected = Graph{};

    d_listView->setGraphs(d_ourGraphs[d_currentGraphScene]);
    d_dockWidget->setVisible(true);
}

K_PLUGIN_CLASS_WITH_JSON(FindSubgraphsPlugin, "metadata.json")

#include "moc_findsubgraphsplugin.cpp"

#include "findsubgraphsplugin.moc"
