// ct_lvtqtw_configurationdialog.t.cpp                               -*-C++-*-

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

#include <ct_lvtqtw_configurationdialog.h>
#include <ct_lvtqtw_modifierhelpers.h>
#include <ct_lvttst_fixture_qt.h>
#include <preferences.h>

#include <catch2-local-includes.h>
#include <ui_ct_lvtqtw_configurationdialog.h>

#include <QObject>

using namespace Codethink::lvtqtw;

TEST_CASE_METHOD(QTApplicationFixture, "Smoke Test ConfigurationDialog class")
{
    SECTION("Test load function")
    {
        ConfigurationDialog configDialog(nullptr);
        REQUIRE_NOTHROW(configDialog.load());
    }

    SECTION("Test save static function")
    {
        REQUIRE_NOTHROW(ConfigurationDialog::save());
    }

    SECTION("Test restoreDefaults function")
    {
        ConfigurationDialog configDialog(nullptr);
        REQUIRE_NOTHROW(configDialog.restoreDefaults());
    }

    SECTION("Test changeCurrentWidgetByString function")
    {
        ConfigurationDialog configDialog(nullptr);
        auto *listWidget = configDialog.findChild<QListWidget *>("listWidget");
        REQUIRE_NOTHROW(configDialog.changeCurrentWidgetByString("Test Sample"));

        for (int i = 0; i < listWidget->count(); i++) {
            const QString t = listWidget->item(i)->text();
            configDialog.changeCurrentWidgetByString(t);
            REQUIRE(listWidget->currentItem()->text() == t);
        }

        listWidget->item(0)->setText(QString());
        REQUIRE_NOTHROW(listWidget->setCurrentRow(0));
    }

    SECTION("Test entityNamePos ")
    {
        ConfigurationDialog configDialog(nullptr);
        auto *entityNamePos = configDialog.findChild<QComboBox *>("entityNamePos");
        for (int i = 0; i < entityNamePos->count(); i++) {
            REQUIRE_NOTHROW(entityNamePos->setCurrentIndex(i));
        }
    }

    SECTION("Test comboPanModifier")
    {
        ConfigurationDialog configDialog(nullptr);
        auto *comboPanModifier = configDialog.findChild<QComboBox *>("comboPanModifier");
        REQUIRE_NOTHROW(comboPanModifier->setCurrentIndex(0));
        REQUIRE_NOTHROW(comboPanModifier->setCurrentIndex(comboPanModifier->count() - 1));
    }

    SECTION("Test showLevelNumbers")
    {
        ConfigurationDialog configDialog(nullptr);
        auto *showLevelNumbers = configDialog.findChild<QCheckBox *>("showLevelNumbers");
        REQUIRE_NOTHROW(showLevelNumbers->setCheckState(Qt::Checked));
        REQUIRE_NOTHROW(showLevelNumbers->setCheckState(Qt::Unchecked));
    }

    SECTION("Test backgroundColor")
    {
        ConfigurationDialog configDialog(nullptr);
        auto *backgroundColor = configDialog.findChild<KColorButton *>("backgroundColor");
        REQUIRE_NOTHROW(backgroundColor->setColor(Qt::red));
        REQUIRE_NOTHROW(backgroundColor->setColor(Qt::blue));
    }

    SECTION("Test entityBackgroundColor")
    {
        ConfigurationDialog configDialog(nullptr);
        auto *entityBackgroundColor = configDialog.findChild<KColorButton *>("entityBackgroundColor");
        REQUIRE_NOTHROW(entityBackgroundColor->setColor(Qt::red));
        REQUIRE_NOTHROW(entityBackgroundColor->setColor(Qt::blue));
    }
    SECTION("Test selectedEntityBackgroundColor")
    {
        ConfigurationDialog configDialog(nullptr);
        auto *selectedEntityBackgroundColor = configDialog.findChild<KColorButton *>("selectedEntityBackgroundColor");
        REQUIRE_NOTHROW(selectedEntityBackgroundColor->setColor(Qt::red));
        REQUIRE_NOTHROW(selectedEntityBackgroundColor->setColor(Qt::blue));
    }

    SECTION("Test chkSelectedEntityHasGradient")
    {
        ConfigurationDialog configDialog(nullptr);
        auto *chkSelectedEntityHasGradient = configDialog.findChild<QCheckBox *>("chkSelectedEntityHasGradient");
        REQUIRE_NOTHROW(chkSelectedEntityHasGradient->setCheckState(Qt::Checked));
        REQUIRE_NOTHROW(chkSelectedEntityHasGradient->setCheckState(Qt::Unchecked));
    }

    SECTION("Test edgeColor")
    {
        ConfigurationDialog configDialog(nullptr);
        auto *edgeColor = configDialog.findChild<KColorButton *>("edgeColor");
        REQUIRE_NOTHROW(edgeColor->setColor(Qt::red));
        REQUIRE_NOTHROW(edgeColor->setColor(Qt::blue));
    }
    SECTION("Test highlightEdgeColor")
    {
        ConfigurationDialog configDialog(nullptr);
        auto *highlightEdgeColor = configDialog.findChild<KColorButton *>("highlightEdgeColor");
        REQUIRE_NOTHROW(highlightEdgeColor->setColor(Qt::red));
        REQUIRE_NOTHROW(highlightEdgeColor->setColor(Qt::blue));
    }

    SECTION("Test comboZoomModifier")
    {
        ConfigurationDialog configDialog(nullptr);
        auto *comboZoomModifier = configDialog.findChild<QComboBox *>("comboZoomModifier");
        REQUIRE_NOTHROW(comboZoomModifier->setCurrentIndex(0));
        REQUIRE_NOTHROW(comboZoomModifier->setCurrentIndex(comboZoomModifier->count() - 1));
    }
}

TEST_CASE_METHOD(QTApplicationFixture, "Test ConfigurationDialog class Functionality")
{
    ConfigurationDialog configDialog(nullptr);
    auto *toolBox = configDialog.findChild<QCheckBox *>("toolBox");
    REQUIRE(toolBox != nullptr);
    SECTION("Test load function")
    {
        const bool t = Preferences::self()->showLegend();
        REQUIRE((bool) toolBox->checkState() == Preferences::self()->showLegend());
        Preferences::self()->setShowLegend(!t);
        REQUIRE((bool) toolBox->checkState() != Preferences::self()->showLegend());
        configDialog.load();
        REQUIRE((bool) toolBox->checkState() == Preferences::self()->showLegend());
    }

    SECTION("Test UI effect on Preferences")
    {
        toolBox->setCheckState(Qt::Unchecked);
        REQUIRE((bool) toolBox->checkState() == Preferences::self()->showLegend());

        toolBox->setCheckState(Qt::Checked);
        REQUIRE((bool) toolBox->checkState() == Preferences::self()->showLegend());

        toolBox->setCheckState(Qt::Unchecked);
        REQUIRE((bool) toolBox->checkState() == Preferences::self()->showLegend());
    }

    SECTION("Test restoreDefaults function")
    {
        const bool t = Preferences::self()->showLegend();
        REQUIRE((bool) toolBox->checkState() == Preferences::self()->showLegend());
        Preferences::self()->setShowLegend(!t);
        REQUIRE((bool) toolBox->checkState() != Preferences::self()->showLegend());
        configDialog.restoreDefaults();
        REQUIRE((bool) toolBox->checkState() == Preferences::self()->showLegend());
    }
}
