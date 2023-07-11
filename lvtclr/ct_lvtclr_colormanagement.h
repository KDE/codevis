// ct_lvtgclr_colormanagement.h                                      -*-C++-*-

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

#ifndef INCLUDED_CT_LVTCLR_COLORMANAGEMENT_H
#define INCLUDED_CT_LVTCLR_COLORMANAGEMENT_H

#include <lvtclr_export.h>

#include <memory>

#include <QColor>
#include <QObject>

namespace Codethink::lvtclr {

// =====================
// class ColorManagement
// =====================

class LVTCLR_EXPORT ColorManagement : public QObject
// Class that helps managing the ColorSchemes for the visual graph
{
    Q_OBJECT
  public:
    ColorManagement(bool doConnectSignals = true);
    // Constructor
    ~ColorManagement() override;
    // Destructor

    // MANIPULATORS
    void setBaseColor(const QColor& color);
    // Sets the base color for the palette generation

    void setColorFor(const std::string& id, const QColor& color);
    // Sets the color for a particular id

    QColor getColorFor(const std::string& id);
    // Returns the stored color value for the id, or generates a new one if it does not exist.

    Qt::BrushStyle fillPattern(const std::string& id);

    Q_SIGNAL void requestNewColor();
    // signals the nodes that they should request a new color.

    Q_SLOT void colorBlindChanged();
    // connected to the Settings, and triggered when the information about colorblindness changes.

  protected:
    [[nodiscard]] virtual bool isColorBlindModeActive() const;
    [[nodiscard]] virtual bool isUsingColorBlindFill() const;

    void resetCaches();

  private:
    [[nodiscard]] QColor generateUniqueColor(const std::string& id) const;
    [[nodiscard]] QColor generateColorblindColor() const;

    struct Private;
    std::unique_ptr<Private> d;
};

} // end namespace Codethink::lvtclr

#endif
