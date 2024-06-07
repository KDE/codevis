// ct_lvtqtw_bulkedit.h                               -*-C++-*-

/*
 * // Copyright 2023 Codethink Ltd <codethink@codethink.co.uk>
 * // SPDX-License-Identifier: Apache-2.0
 * //
 * // Licensed under the Apache License, Version 2.0 (the "License");
 * // you may not use this file except in compliance with the License.
 * // You may obtain a copy of the License at
 * //
 * //     http://www.apache.org/licenses/LICENSE-2.0
 * //
 * // Unless required by applicable law or agreed to in writing, software
 * // distributed under the License is distributed on an "AS IS" BASIS,
 * // WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * // See the License for the specific language governing permissions and
 * // limitations under the License.
 */

#ifndef CT_LVTQTW_BULKEDIT_H
#define CT_LVTQTW_BULKEDIT_H

#include <QDialog>

namespace Codethink::lvtqtw {

class BulkEdit : public QDialog {
    Q_OBJECT
  public:
    BulkEdit(QWidget *parent);
    Q_SIGNAL void sendBulkJson(const QString& j);

  private:
    void loadFile(const QString& json);
};

} // namespace Codethink::lvtqtw

#endif
