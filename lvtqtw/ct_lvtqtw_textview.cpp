// ct_lvtqtw_textview.cpp                                             -*-C++-*-

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

#include <ct_lvtqtw_textview.h>

#include <QIODevice>
#include <QTabWidget>
#include <QTemporaryFile>
#include <QUuid>

using namespace Codethink::lvtqtw;

struct TextView::Private {
    int id;
    QTemporaryFile tmp;

    explicit Private(int inId): id(inId)
    {
    }
};

TextView::TextView(int id, QWidget *parent): QTextEdit(parent), d(std::make_unique<TextView::Private>(id))
{
}

TextView::~TextView() = default;

void TextView::appendText(const QString& text)
{
    if (text.isEmpty()) {
        return;
    }

    if (!d->tmp.isOpen()) {
        d->tmp.open();
    }
    d->tmp.seek(d->tmp.size());
    d->tmp.write(text.toLocal8Bit() + "\n");

    if (isVisible()) {
        QTextEdit::append(text);
    }
}

void TextView::showEvent(QShowEvent *ev)
{
    Q_UNUSED(ev);
    d->tmp.seek(0);
    setText(d->tmp.readAll());
}

void TextView::hideEvent(QHideEvent *ev)
{
    Q_UNUSED(ev);
    setText(QString());
}

void TextView::saveFileTo(const QString& path)
{
    d->tmp.seek(0);
    d->tmp.copy(path);
}

#include "moc_ct_lvtqtw_textview.cpp"
