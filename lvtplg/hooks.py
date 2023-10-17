from collections import namedtuple

HookInfo = namedtuple('HookInfo', ['name', 'handler', 'comments'])

HOOKS = [
    HookInfo("SetupPlugin", "PluginSetupHandler", "This hook is called as soon as the application initializes, and should be used to setup plugin data structures."),
    HookInfo("TeardownPlugin", "PluginSetupHandler", "This hook is called just before the application closes, and must be used to cleanup any resource the plugin acquired."),
    HookInfo("GraphicsViewContextMenu", "PluginContextMenuHandler", "Hook to control the graphics view context menu."),
    HookInfo("SetupDockWidget", "PluginDockWidgetHandler", "Can be used to setup new dock widgets (See PluginDockWidgetHandler)"),
    HookInfo("SetupEntityReport", "PluginEntityReportHandler", "If implemented, will generate an action in the reports menu to create a HTML report."),
    HookInfo("PhysicalParserOnHeaderFound", "PluginPhysicalParserOnHeaderFoundHandler", "Called every time a header is found in the physical parser."),
    HookInfo("LogicalParserOnCppCommentFound", "PluginLogicalParserOnCppCommentFoundHandler", "Called every time a comment is found in the logical parser."),
    HookInfo("OnParseCompleted", "PluginParseCompletedHandler", "Called after the Physical and Logical (if enabled) parsing are done."),
    HookInfo("ActiveSceneChanged", "PluginActiveSceneChangedHandler", "Called when the active scene is changed in the GUI."),
    HookInfo("MainNodeChanged", "PluginMainNodeChangedHandler", "Called when the main node of a given graphics scene has changed."),
]
