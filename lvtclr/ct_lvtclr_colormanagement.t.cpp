// ct_lvtgclr_colormanagement.t.cpp                                    -*-C++-*-

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

#include <ct_lvtclr_colormanagement.h>

#include <catch2-local-includes.h>

#include <iostream>

using namespace Codethink::lvtclr;

// Extends ColorManagement to avoid touching 'Preferences' directly
class ColorManagementForTesting : public ColorManagement {
  public:
    ColorManagementForTesting(): ColorManagement(false)
    {
    }

    void setColorBlindMode(bool new_value)
    {
        if (enableColorBlindMode != new_value) {
            enableColorBlindMode = new_value;
            resetCaches();
        }
    }

    void setColorBlindFill(bool new_value)
    {
        if (enableColorBlindFill != new_value) {
            enableColorBlindFill = new_value;
            resetCaches();
        }
    }

  protected:
    [[nodiscard]] bool isColorBlindModeActive() const override
    {
        return enableColorBlindMode;
    }
    [[nodiscard]] bool isUsingColorBlindFill() const override
    {
        return enableColorBlindFill;
    }

  private:
    bool enableColorBlindMode = false;
    bool enableColorBlindFill = false;
};

TEST_CASE("Get color from manager")
{
    auto c = ColorManagementForTesting{};
    c.setColorBlindMode(false);
    auto some_color = c.getColorFor("some id");

    // Getting color for the same id, should return the same color
    REQUIRE(c.getColorFor("some id") == some_color);
    // But getting for a different id, should return a diferent color
    REQUIRE(c.getColorFor("some other id") != some_color);

    // If color blind mode is activated, generate a new color
    c.setColorBlindMode(true);
    REQUIRE(c.getColorFor("some id") != some_color);
    c.setColorBlindMode(false);
    REQUIRE(c.getColorFor("some id") == some_color);

    // User is able to override the color for a given id...
    auto new_color = QColor("#123456");
    c.setColorFor("some id", new_color);
    REQUIRE(c.getColorFor("some id") == new_color);
    // ...and this color is always preferred
    c.setColorBlindMode(true);
    REQUIRE(c.getColorFor("some id") == new_color);
}

TEST_CASE("Chance base color")
{
    auto c = ColorManagementForTesting{};
    c.setColorBlindMode(false);
    auto some_color = c.getColorFor("some id");

    // Changing the base color should affect the automatic color generation
    REQUIRE(c.getColorFor("some id") == some_color);
    c.setBaseColor(QColor{0, 0, 0});
    REQUIRE(c.getColorFor("some id") != some_color);

    // But not the user defined color
    auto new_color = QColor("#123456");
    c.setColorFor("some id", new_color);
    REQUIRE(c.getColorFor("some id") == new_color);
    c.setBaseColor(QColor{0, 255, 0});
    REQUIRE(c.getColorFor("some id") == new_color);
}

TEST_CASE("Get fill pattern from manager")
{
    auto c = ColorManagementForTesting{};
    c.setColorBlindFill(false);
    auto some_fill = c.fillPattern("some id");

    // With colorFill disabled, all ids return the same fill
    REQUIRE(c.fillPattern("some id") == some_fill);
    REQUIRE(c.fillPattern("some other id") == some_fill);

    // With colorFill enabled, fill patterns may be different for each id
    c.setColorBlindFill(true);
    some_fill = c.fillPattern("some id");
    REQUIRE(c.fillPattern("some id") == some_fill);
    REQUIRE(c.fillPattern("some other id") != some_fill);
}
