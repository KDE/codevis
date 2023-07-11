// ct_lvtqtw_statusbar.h                                                                                       -*-C++-*-

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

#ifndef DIAGRAM_SERVER_STATUSBAR_H
#define DIAGRAM_SERVER_STATUSBAR_H

#include <lvtqtw_export.h>

#include <QLabel>
#include <QPushButton>
#include <QStatusBar>
#include <ct_lvtqtw_parse_codebase.h>
#include <optional>

namespace Codethink::lvtqtw {

class LVTQTW_EXPORT CodeVisStatusBar : public QStatusBar {
    Q_OBJECT

  public:
    CodeVisStatusBar();
    void setParseCodebaseWindow(ParseCodebaseDialog& dialog);
    void reset();

    Q_SIGNAL void mouseInteractionLabelClicked();

  protected:
    void updatePanText(int newModifier);
    void updateZoomText(int newModifier);

    void handleParseStart(ParseCodebaseDialog::State);
    void handleParseStep(ParseCodebaseDialog::State, int, int);
    void handleParseFinished();
    void openParseCodebaseWindow() const;

    QPushButton *m_labelPan = nullptr;
    QPushButton *m_labelZoom = nullptr;

    QPushButton *m_labelParseCodebaseWindowStatus = nullptr;
    ParseCodebaseDialog *m_parseCodebaseDialog = nullptr;
    // If set, will observe and give status from the dialog
};

} // namespace Codethink::lvtqtw

#endif // DIAGRAM_SERVER_STATUSBAR_H
