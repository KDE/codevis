// ct_lvtqtw_textview.t.cpp

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

#include <catch2/catch.hpp>

#include <QDir>
#include <ct_lvtqtw_textview.cpp>
#include <ct_lvttst_fixture_qt.h>
#include <qevent.h>

using namespace Codethink::lvtqtw;

TEST_CASE_METHOD(QTApplicationFixture, "Append text to TextView")
{
    TextView tv(1);
    tv.show();
    // smoke test
    tv.appendText("");
    tv.appendText("Hello");
    tv.appendText("world");
    REQUIRE(tv.toPlainText().toStdString() == "Hello\nworld");
    tv.hide();
    REQUIRE(tv.toPlainText().toStdString().empty());
    tv.show();
    REQUIRE(tv.toPlainText().trimmed().toStdString() == "Hello\nworld");
}

TEST_CASE_METHOD(QTApplicationFixture, "Save text to file from TextView")
{
    // create a TextView object
    TextView tv(1);
    tv.show();
    const QString testText = "This Is a Unit Test";
    // test appendText method
    tv.appendText(testText);
    REQUIRE(tv.toPlainText() == testText);
    // prepare directory
    QString filePath = QDir::tempPath() + QDir::separator() + "test.txt";
    QFile file(filePath);
    file.remove();
    // test saveFileTo method
    tv.saveFileTo(filePath);
    // Reading saved File content
    QString savedText;
    REQUIRE(file.open(QIODevice::ReadOnly | QIODevice::Text));
    while (!file.atEnd()) {
        QByteArray line = file.readLine();
        savedText.append(line);
    }
    // Check that the text was saved correctly
    REQUIRE(savedText.trimmed().toStdString() == testText.toStdString());
}
