// ct_lvtqtw_statusbar.t.cpp                                                                                   -*-C++-*-

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

#include <ct_lvtqtw_statusbar.h>

#include <ct_lvttst_fixture_qt.h>
#include <preferences.h>

#include <catch2-local-includes.h>

using namespace Codethink::lvtqtw;

class FakeParseCodebaseDialog : public ParseCodebaseDialog {
  public:
    void fakeParseStartedSignal(ParseCodebaseDialog::State s)
    {
        Q_EMIT parseStarted(s);
    }

    void fakeParseStepSignal(ParseCodebaseDialog::State s, int value, int max)
    {
        Q_EMIT parseStep(s, value, max);
    }

    void fakeParseFinished()
    {
        Q_EMIT parseFinished(Codethink::lvtqtw::ParseCodebaseDialog::State::Idle);
    }
};

class CodeVisStatusBarForTesting : public CodeVisStatusBar {
  public:
    [[nodiscard]] QString currentPanText() const
    {
        return m_labelPan->text();
    }

    [[nodiscard]] QString currentZoomText() const
    {
        return m_labelZoom->text();
    }

    void clickParseCodebaseStatus()
    {
        m_labelParseCodebaseWindowStatus->click();
    }

    QString parseCodebaseStatusText()
    {
        return m_labelParseCodebaseWindowStatus->text();
    }
};

TEST_CASE_METHOD(QTApplicationFixture, "Basic status bar workflow")
{
    auto *preferences = Preferences::self()->window()->graphWindow();
    preferences->setPanModifier(Qt::ALT);
    preferences->setZoomModifier(Qt::CTRL);

    auto statusBar = CodeVisStatusBarForTesting{};
    statusBar.show();

#ifdef __APPLE__
    REQUIRE(statusBar.currentPanText() == "Pan Graph: OPTION + Click");
    REQUIRE(statusBar.currentZoomText() == "Zoom: COMMAND + Wheel");
#else
    REQUIRE(statusBar.currentPanText() == "Pan Graph: ALT + Click");
    REQUIRE(statusBar.currentZoomText() == "Zoom: CONTROL + Wheel");
#endif

    preferences->setPanModifier(Qt::SHIFT);
    preferences->setZoomModifier(Qt::ALT);

#ifdef __APPLE__
    REQUIRE(statusBar.currentPanText() == "Pan Graph: Click + SHIFT");
    REQUIRE(statusBar.currentZoomText() == "Zoom: Wheel + OPTION");
#else
    REQUIRE(statusBar.currentPanText() == "Pan Graph: SHIFT + Click");
    REQUIRE(statusBar.currentZoomText() == "Zoom: ALT + Wheel");
#endif
}

TEST_CASE_METHOD(QTApplicationFixture, "Logical parse on status bar")
{
    auto fakeParseDialog = FakeParseCodebaseDialog{};
    fakeParseDialog.hide();

    auto statusBar = CodeVisStatusBarForTesting{};
    statusBar.show();

    // Must do nothing if there's no parseCodebaseWindow set.
    statusBar.clickParseCodebaseStatus();
    REQUIRE(fakeParseDialog.isHidden());

    statusBar.setParseCodebaseWindow(fakeParseDialog);
    statusBar.clickParseCodebaseStatus();
    REQUIRE(fakeParseDialog.isVisible());

    // Nothing is shown for the physical analysis
    REQUIRE(statusBar.parseCodebaseStatusText().isEmpty());
    fakeParseDialog.fakeParseStartedSignal(ParseCodebaseDialog::State::Idle);
    REQUIRE(statusBar.parseCodebaseStatusText().isEmpty());
    fakeParseDialog.fakeParseStartedSignal(ParseCodebaseDialog::State::RunAllPhysical);
    REQUIRE(statusBar.parseCodebaseStatusText().isEmpty());
    fakeParseDialog.fakeParseStepSignal(ParseCodebaseDialog::State::RunAllPhysical, 0, 1);
    REQUIRE(statusBar.parseCodebaseStatusText() == "Running physical parse [0/1].");

    // Text is updated for logical analysis
    fakeParseDialog.fakeParseStartedSignal(ParseCodebaseDialog::State::RunAllLogical);
    fakeParseDialog.fakeParseStepSignal(ParseCodebaseDialog::State::RunAllLogical, 0, 1);
    REQUIRE(statusBar.parseCodebaseStatusText() == "Running logical parse [0/1]. Some diagrams may be incomplete.");
    fakeParseDialog.fakeParseStepSignal(ParseCodebaseDialog::State::RunAllLogical, 1, 1);
    REQUIRE(statusBar.parseCodebaseStatusText() == "Running logical parse [1/1]. Some diagrams may be incomplete.");
    fakeParseDialog.fakeParseFinished();
    REQUIRE(statusBar.parseCodebaseStatusText() == "View last parse run");
}
