from collections import namedtuple

Param = namedtuple('param', ['type', 'name'])
Function = namedtuple('Function', ['return_type', 'name', 'params', 'docs', 'bind_f'])
HandlerInfo = namedtuple('HandlerInfo', ['name', 'functions'])

AS_LAMBDA = 'as_lambda'
NO_BINDINGS = 'no_bindings'

HANDLERS = [
    HandlerInfo("PluginSetupHandler", [
        Function('void', 'registerPluginData', [Param('std::string const&', 'id'), Param('void*', 'data')],
                 'Register a user-defined plugin data structure, that can be retrieved in other hooks.',
                 'pyRegisterPluginData<T>'),
        Function('void*', 'getPluginData', [Param('std::string const&', 'id')],
                 'Returns the plugin data previously registered with `registerPluginData`.',
                 'pyGetPluginData<T>'),
        Function('void', 'unregisterPluginData', [Param('std::string const&', 'id')],
                 'Unregister a plugin data. Please make sure you delete the data before calling this, or the resource will leak.',
                 'pyUnregisterPluginData<T>'),
        Function('PluginPythonInterpHandler', 'getPyInterpHandler', [],
                 'Returns a PluginPythonInterpHandler instance so user can execute Python code within C++ plugins.',
                 NO_BINDINGS),
    ]),

    HandlerInfo("PluginMainWindowReadyHandler", [
        Function('void*', 'getPluginData', [Param('std::string const&', 'id')],
                 'Returns the plugin data previously registered with `registerPluginData`.',
                 'pyGetPluginData<T>'),
        Function('void', 'addMenu', [
                Param('std::string const&', 'title'),
                Param('std::vector<std::tuple<std::string, std::function<void(PluginMenuBarActionHandler)>>> const&', 'menuDescription')
            ],
            'Add a new menu in the interface.',
            AS_LAMBDA
        ),
    ]),

    HandlerInfo("PluginMenuBarActionHandler", [
        Function('void*', 'getPluginData', [Param('std::string const&', 'id')],
                 'Returns the plugin data previously registered with `registerPluginData`.',
                 'pyGetPluginData<T>'),
        Function('void', 'createWizard', [
                Param('std::string const&', 'title'),
                Param('std::vector<std::tuple<std::string, std::string, Codethink::lvtplg::PluginFieldType>> const&', 'fields'),
                Param('std::function<void(PluginWizardActionHandler)> const&', 'action'),
            ],
            'Helper function to create a simple wizard-like window to run something.',
            AS_LAMBDA),
        ],
    ),

    HandlerInfo("PluginWizardActionHandler", [
        Function('void*', 'getPluginData', [Param('std::string const&', 'id')],
                 'Returns the plugin data previously registered with `registerPluginData`.',
                 'pyGetPluginData<T>'),
        Function('std::string', 'getFieldContents', [Param('std::string const&', 'field_id')],
                 'Returns the user input data of the field inside the Wizard.',
                 AS_LAMBDA),
        Function('void', 'setFieldContents', [
            Param('std::string const&', 'field_id'),
            Param('std::string const&', 'contents')
        ],
        'Set a field in the wizard with the specified contents. Does nothing if the field id doesnt exist.',
        AS_LAMBDA),
    ]),

    HandlerInfo("PluginPythonInterpHandler", [
        Function('void', 'pyExec', [Param('std::string const&', 'pyCode')],
                 'Executes the given python code within Codevis bundled Python interpreter',
                 AS_LAMBDA),
    ]),

    HandlerInfo("PluginContextMenuActionHandler", [
        Function('void*', 'getPluginData', [Param('std::string const&', 'id')],
                 'Returns the plugin data previously registered with `registerPluginData`.',
                 'pyGetPluginData<T>'),
        Function('std::vector<std::shared_ptr<Codethink::lvtplg::Entity>>', 'getAllEntitiesInCurrentView', [],
                 'Returns a vector of a wrapper for the entities in the current view. It is not guarantee that such entities will '
                 'be available outside the caller hook scope (So it is adviseable that you do not keep references or copies of '
                 'such entities).',
                 AS_LAMBDA),
        Function('std::shared_ptr<Codethink::lvtplg::Entity>', 'getEntityByQualifiedName', [Param('std::string const&', 'qualifiedName')],
                 'Returns one instance of a wrapper for one entity in the current view. It is not guarantee that such entities will '
                 'be available outside the caller hook scope (So it is adviseable that you do not keep references or copies of '
                 'such entities).',
                 AS_LAMBDA),
        Function('PluginTreeWidgetHandler', 'getTree', [Param('std::string const&', 'id')],
                 '',
                 AS_LAMBDA),
        Function('PluginDockWidgetHandler', 'getDock', [Param('std::string const&', 'id')],
                 '',
                 AS_LAMBDA),
        Function('std::optional<Codethink::lvtplg::Edge>', 'getEdgeByQualifiedName', [Param('std::string const&', 'fromQualifiedName'), Param('std::string const&', 'toQualifiedName')],
                 '',
                 AS_LAMBDA),
        Function('void', 'loadEntityByQualifiedName', [Param('std::string const&', 'qualifiedName')],
                 '',
                 AS_LAMBDA),
        Function('std::optional<Codethink::lvtplg::Edge>', 'addEdgeByQualifiedName', [Param('std::string const&', 'fromQualifiedName'), Param('std::string const&', 'toQualifiedName')],
                 'Creates a new edge connecting the entities with the respective qualified names. If any of those entities is not '
                 'found, will not create the edge. The edge won\'t be persisted.',
                 AS_LAMBDA),
        Function('void', 'removeEdgeByQualifiedName', [Param('std::string const&', 'fromQualifiedName'), Param('std::string const&', 'toQualifiedName')],
                 'Removes an edge connecting the entities with the respective qualified names. If connection or entities are not '
                 'found, nothing is done. This action is not persisted.',
                 AS_LAMBDA),
        Function('bool', 'hasEdgeByQualifiedName', [Param('std::string const&', 'fromQualifiedName'), Param('std::string const&', 'toQualifiedName')],
                 'Check if there\'s an edge between the entities with the respective qualified name currently on the scene. '
                 'Note that this doesn\'t necessarily mean that they have or they have not an actual dependency, since it could be '
                 'a plugin dependency.',
                 AS_LAMBDA),
        Function('Codethink::lvtplg::RawDBRows', 'runQueryOnDatabase', [Param('std::string const&', 'query')],
                 'Run a query in the active database and return it\'s results',
                 AS_LAMBDA),
    ]),

    HandlerInfo("PluginTreeItemHandler", [
        Function('void*', 'getPluginData', [Param('std::string const&', 'id')],
                 'Returns the plugin data previously registered with `registerPluginData`.',
                 'pyGetPluginData<T>'),
        Function('PluginTreeItemHandler', 'addChild', [Param('std::string const&', 'label')],
                 '',
                 AS_LAMBDA),
        Function('void', 'addUserData', [Param('std::string const&', 'dataId'), Param('void *', 'userData')],
                 '',
                 NO_BINDINGS),
        Function('void*', 'getUserData', [Param('std::string const&', 'dataId')],
                 '',
                 NO_BINDINGS),
        Function('void', 'addOnClickAction', [Param('std::function<void(PluginTreeItemClickedActionHandler *selectedItem)> const&', 'action')],
                 '',
                 NO_BINDINGS),
    ]),

    HandlerInfo("PluginTreeItemClickedActionHandler", [
        Function('void*', 'getPluginData', [Param('std::string const&', 'id')],
                 'Returns the plugin data previously registered with `registerPluginData`.',
                 'pyGetPluginData<T>'),
        Function('PluginTreeItemHandler', 'getItem', [],
                 '',
                 NO_BINDINGS),
        Function('PluginGraphicsViewHandler', 'getGraphicsView', [],
                 '',
                 NO_BINDINGS),
    ]),

    HandlerInfo("PluginTreeWidgetHandler", [
        Function('void*', 'getPluginData', [Param('std::string const&', 'id')],
                 'Returns the plugin data previously registered with `registerPluginData`.',
                 'pyGetPluginData<T>'),
        Function('PluginTreeItemHandler', 'addRootItem', [Param('std::string const&', 'label')],
                 '',
                 NO_BINDINGS),
        Function('void', 'clear', [],
                 '',
                 NO_BINDINGS),
    ]),

    HandlerInfo("PluginGraphicsViewHandler", [
        Function('std::shared_ptr<Codethink::lvtplg::Entity>', 'getEntityByQualifiedName', [Param('std::string const&', 'qualifiedName')],
                 '',
                 AS_LAMBDA),
        Function('std::vector<std::shared_ptr<Codethink::lvtplg::Entity>>', 'getVisibleEntities', [],
                 '',
                 AS_LAMBDA),
        Function('std::optional<Codethink::lvtplg::Edge>', 'getEdgeByQualifiedName', [Param('std::string const&', 'fromQualifiedName'), Param('std::string const&', 'toQualifiedName')],
                 '',
                 AS_LAMBDA),
    ]),

    HandlerInfo("PluginActiveSceneChangedHandler", [
        Function('void*', 'getPluginData', [Param('std::string const&', 'id')],
                 'Returns the plugin data previously registered with `registerPluginData`.',
                 'pyGetPluginData<T>'),
        Function('std::string', 'getSceneName', [],
                 '',
                 AS_LAMBDA),
        Function('PluginTreeWidgetHandler', 'getTree', [Param('std::string const&', 'id')],
                 '',
                 AS_LAMBDA),
    ]),

    HandlerInfo("PluginSceneDestroyedHandler", [
        Function('void*', 'getPluginData', [Param('std::string const&', 'id')],
                 'Returns the plugin data previously registered with `registerPluginData`.',
                 'pyGetPluginData<T>'),
        Function('std::string', 'getSceneName', [],
                 '',
                 AS_LAMBDA),
        Function('PluginTreeWidgetHandler', 'getTree', [Param('std::string const&', 'id')],
                 '',
                 AS_LAMBDA),
    ]),

    HandlerInfo("PluginGraphChangedHandler", [
        Function('void*', 'getPluginData', [Param('std::string const&', 'id')],
                 'Returns the plugin data previously registered with `registerPluginData`.',
                 'pyGetPluginData<T>'),
        Function('std::string', 'getSceneName', [],
                 '',
                 AS_LAMBDA),
        Function('std::vector<std::shared_ptr<Codethink::lvtplg::Entity>>', 'getVisibleEntities', [],
                 '',
                 AS_LAMBDA),
        Function('std::optional<Codethink::lvtplg::Edge>', 'getEdgeByQualifiedName', [Param('std::string const&', 'fromQualifiedName'), Param('std::string const&', 'toQualifiedName')],
                 '',
                 AS_LAMBDA),
        Function('Codethink::lvtplg::ProjectData', 'getProjectData', [],
                 '',
                 AS_LAMBDA),
    ]),

    HandlerInfo("PluginContextMenuHandler", [
        Function('void*', 'getPluginData', [Param('std::string const&', 'id')],
                 'Returns the plugin data previously registered with `registerPluginData`.',
                 'pyGetPluginData<T>'),
        Function('std::vector<std::shared_ptr<Codethink::lvtplg::Entity>>', 'getAllEntitiesInCurrentView', [],
                 'Returns a vector of a wrapper for the entities in the current view. It is not guarantee that such entities will '
                 'be available outside the caller hook scope (So it is adviseable that you do not keep references or copies of '
                 'such entities).',
                 AS_LAMBDA),
        Function('std::shared_ptr<Codethink::lvtplg::Entity>', 'getEntityByQualifiedName', [Param('std::string const&', 'qualifiedName')],
                 'Returns one instance of a wrapper for one entity in the current view. It is not guarantee that such entities will '
                 'be available outside the caller hook scope (So it is adviseable that you do not keep references or copies of '
                 'such entities).',
                 AS_LAMBDA),
        Function('void', 'registerContextMenu', [Param('std::string const&', 'title'), Param('std::function<void(PluginContextMenuActionHandler *)> const&', 'action')],
                 '',
                 AS_LAMBDA),
        Function('std::optional<Codethink::lvtplg::Edge>', 'getEdgeByQualifiedName', [Param('std::string const&', 'fromQualifiedName'), Param('std::string const&', 'toQualifiedName')],
                 '',
                 AS_LAMBDA),
    ]),

    HandlerInfo("PluginSetupDockWidgetHandler", [
        Function('void*', 'getPluginData', [Param('std::string const&', 'id')],
                 'Returns the plugin data previously registered with `registerPluginData`.',
                 'pyGetPluginData<T>'),
        Function('PluginDockWidgetHandler', 'createNewDock', [Param('std::string const&', 'dockId'), Param('std::string const&', 'title')],
                 'Creates a new dock in the GUI.',
                 AS_LAMBDA),
    ]),
    HandlerInfo("PluginDockWidgetHandler", [
        Function('void', 'addDockWdgTextField', [Param('std::string const&', 'title'), Param('std::string&', 'dataModel')],
                 'Adds a text field in the dock widget. When the field is changed, the dataModel will be automatically updated. '
                 'Make sure to manage the lifetime of the dataModel to ensure it\'s available outside the hook function\'s scope.',
                 NO_BINDINGS),
        Function('void', 'addTree', [Param('std::string const&', 'treeId')],
                 'Create a new tree in the dock widget.',
                 AS_LAMBDA),
        Function('void', 'setVisible', [Param('bool', 'visible')],
                 'Set the dock visibility.',
                 AS_LAMBDA),
    ]),


    HandlerInfo("PluginEntityMenuItemHandler", [
        Function('void*', 'getPluginData', [Param('std::string const&', 'id')],
                 'Returns the plugin data previously registered with `registerPluginData`.',
                 'pyGetPluginData<T>'),
        Function('std::shared_ptr<Codethink::lvtplg::Entity>', 'getEntity', [],
                 'Returns the active entity.',
                 AS_LAMBDA),
        Function('void', 'addAction', [Param('std::string const&', 'contextMenuTitle'), Param('std::function<void(PluginEntityMenuItemActionHandler *)>', 'action')],
                 'Setup and add a new report action in the entity context menu.',
                 AS_LAMBDA),
    ]),

    HandlerInfo("PluginEntityMenuItemActionHandler", [
        Function('void*', 'getPluginData', [Param('std::string const&', 'id')],
                 'Returns the plugin data previously registered with `registerPluginData`.',
                 'pyGetPluginData<T>'),
        Function('std::shared_ptr<Codethink::lvtplg::Entity>', 'getEntity', [],
                 '',
                 AS_LAMBDA)
    ]),

    HandlerInfo("PluginEntityReportHandler", [
        Function('void*', 'getPluginData', [Param('std::string const&', 'id')],
                 'Returns the plugin data previously registered with `registerPluginData`.',
                 'pyGetPluginData<T>'),
        Function('std::shared_ptr<Codethink::lvtplg::Entity>', 'getEntity', [],
                 'Returns the active entity.',
                 AS_LAMBDA),
        Function('void', 'addReport', [Param('std::string const&', 'contextMenuTitle'), Param('std::string const&', 'reportTitle'), Param('std::function<void(PluginEntityReportActionHandler *)>', 'action')],
                 'Setup and add a new report action in the entity context menu.',
                 AS_LAMBDA),
    ]),

    HandlerInfo("PluginEntityReportActionHandler", [
        Function('void*', 'getPluginData', [Param('std::string const&', 'id')],
                 'Returns the plugin data previously registered with `registerPluginData`.',
                 'pyGetPluginData<T>'),
        Function('std::shared_ptr<Codethink::lvtplg::Entity>', 'getEntity', [],
                 '',
                 AS_LAMBDA),
        Function('void', 'setReportContents', [Param('std::string const&', 'contentsHTML')],
                 'Set the contents of the generated report after the user clicks in the context menu action.',
                 AS_LAMBDA),
    ]),

    HandlerInfo("PluginPhysicalParserOnHeaderFoundHandler", [
        Function('void*', 'getPluginData', [Param('std::string const&', 'id')],
                 'Returns the plugin data previously registered with `registerPluginData`.',
                 'pyGetPluginData<T>'),
        Function('std::string', 'getSourceFile', [],
                 '',
                 AS_LAMBDA),
        Function('std::string', 'getIncludedFile', [],
                 '',
                 AS_LAMBDA),
        Function('unsigned', 'getLineNo', [],
                 '',
                 AS_LAMBDA),
    ]),

    HandlerInfo("PluginLogicalParserOnCppCommentFoundHandler", [
        Function('void*', 'getPluginData', [Param('std::string const&', 'id')],
                 'Returns the plugin data previously registered with `registerPluginData`.',
                 'pyGetPluginData<T>'),
        Function('std::string', 'getFilename', [],
                 '',
                 AS_LAMBDA),
        Function('std::string', 'getBriefText', [],
                 '',
                 AS_LAMBDA),
        Function('unsigned', 'getStartLine', [],
                 '',
                 AS_LAMBDA),
        Function('unsigned', 'getEndLine', [],
                 '',
                 AS_LAMBDA),
    ]),

    HandlerInfo("PluginParseCompletedHandler", [
        Function('void*', 'getPluginData', [Param('std::string const&', 'id')],
                 'Returns the plugin data previously registered with `registerPluginData`.',
                 'pyGetPluginData<T>'),
        Function('Codethink::lvtplg::RawDBRows', 'runQueryOnDatabase', [Param('std::string const&', 'query')],
                 'Run a query in the active database and return it\'s results',
                 AS_LAMBDA),
    ]),
]
