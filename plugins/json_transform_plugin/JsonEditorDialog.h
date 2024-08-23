#pragma once

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

#include <QDialog>

#include <ct_lvtqtc_graphicsscene.h>

namespace KTextEditor {
class View;
}

class KMessageWidget;

class BulkEdit : public QDialog {
    Q_OBJECT
  public:
    BulkEdit(QWidget *parent);
    void setScene(Codethink::lvtqtc::GraphicsScene *scene);
    void triggerJsonChange();
    void loadFile(const QString& json);
    void saveFile();

  private:
    Codethink::lvtqtc::GraphicsScene *_scene;
    KTextEditor::View *_view;
    KMessageWidget *_messageWidget;
};
