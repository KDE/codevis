PLUGIN_ID = "pyTestPlugin";


print("Module LOADED!!!")


class DataModel:
    pass


def hookSetupPlugin(h):
    print("Hello world from PYTHON PLUGIN!!")
    h.registerPluginData(PLUGIN_ID, DataModel())


def hookTeardownPlugin(h):
    d = h.getPluginData(PLUGIN_ID)
    # print(d)
    h.unregisterPluginData(PLUGIN_ID)
