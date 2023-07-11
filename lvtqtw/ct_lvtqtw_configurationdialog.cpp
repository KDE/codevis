// ct_lvtqtw_configurationdialog.cpp                               -*-C++-*-

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
#include <ui_ct_lvtqtw_configurationdialog.h>

#include <ct_lvtprj_projectfile.h>

#include <preferences.h>

#include <QDebug>
#include <QDialogButtonBox>
#include <QPushButton>

#include <cassert>

namespace Codethink::lvtqtw {

struct ConfigurationDialog::Private {
    Ui::ConfigurationDialog ui;
};

namespace {
Qt::Corner stringToCorner(const QString& txt)
{
    return txt == QObject::tr("Top Left")   ? Qt::TopLeftCorner
        : txt == QObject::tr("Top Right")   ? Qt::TopRightCorner
        : txt == QObject::tr("Bottom Left") ? Qt::BottomLeftCorner
                                            : Qt::BottomRightCorner;
}
} // namespace

ConfigurationDialog::ConfigurationDialog(QWidget *parent):
    QDialog(parent), d(std::make_unique<ConfigurationDialog::Private>())
{
    d->ui.setupUi(this);
    populateMouseTabOptions();
    load();

    auto *debugPreferences = Preferences::self()->debug();
    connect(d->ui.debugContextMenu, &QCheckBox::toggled, debugPreferences, &Debug::setEnableSceneContextMenu);
    connect(d->ui.enableDebugOutput, &QCheckBox::toggled, debugPreferences, &Debug::setEnableDebugOutput);
    connect(d->ui.storeDebugOutput, &QCheckBox::toggled, debugPreferences, &Debug::setStoreDebugOutput);

    auto *graphPreferences = Preferences::self()->window()->graphTab();
    auto *graphLoadInfo = Preferences::self()->graphLoadInfo();

    connect(d->ui.isARelation, &QCheckBox::toggled, graphLoadInfo, &GraphLoadInfo::setShowIsARelation);
    connect(d->ui.usesInTheImplementation,
            &QCheckBox::toggled,
            graphLoadInfo,
            &GraphLoadInfo::setShowUsesInTheImplementationRelation);
    connect(d->ui.usesInTheInterface,
            &QCheckBox::toggled,
            graphLoadInfo,
            &GraphLoadInfo::setShowUsesInTheInterfaceRelation);

    connect(d->ui.showClients, &QCheckBox::toggled, graphLoadInfo, &GraphLoadInfo::setShowClients);
    connect(d->ui.showProviders, &QCheckBox::toggled, graphLoadInfo, &GraphLoadInfo::setShowProviders);

    connect(d->ui.minimap, &QCheckBox::toggled, graphPreferences, &GraphTab::setShowMinimap);
    connect(d->ui.toolBox, &QCheckBox::toggled, graphPreferences, &GraphTab::setShowLegend);
    connect(d->ui.classLimit, QOverload<int>::of(&QSpinBox::valueChanged), graphPreferences, &GraphTab::setClassLimit);
    connect(d->ui.relationLimit,
            QOverload<int>::of(&QSpinBox::valueChanged),
            graphPreferences,
            &GraphTab::setRelationLimit);
    connect(d->ui.zoomLevel, QOverload<int>::of(&QSpinBox::valueChanged), graphPreferences, &GraphTab::setZoomLevel);

    auto *graphWindowPreferences = Preferences::self()->window()->graphWindow();
    connect(d->ui.comboPanModifier,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            [this, graphWindowPreferences] {
                graphWindowPreferences->setPanModifier(
                    ModifierHelpers::stringToModifier(d->ui.comboPanModifier->currentText()));
            });

    connect(d->ui.showLevelNumbers, &QCheckBox::toggled, this, [this, graphWindowPreferences] {
        graphWindowPreferences->setShowLevelNumbers(d->ui.showLevelNumbers->isChecked());
    });

    connect(d->ui.backgroundColor, &KColorButton::changed, this, [this, graphWindowPreferences] {
        graphWindowPreferences->setBackgroundColor(d->ui.backgroundColor->color());
    });
    connect(d->ui.entityBackgroundColor, &KColorButton::changed, this, [this, graphWindowPreferences] {
        graphWindowPreferences->setEntityBackgroundColor(d->ui.entityBackgroundColor->color());
    });
    connect(d->ui.selectedEntityBackgroundColor, &KColorButton::changed, this, [this, graphWindowPreferences] {
        graphWindowPreferences->setSelectedEntityBackgroundColor(d->ui.selectedEntityBackgroundColor->color());
    });
    connect(d->ui.chkSelectedEntityHasGradient,
            &QCheckBox::toggled,
            graphWindowPreferences,
            [graphWindowPreferences](bool value) {
                graphWindowPreferences->setEnableGradientOnMainNode(value);
            });
    connect(d->ui.edgeColor, &KColorButton::changed, this, [this, graphWindowPreferences] {
        graphWindowPreferences->setEdgeColor(d->ui.edgeColor->color());
    });
    connect(d->ui.highlightEdgeColor, &KColorButton::changed, this, [this, graphWindowPreferences] {
        graphWindowPreferences->setHighlightEdgeColor(d->ui.highlightEdgeColor->color());
    });

    connect(d->ui.comboZoomModifier,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            [this, graphWindowPreferences] {
                graphWindowPreferences->setZoomModifier(
                    ModifierHelpers::stringToModifier(d->ui.comboZoomModifier->currentText()));
            });
    connect(d->ui.chkColorBlindness, &QCheckBox::toggled, graphWindowPreferences, &GraphWindow::setColorBlindMode);
    connect(d->ui.chkColorPattern, &QCheckBox::toggled, graphWindowPreferences, &GraphWindow::setUseColorBlindFill);

    connect(d->ui.entityNamePos,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            [this, graphWindowPreferences] {
                graphWindowPreferences->setLakosEntityNamePos(stringToCorner(d->ui.entityNamePos->currentText()));
            });
    auto *documentPreferences = Preferences::self()->document();
    connect(d->ui.lakosianRules, &QCheckBox::toggled, documentPreferences, &Document::setUseLakosianRules);
    connect(d->ui.useDependencyTypes, &QCheckBox::toggled, documentPreferences, &Document::setUseDependencyTypes);
    connect(d->ui.showRedundantEdgesDefaultCheckbox,
            &QCheckBox::toggled,
            graphWindowPreferences,
            &GraphWindow::setShowRedundantEdgesDefault);
    connect(d->ui.hidePkgPrefixOnComponents,
            &QCheckBox::toggled,
            graphWindowPreferences,
            &GraphWindow::setHidePackagePrefixOnComponents);
    connect(d->ui.invertHorizontalLvlLayout,
            &QCheckBox::toggled,
            graphWindowPreferences,
            &GraphWindow::setInvertHorizontalLevelizationLayout);
    connect(d->ui.invertVerticalLvlLayout,
            &QCheckBox::toggled,
            graphWindowPreferences,
            &GraphWindow::setInvertVerticalLevelizationLayout);

    auto *font = Preferences::self()->window()->fonts();
    connect(d->ui.pkgGroupFont, &KFontRequester::fontSelected, font, &Fonts::setPkgGroupFont);
    connect(d->ui.pkgFont, &KFontRequester::fontSelected, font, &Fonts::setPkgFont);
    connect(d->ui.componentFont, &KFontRequester::fontSelected, font, &Fonts::setComponentFont);
    connect(d->ui.classFont, &KFontRequester::fontSelected, font, &Fonts::setClassFont);
    connect(d->ui.structFont, &KFontRequester::fontSelected, font, &Fonts::setStructFont);
    connect(d->ui.enumFont, &KFontRequester::fontSelected, font, &Fonts::setEnumFont);

    auto *doc = Preferences::self()->document();
    connect(d->ui.autoSaveBackupIntervalMsecs,
            QOverload<int>::of(&QSpinBox::valueChanged),
            doc,
            &Document::setAutoSaveBackupIntervalMsecs);

    auto *btn = d->ui.buttonBox->button(QDialogButtonBox::Save);
    auto *btnDefaults = d->ui.buttonBox->button(QDialogButtonBox::RestoreDefaults);

    connect(btnDefaults, &QPushButton::clicked, this, &ConfigurationDialog::restoreDefaults);
    connect(btn, &QPushButton::clicked, this, &ConfigurationDialog::save);

    connect(d->ui.listWidget, &QListWidget::currentItemChanged, this, [this](const QListWidgetItem *item) {
        const auto text = item->text();
        auto *selectedWidget = [&]() {
            if (text == tr("Colors")) {
                return d->ui.colorsPage;
            }
            if (text == tr("Debug")) {
                return d->ui.debugPage;
            }
            if (text == tr("Design")) {
                return d->ui.designPage;
            }
            if (text == tr("Document")) {
                return d->ui.documentPage;
            }
            if (text == tr("Graphics")) {
                return d->ui.graphicsPage;
            }
            if (text == tr("Mouse")) {
                return d->ui.mousePage;
            }
            // Default for unknown pages
            return d->ui.colorsPage;
        }();
        d->ui.stackedWidget->setCurrentWidget(selectedWidget);
    });

    const QString backupFolder = QString::fromStdString(lvtprj::ProjectFile::backupFolder().string());
    d->ui.saveFolder->setText(backupFolder);
}

ConfigurationDialog::~ConfigurationDialog() = default;

void ConfigurationDialog::load()
{
    auto *debugPreferences = Preferences::self()->debug();
    d->ui.debugContextMenu->setChecked(debugPreferences->enableSceneContextMenu());
    d->ui.enableDebugOutput->setChecked(debugPreferences->enableDebugOutput());
    d->ui.storeDebugOutput->setChecked(debugPreferences->storeDebugOutput());
    auto *graphPreferences = Preferences::self()->window()->graphTab();
    auto *graphLoadInfo = Preferences::self()->graphLoadInfo();
    d->ui.showProviders->setChecked(graphLoadInfo->showProviders());
    d->ui.showClients->setChecked(graphLoadInfo->showClients());
    d->ui.isARelation->setChecked(graphLoadInfo->showIsARelation());
    d->ui.usesInTheImplementation->setChecked(graphLoadInfo->showUsesInTheImplementationRelation());
    d->ui.usesInTheInterface->setChecked(graphLoadInfo->showUsesInTheInterfaceRelation());
    d->ui.minimap->setChecked(graphPreferences->showMinimap());
    d->ui.toolBox->setChecked(graphPreferences->showLegend());
    d->ui.classLimit->setValue(graphPreferences->classLimit());
    d->ui.relationLimit->setValue(graphPreferences->relationLimit());
    d->ui.zoomLevel->setValue(graphPreferences->zoomLevel());

    auto *graphWindowPreferences = Preferences::self()->window()->graphWindow();
    d->ui.chkColorBlindness->setChecked(graphWindowPreferences->colorBlindMode());
    d->ui.chkColorPattern->setChecked(graphWindowPreferences->useColorBlindFill());
    d->ui.showLevelNumbers->setChecked(graphWindowPreferences->showLevelNumbers());

    d->ui.comboPanModifier->setCurrentText(
        ModifierHelpers::modifierToText(static_cast<Qt::KeyboardModifier>(graphWindowPreferences->panModifier())));
    d->ui.comboZoomModifier->setCurrentText(
        ModifierHelpers::modifierToText(static_cast<Qt::KeyboardModifier>(graphWindowPreferences->zoomModifier())));

    d->ui.backgroundColor->setColor(graphWindowPreferences->backgroundColor());
    d->ui.entityBackgroundColor->setColor(graphWindowPreferences->entityBackgroundColor());
    d->ui.selectedEntityBackgroundColor->setColor(graphWindowPreferences->selectedEntityBackgroundColor());
    d->ui.edgeColor->setColor(graphWindowPreferences->edgeColor());
    d->ui.highlightEdgeColor->setColor(graphWindowPreferences->highlightEdgeColor());

    const Qt::Corner cnr = graphWindowPreferences->lakosEntityNamePos();
    d->ui.entityNamePos->setCurrentText(cnr == Qt::TopLeftCorner          ? tr("Top Left")
                                            : cnr == Qt::TopRightCorner   ? tr("Top Right")
                                            : cnr == Qt::BottomLeftCorner ? tr("Bottom Left")
                                                                          : tr("Bottom Right"));

    auto *font = Preferences::self()->window()->fonts();
    d->ui.pkgGroupFont->setFont(font->pkgGroupFont());
    d->ui.pkgFont->setFont(font->pkgFont());
    d->ui.componentFont->setFont(font->componentFont());
    d->ui.classFont->setFont(font->classFont());
    d->ui.structFont->setFont(font->structFont());
    d->ui.enumFont->setFont(font->enumFont());

    auto *document = Preferences::self()->document();
    d->ui.autoSaveBackupIntervalMsecs->setValue(document->autoSaveBackupIntervalMsecs());

    auto *documentPreferences = Preferences::self()->document();
    d->ui.lakosianRules->setChecked(documentPreferences->useLakosianRules());
    d->ui.useDependencyTypes->setChecked(documentPreferences->useDependencyTypes());
    d->ui.showRedundantEdgesDefaultCheckbox->setChecked(graphWindowPreferences->showRedundantEdgesDefault());
    d->ui.hidePkgPrefixOnComponents->setChecked(graphWindowPreferences->hidePackagePrefixOnComponents());
    d->ui.hidePkgPrefixOnComponents->setChecked(graphWindowPreferences->invertHorizontalLevelizationLayout());
    d->ui.hidePkgPrefixOnComponents->setChecked(graphWindowPreferences->invertVerticalLevelizationLayout());
}

void ConfigurationDialog::save()
{
    Preferences::self()->sync();
}

void ConfigurationDialog::restoreDefaults()
{
    Preferences::self()->loadDefaults();
    load();
}

void ConfigurationDialog::changeCurrentWidgetByString(QString const& text)
{
    auto items = d->ui.listWidget->findItems(text, Qt::MatchFlag::MatchExactly);
    if (items.isEmpty()) {
        // Option not found
        return;
    }
    d->ui.listWidget->setCurrentItem(items[0]);
}

void ConfigurationDialog::populateMouseTabOptions()
{
    std::initializer_list<QComboBox *> comboBoxes = {
        d->ui.comboPanModifier,
        d->ui.comboZoomModifier,
    };
    for (QComboBox *combo : comboBoxes) {
#ifdef __APPLE__
        combo->addItem(tr("OPTION"));
#else
        combo->addItem(tr("ALT"));
#endif

#ifdef __APPLE__
        combo->addItem(tr("COMMAND"));
#else
        combo->addItem(tr("CONTROL"));
#endif

        combo->addItem(tr("SHIFT"));
        combo->addItem(tr("No modifier"));
    }
}

} // namespace Codethink::lvtqtw
