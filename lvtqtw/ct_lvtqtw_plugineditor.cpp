// ct_lvtqtw_plugineditor.cpp                                             -*-C++-*-

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

#include <ct_lvtqtw_plugineditor.h>

#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/View>

#include <QBoxLayout>

using namespace Codethink::lvtqtw;

struct PluginEditor::Private {
    KTextEditor::Document *doc;
    KTextEditor::View *view;
};

PluginEditor::PluginEditor(QWidget *parent): QWidget(parent), d(std::make_unique<PluginEditor::Private>())
{
    auto editor = KTextEditor::Editor::instance();

    d->doc = editor->createDocument(this);
    d->view = d->doc->createView(this);

    auto *l = new QBoxLayout(QBoxLayout::BottomToTop);
    l->addWidget(d->view);
    setLayout(l);
}

PluginEditor::~PluginEditor() = default;
