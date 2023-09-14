// ct_lvtqtw_statusbar.cpp                                                                                     -*-C++-*-

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

#include <ct_lvtqtw_modifierhelpers.h>
#include <ct_lvtqtw_statusbar.h>
#include <preferences.h>

namespace Codethink::lvtqtw {

CodeVisStatusBar::CodeVisStatusBar():
    m_labelPan(new QPushButton(this)),
    m_labelZoom(new QPushButton(this)),
    m_labelParseCodebaseWindowStatus(new QPushButton(this))
{
    addWidget(m_labelPan);
    addWidget(m_labelZoom);
    m_labelPan->setFlat(true);
    m_labelZoom->setFlat(true);
    connect(m_labelPan, &QPushButton::clicked, this, &CodeVisStatusBar::mouseInteractionLabelClicked);
    connect(m_labelZoom, &QPushButton::clicked, this, &CodeVisStatusBar::mouseInteractionLabelClicked);

    addWidget(new QLabel(" | ", this));

    addWidget(m_labelParseCodebaseWindowStatus);
    m_labelParseCodebaseWindowStatus->hide();
    m_labelParseCodebaseWindowStatus->setFlat(true);
    m_labelParseCodebaseWindowStatus->setStyleSheet("font-weight: bold; color: #F19143;");
    connect(m_labelParseCodebaseWindowStatus, &QPushButton::clicked, this, &CodeVisStatusBar::openParseCodebaseWindow);

    connect(Preferences::self(), &Preferences::panModifierChanged, this, [this] {
        updatePanText(Preferences::panModifier());
    });
    connect(Preferences::self(), &Preferences::zoomModifierChanged, this, [this] {
        updateZoomText(Preferences::zoomModifier());
    });

    updatePanText(Preferences::panModifier());
    updateZoomText(Preferences::zoomModifier());
}

void CodeVisStatusBar::updatePanText(int newModifier)
{
    auto modifier = static_cast<Qt::KeyboardModifier>(newModifier);
    if (modifier == Qt::KeyboardModifier::NoModifier) {
        m_labelPan->setText(tr("Pan Graph: Click"));
    } else {
        m_labelPan->setText(tr("Pan Graph: %1 + Click").arg(ModifierHelpers::modifierToText(modifier)));
    }
}

void CodeVisStatusBar::updateZoomText(int newModifier)
{
    auto modifier = static_cast<Qt::KeyboardModifier>(newModifier);
    if (modifier == Qt::KeyboardModifier::NoModifier) {
        m_labelZoom->setText(tr("Zoom: Wheel"));
    } else {
        m_labelZoom->setText(tr("Zoom: %1 + Wheel").arg(ModifierHelpers::modifierToText(modifier)));
    }
}

void CodeVisStatusBar::reset()
{
    m_labelParseCodebaseWindowStatus->setText(QString{""});
    m_labelParseCodebaseWindowStatus->hide();
}

void CodeVisStatusBar::setParseCodebaseWindow(ParseCodebaseDialog& dialog)
{
    if (m_parseCodebaseDialog) {
        disconnect(m_parseCodebaseDialog,
                   &ParseCodebaseDialog::parseStarted,
                   this,
                   &CodeVisStatusBar::handleParseStart);
        disconnect(m_parseCodebaseDialog, &ParseCodebaseDialog::parseStep, this, &CodeVisStatusBar::handleParseStep);
        disconnect(m_parseCodebaseDialog,
                   &ParseCodebaseDialog::parseFinished,
                   this,
                   &CodeVisStatusBar::handleParseFinished);
    }
    m_parseCodebaseDialog = &dialog;
    connect(m_parseCodebaseDialog, &ParseCodebaseDialog::parseStarted, this, &CodeVisStatusBar::handleParseStart);
    connect(m_parseCodebaseDialog, &ParseCodebaseDialog::parseStep, this, &CodeVisStatusBar::handleParseStep);
    connect(m_parseCodebaseDialog, &ParseCodebaseDialog::parseFinished, this, &CodeVisStatusBar::handleParseFinished);
}

void CodeVisStatusBar::handleParseStart(ParseCodebaseDialog::State state)
{
    m_labelParseCodebaseWindowStatus->show();
}

void CodeVisStatusBar::handleParseStep(ParseCodebaseDialog::State state, int currentProgress, int maxProgress)
{
    if (state == ParseCodebaseDialog::State::RunAllLogical) {
        auto message = std::string("Running logical parse [");
        message += std::to_string(currentProgress) + "/" + std::to_string(maxProgress);
        message += "]. Some diagrams may be incomplete.";
        m_labelParseCodebaseWindowStatus->setText(QString::fromStdString(message));
    }

    if (state == ParseCodebaseDialog::State::RunAllPhysical || state == ParseCodebaseDialog::State::RunPhysicalOnly) {
        auto message = std::string("Running physical parse [");
        message += std::to_string(currentProgress) + "/" + std::to_string(maxProgress);
        message += "].";
        m_labelParseCodebaseWindowStatus->setText(QString::fromStdString(message));
    }
}

void CodeVisStatusBar::handleParseFinished()
{
    m_labelParseCodebaseWindowStatus->setText(QString{"View last parse run"});
}

void CodeVisStatusBar::openParseCodebaseWindow() const
{
    if (!m_parseCodebaseDialog) {
        return;
    }
    m_parseCodebaseDialog->show();
}

} // namespace Codethink::lvtqtw
