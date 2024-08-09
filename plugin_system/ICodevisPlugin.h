#pragma once

#include <QtPlugin>

#include <pluginsystemheaders_export.h>

namespace Codevis::PluginSystem {

class PLUGINSYSTEMHEADERS_EXPORT ICodevisPlugin : public QObject {
    Q_OBJECT
  public:
    ICodevisPlugin(QObject *parent, const QVariantList& args);
    virtual QString pluginName() = 0;
    virtual QString pluginDescription() = 0;
    virtual QList<QString> pluginAuthors() = 0;
};

} // namespace Codevis::PluginSystem
