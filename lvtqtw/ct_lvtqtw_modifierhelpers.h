// ct_lvtqtw_modifierhelpers.h                                                                                 -*-C++-*-

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

#ifndef DIAGRAM_SERVER_CT_LVTQTW_MODIFIERHELPERS_H
#define DIAGRAM_SERVER_CT_LVTQTW_MODIFIERHELPERS_H

#include <QObject>
#include <QString>

namespace Codethink::lvtqtw {

class ModifierHelpers : public QObject {
  public:
    static Qt::KeyboardModifier stringToModifier(const QString& txt);
    static QString modifierToText(Qt::KeyboardModifier modifier);
};

} // namespace Codethink::lvtqtw

#endif // DIAGRAM_SERVER_CT_LVTQTW_MODIFIERHELPERS_H
