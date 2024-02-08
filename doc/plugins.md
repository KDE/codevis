Plugins
===

Codevis application supports plugins. It is possible to create plugins in C++, Python or mix the two languages.

The plugin system works as follows: There's a set of places in the application code that call user-definable functions.
 Those functions are called "Hooks". Each hook receives a pointer to a "handler" that'll be provided by the application.
 So a "hook" is a "place" _where_ the function will be called and the handler is _what_ can be called back to change
 application behavior.

All hooks are defined in the [hooks.py](../lvtplg/hooks.py) file. All handlers are defined in one of the handler files
in the [handlers.py](../lvtplg/handlers.py) file. Handlers may return application data structures, such as an "Entity".
Those are available in the [plugin data types file](../lvtplg/ct_lvtplg_plugindatatypes.h).

In order for the application to recognize the plugins, they need to be in a specific place and they need to have a
specific set of files. This is the file structure each plugin must have:

```
$plugin_name/
+ $plugin_name.[py|so]
+ metadata.json
+ README.md
```

Where `$plugin_name` is the plugin's name. `metadata.json` file follows the [kcoreaddons](https://api.kde.org/frameworks/kcoreaddons/html/)
specification (There's an example below). And the `README.md` file should contain a brief description, and
possibly examples on how to use the plugin.

The `$plugin_name` folder must be copied to one of those paths, so that the application can find the plugin:

- `$user_home/lks-plugins/` (Preferred for local development)
- `$app_installation_path/lks-plugins/` (Used for plugins that are bundled with the app)
- `$app_local_data/plugins/` (Used for downloaded plugins)

There are a few working examples in the [plugins folder](../plugins/). There's also some [plugins for integration test](../lvtplg/testplugins/)
that may be useful as simple examples. But just to give the  reader a more step-by-step explanation, this section will show a working plugin written in Python.
There's also a [python template plugin](../plugins/python_template_plugin/) that may be used as starting point.

**Step 1**: Create the following folder structure inside `$user_home/lks-plugins/` (example: `/home/tarcisio/lks-plugins/`):

```
myfirstplugin/
+ myfirstplugin.py
+ metadata.json
+ README.md
```

**Step 2**: Populate the `metadata.json` file with the following contents:

```
 {
  "KPlugin": {
     "Name": "My first plugin",
     "Description": "Example plugin",
     "Icon": "none",
     "Authors": [ { "Name": "YourName", "Email": "YourEmail" } ],
     "Category": "Codevis Plugins",
     "EnabledByDefault": true,
     "License": "YourLicense",
     "Id": "myfirstplugin",
     "Version": "0.1",
     "Website": "YourPluginWebsite"
  }
}
```

**Step 3**: Populate the `myfirstplugin.py` file with the actual plugin code:

```python
import pyLksPlugin as plg # Contains Codevis data structures

def hookSetupPlugin(handler):
# This will be executed once when the application starts
print("Hello world from Python plugin!")


def hookTeardownPlugin(handler):
# This will be executed once when the application closes
print("Okay then. Bye!")
```

**Step 4**: Save all files and start the Codevis application using your terminal. You should see the messages in the
console.

Codevis comes with a builtin plugin editor that can be used for editing the plugins. To access it just click on
`View > Plugin Editor`.