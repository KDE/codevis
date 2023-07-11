// ct_lvtqtc_ellipsistextitem.cpp                                     -*-C++-*-

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

#include <ct_lvtqtc_ellipsistextitem.h>

#include <QFontMetrics>
#include <QString>

namespace {

constexpr QString::size_type MAXIMUM_LABEL_LENGTH = 15;
}

namespace Codethink::lvtqtc {

struct EllipsisTextItem::Private {
    EllipsisTextItem::Truncate truncate = EllipsisTextItem::Truncate::Yes;
    QString fullText;
};

EllipsisTextItem::EllipsisTextItem(const QString& text, QGraphicsItem *parent):
    QGraphicsSimpleTextItem(parent), d(std::make_unique<EllipsisTextItem::Private>())
{
    d->fullText = text;
    setText(text);
}

EllipsisTextItem::~EllipsisTextItem() = default;

void EllipsisTextItem::setText(const QString& text)
{
    d->fullText = text;
    setupText();
}

void EllipsisTextItem::truncate(EllipsisTextItem::Truncate v)
{
    d->truncate = v;
    setupText();
}

void EllipsisTextItem::setupText()
{
    QString myText = d->fullText;
    if (d->truncate == EllipsisTextItem::Truncate::Yes && d->fullText.length() > MAXIMUM_LABEL_LENGTH) {
        myText.truncate(MAXIMUM_LABEL_LENGTH);
        myText += "...";
    }
    QGraphicsSimpleTextItem::setText(myText);
}

} // namespace Codethink::lvtqtc
