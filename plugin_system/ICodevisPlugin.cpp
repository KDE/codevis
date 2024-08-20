#include <ICodevisPlugin.h>

namespace Codevis::PluginSystem {

ICodevisPlugin::ICodevisPlugin(QObject *parent, const QVariantList& args)
{
    std::ignore = parent;
    std::ignore = args;
}

} // namespace Codevis::PluginSystem
#include "moc_ICodevisPlugin.cpp"
