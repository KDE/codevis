// ct_lvtqtc_inputdialog.h                                    -*-C++-*-

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

#ifndef INCLUDED_CT_LVTQTC_NAME_TOOLTIP_DIALOG
#define INCLUDED_CT_LVTQTC_NAME_TOOLTIP_DIALOG

#include <lvtqtc_export.h>

#include <QByteArray>
#include <QDialog>
#include <QLabel>
#include <QString>

#include <any>
#include <functional>
#include <memory>
#include <vector>

namespace Codethink::lvtqtc {
class LVTQTC_EXPORT InputDialog : public QDialog {
    // dialog that returns the name and tooltip of a created entity
    // upon accepted.
    // should not be used if rejected.

    Q_OBJECT
  public:
    explicit InputDialog(const QString& title = tr("Input Dialog"), QWidget *parent = nullptr);
    ~InputDialog() override;

    // List of pair - field name, field text.
    void prepareFields(const std::vector<std::pair<QByteArray, QString>>& fields);
    [[nodiscard]] virtual std::any fieldValue(const QByteArray& fieldId) const;

    void addTextField(const QByteArray& fieldId, const QString& labelText);
    void setTextFieldValue(const QByteArray& fieldId, const QString& value);

    void addComboBoxField(const QByteArray& fieldId, const QString& labelText, const std::vector<QString>& options);
    void addSpinBoxField(const QByteArray& fieldId, const QString& labelText, std::pair<int, int> minMax, int value);
    void finish();

    int exec() override;

    void overrideExec(std::function<bool(void)> exec);
    // should be used only for testing purposes.

  protected:
    void registerField(const QByteArray& fieldId, const QString& labelText, QWidget *widget);
    void showEvent(QShowEvent *ev) override;

  private:
    struct Private;
    std::unique_ptr<Private> d;
};

} // namespace Codethink::lvtqtc

#endif
