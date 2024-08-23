
#include <KPluginFactory>

#include "JsonEditorDialog.h"
#include "JsonTransformPlugin.h"

#include <QGraphicsView>
#include <QMenu>

JsonTransformPlugin::JsonTransformPlugin(QObject *parent): ICodevisPlugin(parent)
{
    Q_INIT_RESOURCE(json_transform_plugin);
};

/* Provides a menu that have actions that will work work on the list of selected entities */
QList<QMenu *> JsonTransformPlugin::menuActions(const std::vector<LakosEntity *>& selectionForActions)
{
    return {};
}

/* Provides a menu that will work on the single entity */
QList<QMenu *> JsonTransformPlugin::menuActions(LakosEntity *entityForActions)
{
    return {};
}

/* Provides a menu that will work on the scene as a whole */
QList<QMenu *> JsonTransformPlugin::menuActions(GraphicsScene *sceneForActions)
{
    auto menu = new QMenu("JSON Filter Editor");
    auto dialogAction = menu->addAction("Show Dialog");
    connect(dialogAction, &QAction::triggered, this, [sceneForActions] {
        BulkEdit dialog(sceneForActions->views()[0]);
        dialog.setScene(sceneForActions);
        dialog.exec();
    });
    menu->addAction("Reset");
    menu->addAction("Run Last Filter");
    return {menu};
}

K_PLUGIN_CLASS_WITH_JSON(JsonTransformPlugin, "codevis_jsontransformplugin.json")

#include "moc_JsonTransformPlugin.cpp"

#include "JsonTransformPlugin.moc"
