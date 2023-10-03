// ct_lvtqtw_plugineditor.t.cpp                           -*-C++-*-

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

#include <ct_lvtqtc_graphicsview.h>
#include <ct_lvtqtc_testing_utils.h>

#include <ct_lvtplg_pluginmanager.h>
#include <ct_lvtqtw_plugineditor.h>

#include <ct_lvttst_fixture_qt.h>

#include <catch2-local-includes.h>

#include <QFileInfo>
#include <QTemporaryDir>
#include <qtemporarydir.h>

using namespace Codethink::lvtqtw;

TEST_CASE_METHOD(QTApplicationFixture, "Test Plugin Editor without Manager")
{
    QTemporaryDir tempDir;

    auto *editor = new PluginEditor();
    editor->setBasePluginPath(tempDir.path());

    int numErrorMessages = 0;

    QObject::connect(editor,
                     &Codethink::lvtqtw::PluginEditor::sendErrorMsg,
                     editor,
                     [&numErrorMessages](const QString& s) {
                         qDebug() << s;
                         numErrorMessages += 1;
                     });

    editor->create("test");
    auto res = editor->save();

    // Error required because here the dialog has no plugin manager.
    REQUIRE(res.has_error());
    editor->reloadPlugin();
    editor->close();

    REQUIRE(numErrorMessages == 4);

    numErrorMessages = 0;
    Codethink::lvtplg::PluginManager manager;
    editor->setPluginManager(&manager);

    editor->create("test2");
    res = editor->save();
    // And here it should pass because we do have a plugin manager.
    REQUIRE_FALSE(res.has_error());

    editor->reloadPlugin();
    editor->close();

    REQUIRE(numErrorMessages == 0);
}
