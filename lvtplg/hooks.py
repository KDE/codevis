from collections import namedtuple

HookInfo = namedtuple('HookInfo', ['return_type', 'name', 'handler', 'docs'])

HOOKS = [
    HookInfo("void", "SetupPlugin", "PluginSetupHandler", "This hook is called as soon as the application initializes, and should be used to setup plugin data structures."),
    HookInfo("void", "MainWindowReady", "PluginMainWindowReadyHandler", "Called as soon as the mainWindow is ready."),
    HookInfo("void", "TeardownPlugin", "PluginSetupHandler", "This hook is called just before the application closes, and must be used to cleanup any resource the plugin acquired."),
    HookInfo("void", "GraphicsViewContextMenu", "PluginContextMenuHandler", "Hook to control the graphics view context menu."),
    HookInfo("void", "SetupDockWidget", "PluginSetupDockWidgetHandler", "Can be used to setup new dock widgets (See PluginDockWidgetHandler)"),
    HookInfo("void", "SetupEntityMenu", "PluginEntityMenuItemHandler", "Creates a new Menu element on the context menu of an entity"),
    HookInfo("void", "SetupEntityReport", "PluginEntityReportHandler", "If implemented, will generate an action in the reports menu to create a HTML report."),
    HookInfo("void", "PhysicalParserOnHeaderFound", "PluginPhysicalParserOnHeaderFoundHandler", "Called every time a header is found in the physical parser."),
    HookInfo("void", "LogicalParserOnCppCommentFound", "PluginLogicalParserOnCppCommentFoundHandler", "Called every time a comment is found in the logical parser."),
    HookInfo("void", "OnParseCompleted", "PluginParseCompletedHandler", "Called after the Physical and Logical (if enabled) parsing are done."),
    HookInfo("void", "ActiveSceneChanged", "PluginActiveSceneChangedHandler", "Called when the active scene is changed in the GUI."),
    HookInfo("void", "SceneDestroyed", "PluginSceneDestroyedHandler", "Called when a scene is destroyed."),
    HookInfo("void", "GraphChanged", "PluginGraphChangedHandler", "Called when graph has changed."),
]
