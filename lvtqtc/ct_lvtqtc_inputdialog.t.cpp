// ct_lvtqtc_inputdialog.t.cpp                               -*-C++-*-

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
#include <ct_lvtqtc_inputdialog.h>

#include <QLineEdit>
#include <QSpinBox>

#include <catch2-local-includes.h>

#include <ct_lvttst_fixture_qt.h>

TEST_CASE_METHOD(QTApplicationFixture, "Usage with prepareFields")
{
    Codethink::lvtqtc::InputDialog dlg;

    // a is the key value, and the objectName of the lineEdit. SomeText is the label text.
    dlg.prepareFields({{"a", QString("SomeText")}, {"b", QString("OtherText")}});

    // QLineEdit is internal and not exposed API.
    const auto lineEdits = dlg.findChildren<QLineEdit *>();
    for (auto *lineEdit : lineEdits) {
        lineEdit->setText(lineEdit->objectName());
    }

    REQUIRE(std::any_cast<QString>(dlg.fieldValue("a")) == "a");
    REQUIRE(std::any_cast<QString>(dlg.fieldValue("b")) == "b");
}

TEST_CASE_METHOD(QTApplicationFixture, "Usage with Add*Field")
{
    Codethink::lvtqtc::InputDialog dlg;

    dlg.addTextField("a", "label");
    dlg.addComboBoxField("b", "otherlabel", {QString("one"), QString("two"), QString("three")});
    dlg.addSpinBoxField("c", "numericLabel", {0, 99}, 50);
    dlg.finish();

    const auto lineEdits = dlg.findChildren<QLineEdit *>();
    for (auto *lineEdit : lineEdits) {
        lineEdit->setText(lineEdit->objectName());
    }
    auto *spinBox = dlg.findChildren<QSpinBox *>().at(0);
    spinBox->setValue(10);

    REQUIRE(std::any_cast<QString>(dlg.fieldValue("a")) == "a");
    REQUIRE(std::any_cast<QString>(dlg.fieldValue("b")) == "one");
    REQUIRE(std::any_cast<int>(dlg.fieldValue("c")) == 10);
}
