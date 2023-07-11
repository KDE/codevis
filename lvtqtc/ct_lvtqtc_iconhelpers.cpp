// ct_lvtqtc_iconhelpers.cpp                               -*-C++-*-

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
#include "ct_lvtqtc_iconhelpers.h"

#include <QApplication>
#include <QColor>
#include <QImage>
#include <QPainter>
#include <QPalette>
#include <QPixmap>

QIcon IconHelpers::iconFrom(const QString& resource)
{
    if (hasBrightBackground()) {
        return QIcon(resource);
    }

    auto img = QImage(resource);
    img.invertPixels();
    return QPixmap::fromImage(img);
}

QIcon IconHelpers::iconFrom(const QIcon& icon)
{
    if (hasBrightBackground()) {
        return icon;
    }

    auto sizes = icon.availableSizes();
    if (sizes.empty()) {
        return icon;
    }
    auto maximum = *std::max_element(sizes.begin(), sizes.end(), [](const QSize& a, const QSize& b) {
        return a.width() < b.width();
    });

    auto img = QImage(icon.pixmap(maximum).toImage());
    img.invertPixels();
    return QPixmap::fromImage(img);
}

bool IconHelpers::hasBrightBackground()
{
    QPalette pal = qApp->palette();
    const QColor hsvBackground = pal.window().color().toHsv();
    return hsvBackground.value() > 100;
}
