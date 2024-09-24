#pragma once

#include <pluginsystemheaders_export.h>

class QMainWindow;

namespace Codevis::PluginSystem {
/* This interface should be used when the plugin needs to react to changes
 * on the graphics scene.
 */
class IMainWindowPlugin {
  public:
    virtual void mainWindowReady(QMainWindow *mainWindow) = 0;
};
} // namespace Codevis::PluginSystem
