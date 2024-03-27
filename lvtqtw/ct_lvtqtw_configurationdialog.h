// ct_lvtqtw_configurationdialog.h                               -*-C++-*-

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

#ifndef DEFINED_CT_LVTQTW_CONFIGURATIONDIALOG_H
#define DEFINED_CT_LVTQTW_CONFIGURATIONDIALOG_H

#include <lvtplg/ct_lvtplg_pluginmanager.h>
#include <lvtqtw_export.h>

#include <QDialog>

#include <memory>

#if QT_VERSION > QT_VERSION_CHECK(6, 0, 0)
#include <KNSCore/Entry>
#else
#include <KNS3/Entry>
#include <KNSCore/EntryInternal>
#endif

class QWidget;

namespace Codethink::lvtqtw {

class LVTQTW_EXPORT ConfigurationDialog : public QDialog {
    Q_OBJECT
  public:
    explicit ConfigurationDialog(lvtplg::PluginManager *pluginManager, QWidget *parent);
    ~ConfigurationDialog() override;

    void load();
    static void save();
    void restoreDefaults();
    void changeCurrentWidgetByString(QString const& text);
    void updatePluginInformation();

#if QT_VERSION > QT_VERSION_CHECK(6, 0, 0)
    void getNewScriptFinished(const QList<KNSCore::Entry>& changedEntries);
#else
    void getNewScriptFinished(const KNSCore::EntryInternal::List& changedEntries);
#endif

  protected:
    void showEvent(QShowEvent *ev) override;

  private:
    void populateMouseTabOptions();
    void loadCategoryFilteringSettings();

    struct Private;
    std::unique_ptr<Private> d;
};

} // namespace Codethink::lvtqtw

#endif
