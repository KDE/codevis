#pragma once

#include <QtPlugin>

#include <pluginsystemheaders_export.h>

namespace Codevis::PluginSystem {

class PLUGINSYSTEMHEADERS_EXPORT ICodevisPlugin : public QObject {
    Q_OBJECT
  public:
    ICodevisPlugin(QObject *parent);
    ~ICodevisPlugin() override;
};

} // namespace Codevis::PluginSystem
