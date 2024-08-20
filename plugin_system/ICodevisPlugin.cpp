#include <ICodevisPlugin.h>

namespace Codevis::PluginSystem {

ICodevisPlugin::ICodevisPlugin(QObject *parent): QObject(parent)
{
}

ICodevisPlugin::~ICodevisPlugin() = default;

} // namespace Codevis::PluginSystem
#include "moc_ICodevisPlugin.cpp"
