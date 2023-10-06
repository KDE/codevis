PLUGIN_ID = "pyTestPlugin";


class DataModel:
    pass


def hookSetupPlugin(h):
    print("Hello world from PYTHON PLUGIN!!")
    h.registerPluginData(PLUGIN_ID, DataModel())


def hookTeardownPlugin(h):
    d = h.getPluginData(PLUGIN_ID)
    h.unregisterPluginData(PLUGIN_ID)
