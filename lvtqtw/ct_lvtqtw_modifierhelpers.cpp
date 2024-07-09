// ct_lvtqtw_modifierhelpers.cpp                                                                               -*-C++-*-

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

#include <ct_lvtqtw_modifierhelpers.h>

namespace Codethink::lvtqtw {

Qt::KeyboardModifier ModifierHelpers::stringToModifier(const QString& txt)
{
#ifdef __APPLE__
    if (txt == QObject::tr("OPTION")) {
        return Qt::KeyboardModifier::AltModifier;
    }
    if (txt == QObject::tr("COMMAND")) {
        return Qt::KeyboardModifier::ControlModifier;
    }
    if (txt == QObject::tr("SHIFT")) {
        return Qt::KeyboardModifier::ShiftModifier;
    }
#else
    if (txt == QObject::tr("ALT")) {
        return Qt::KeyboardModifier::AltModifier;
    }
    if (txt == QObject::tr("CONTROL")) {
        return Qt::KeyboardModifier::ControlModifier;
    }
    if (txt == QObject::tr("SHIFT")) {
        return Qt::KeyboardModifier::ShiftModifier;
    }
#endif
    if (txt == QObject::tr("No modifier")) {
        return Qt::KeyboardModifier::NoModifier;
    }

    assert(false);
    return Qt::KeyboardModifier::NoModifier;
}

QString ModifierHelpers::modifierToText(Qt::KeyboardModifier modifier)
{
    if (modifier == Qt::KeyboardModifier::AltModifier) {
#ifdef __APPLE__
        return tr("OPTION");
#else
        return tr("ALT");
#endif
    }
    if (modifier == Qt::KeyboardModifier::ControlModifier) {
#ifdef __APPLE__
        return tr("COMMAND");
#else
        return tr("CONTROL");
#endif
    }
    if (modifier == Qt::KeyboardModifier::ShiftModifier) {
        return tr("SHIFT");
    }
    if (modifier == Qt::KeyboardModifier::NoModifier) {
        return tr("No modifier");
    }
    return tr("Unknown modifier");
}

} // namespace Codethink::lvtqtw
