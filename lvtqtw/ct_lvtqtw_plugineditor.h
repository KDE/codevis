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

namespace Codethink::lvtqtw {

class LVTQTW_EXPORT PluginEditor : public QWidget {
    Q_OBJECT
  public:
    PluginEditor(QWidget *parent);
    ~PluginEditor();

    Q_SIGNAL void execute(const QString& plugin);
    Q_SIGNAL void sendErrorMsg(const QString& err);

    void save();
    void close();
    void loadByName(const QString& pluginName);
    void load();
    void create();

    static QDir basePluginPath();

  private:
    struct Private;
    std::unique_ptr<Private> d;
};

} // namespace Codethink::lvtqtw
#endif
