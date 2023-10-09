// ct_lvtqtw_plugineditor.h                                             -*-C++-*-

/*
// Copyright 2023 Codethink Ltd <codethink@codethink.co.uk>
// SPDX-License-Identifier: Apache-2.0
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#ifndef INCLUDED_LVTQTW_PLUGINEDITOR
#define INCLUDED_LVTQTW_PLUGINEDITOR

#include <QDir>
#include <QWidget>

#include <memory>

#include <lvtqtw_export.h>
#include <result/result.hpp>

#include <result/result.hpp>

#if QT_VERSION > QT_VERSION_CHECK(6, 0, 0)
#include <KNSCore/Entry>
#else
#include <KNS3/Entry>
#include <KNSCore/EntryInternal>
#endif

namespace Codethink::lvtplg {
class PluginManager;
}

namespace Codethink::lvtqtw {

class LVTQTW_EXPORT PluginEditor : public QWidget {
    Q_OBJECT
  public:
    PluginEditor(QWidget *parent = 0);
    ~PluginEditor();

    enum class e_Errors {
        Error,
    };

    struct Error {
        e_Errors errNr;
        std::string errStr;
    };

    Q_SIGNAL void execute(const QString& plugin);
    Q_SIGNAL void sendErrorMsg(const QString& err);

    void setPluginManager(lvtplg::PluginManager *manager);
    cpp::result<void, Error> save();
    void close();
    void loadByName(const QString& pluginName);
    void load();
    void create(const QString& pluginName = QString());
    void reloadPlugin();

#if QT_VERSION > QT_VERSION_CHECK(6, 0, 0)
    void getNewScriptFinished(const QList<KNSCore::Entry>& changedEntries);
#else
    void getNewScriptFinished(const KNSCore::EntryInternal::List& changedEntries);
#endif

    // Used for testing purposes. defaults to ~/lks-plugins
    void setBasePluginPath(const QString& path);
    QDir basePluginPath();

  private:
    struct Private;
    std::unique_ptr<Private> d;
};

} // namespace Codethink::lvtqtw
#endif
