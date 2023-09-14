// ct_lvtgclr_colormanagement.cpp                                    -*-C++-*-

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

#include <QCryptographicHash>

#include <QDebug>
#include <preferences.h>

#include <unordered_map>
#include <vector>

// clazy:excludeall=qcolor-from-literal

namespace Codethink::lvtclr {

struct ColorManagement::Private {
    QColor baseColor;
    std::unordered_map<std::string, QColor> colorMap;
    std::unordered_map<std::string, QColor> userDefinedColorMap;

    std::unordered_map<std::string, Qt::BrushStyle> fillMap;

    /* 'v used the data here: http://mkweb.bcgsc.ca/colorblind/ and did
     * a few iterations of software that
     * can simulate color blindness over kwin.
     * - https://www.qt.io/blog/2017/09/15/testing-applications-color-blindness
     */
    const std::vector<QColor> colorBlindPalette = {
        "#E8384F", // Red
        "#FD817D", // orange
        "#FDAE33", // yellow-orange
        "#EECC16", // yellow
        "#A4C61A", // yellow-green
        "#62BB35", // "green"
        "#37A862", // "blue-green"
        "#8D9F9B", // "cool-gray"
        "#208EA3", // "aqua"
        "#4178BC", // "blue"
        "#7A71F6", // "indigo"
        "#AA71FF", // "purple"
        "#E37CFF", // "magenta"
        "#EA4E90", // "hot-pink"
        "#FCA7E4" // "pink"
    };

    const std::vector<Qt::BrushStyle> brushPatterns = {Qt::Dense1Pattern,
                                                       Qt::Dense2Pattern,
                                                       Qt::Dense3Pattern,
                                                       Qt::Dense4Pattern,
                                                       Qt::Dense5Pattern,
                                                       Qt::Dense6Pattern,
                                                       Qt::Dense7Pattern,
                                                       Qt::HorPattern,
                                                       Qt::VerPattern,
                                                       Qt::CrossPattern,
                                                       Qt::BDiagPattern,
                                                       Qt::FDiagPattern,
                                                       Qt::DiagCrossPattern};
};

ColorManagement::ColorManagement(bool doConnectSignals): d(std::make_unique<ColorManagement::Private>())
{
    // Start the ColorManagement with a white base color.
    d->baseColor = QColor(255, 255, 255);

    if (doConnectSignals) {
        connect(Preferences::self(), &Preferences::useColorBlindFillChanged, this, &ColorManagement::colorBlindChanged);
        connect(Preferences::self(), &Preferences::colorBlindModeChanged, this, &ColorManagement::colorBlindChanged);
    }
}

ColorManagement::~ColorManagement() = default;

void ColorManagement::setBaseColor(const QColor& color)
{
    d->baseColor = color;
    resetCaches();
}

void ColorManagement::colorBlindChanged()
{
    resetCaches();
    Q_EMIT requestNewColor();
}

Qt::BrushStyle ColorManagement::fillPattern(const std::string& id)
{
    if (!isUsingColorBlindFill()) {
        return Qt::BrushStyle::SolidPattern;
    }

    auto it = d->fillMap.find(id);
    if (it != d->fillMap.end()) {
        return it->second;
    }

    static size_t currentPattern = 0;
    Qt::BrushStyle retValue = d->brushPatterns[currentPattern];
    currentPattern = (currentPattern + 1) % d->brushPatterns.size();
    d->fillMap[id] = retValue;

    return retValue;
}

QColor ColorManagement::getColorFor(const std::string& id)
{
    // User defined colors are always preferred
    if (d->userDefinedColorMap.find(id) != d->userDefinedColorMap.end()) {
        return d->userDefinedColorMap[id];
    }

    // If used doesn't have any preferences, generate
    if (d->colorMap.find(id) == d->colorMap.end()) {
        // Could not find cached color. Calculate a new one.
        if (isColorBlindModeActive()) {
            d->colorMap[id] = generateColorblindColor();
        } else {
            d->colorMap[id] = generateUniqueColor(id);
        }
    }
    return d->colorMap[id];
}

void ColorManagement::setColorFor(const std::string& id, const QColor& color)
{
    d->userDefinedColorMap[id] = color;
}

QColor ColorManagement::generateUniqueColor(const std::string& id) const
{
    // given the fact that a single byte will change completely
    // the results of a md5 hash, we can use that to use the first
    // three letters as unsigned chars (so, range 0 - 255) to get the
    // color value for red, green and blue. This removes the randomness
    // of the colors on the view.
    auto md5Algorithm = QCryptographicHash(QCryptographicHash::Algorithm::Md5);
    md5Algorithm.addData(QString::fromStdString(id + "salt").toLocal8Bit());
    auto md5 = md5Algorithm.result().toStdString();

    int red = (unsigned char) md5[0];
    int green = (unsigned char) md5[1];
    int blue = (unsigned char) md5[2];

    red = (red + d->baseColor.red()) / 2;
    green = (green + d->baseColor.green()) / 2;
    blue = (blue + d->baseColor.blue()) / 2;

    return {red, green, blue};
}

QColor ColorManagement::generateColorblindColor() const
{
    int idx = 0;
    bool found = false;
    bool brigthen = false;
    bool darken = false;

    QColor currColor;
    do {
        currColor = d->colorBlindPalette[idx];
        if (brigthen) {
            currColor = currColor.lighter();
        } else if (darken) {
            currColor = currColor.darker();
        }

        found = std::find_if(std::begin(d->colorMap),
                             std::end(d->colorMap),
                             [currColor](const std::pair<std::string, QColor>& color) {
                                 return color.second == currColor;
                             })
            != std::end(d->colorMap);

        idx += 1;
        if (idx == (int) d->colorBlindPalette.size()) {
            idx = 0;
            // too many colors. return the currently tested color. we tested
            // more than 45 colors here, it's unlikely that the amount of classes will
            // trigger this.
            if (brigthen && darken) {
                break;
            }
            if (!darken) {
                darken = true;
                continue;
            }
            if (!brigthen) {
                brigthen = true;
                continue;
            }
        }
    } while (found);

    return currColor;
}

void ColorManagement::resetCaches()
{
    d->colorMap.clear();
    d->fillMap.clear();
}

bool ColorManagement::isColorBlindModeActive() const
{
    return Preferences::colorBlindMode();
}

bool Codethink::lvtclr::ColorManagement::isUsingColorBlindFill() const
{
    return Preferences::useColorBlindFill();
}

} // namespace Codethink::lvtclr
