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

#include <KPluginWidget>
#include <cassert>

namespace Codethink::lvtqtw {

struct ConfigurationDialog::Private {
    Ui::ConfigurationDialog ui;
    KPluginWidget *pluginWidget;
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
    this->setWindowTitle("Configure Software");

    d->pluginWidget = new KPluginWidget(this);
    d->ui.pluginsPageLayout->addWidget(d->pluginWidget);

    populateMouseTabOptions();
    load();

    connect(d->ui.debugContextMenu, &QCheckBox::toggled, Preferences::self(), &Preferences::setEnableSceneContextMenu);
    connect(d->ui.enableDebugOutput, &QCheckBox::toggled, Preferences::self(), &Preferences::setEnableDebugOutput);
    connect(d->ui.storeDebugOutput, &QCheckBox::toggled, Preferences::self(), &Preferences::setStoreDebugOutput);

    connect(d->ui.isARelation, &QCheckBox::toggled, Preferences::self(), &Preferences::setShowIsARelation);
    connect(d->ui.usesInTheImplementation,
            &QCheckBox::toggled,
            Preferences::self(),
            &Preferences::setShowUsesInTheImplementationRelation);
    connect(d->ui.usesInTheInterface,
            &QCheckBox::toggled,
            Preferences::self(),
            &Preferences::setShowUsesInTheInterfaceRelation);

    connect(d->ui.showClients, &QCheckBox::toggled, Preferences::self(), &Preferences::setShowClients);
    connect(d->ui.showProviders, &QCheckBox::toggled, Preferences::self(), &Preferences::setShowProviders);

    connect(d->ui.minimap, &QCheckBox::toggled, Preferences::self(), &Preferences::setShowMinimap);
    connect(d->ui.toolBox, &QCheckBox::toggled, Preferences::self(), &Preferences::setShowLegend);
    connect(d->ui.classLimit,
            QOverload<int>::of(&QSpinBox::valueChanged),
            Preferences::self(),
            &Preferences::setClassLimit);
    connect(d->ui.relationLimit,
            QOverload<int>::of(&QSpinBox::valueChanged),
            Preferences::self(),
            &Preferences::setRelationLimit);
    connect(d->ui.zoomLevel,
            QOverload<int>::of(&QSpinBox::valueChanged),
            Preferences::self(),
            &Preferences::setZoomLevel);

    connect(d->ui.comboPanModifier, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this] {
        Preferences::setPanModifier(ModifierHelpers::stringToModifier(d->ui.comboPanModifier->currentText()));
    });

    connect(d->ui.showLevelNumbers, &QCheckBox::toggled, this, [this] {
        Preferences::setShowLevelNumbers(d->ui.showLevelNumbers->isChecked());
    });

    connect(d->ui.backgroundColor, &KColorButton::changed, this, [this] {
        Preferences::setBackgroundColor(d->ui.backgroundColor->color());
    });
    connect(d->ui.entityBackgroundColor, &KColorButton::changed, this, [this] {
        Preferences::setEntityBackgroundColor(d->ui.entityBackgroundColor->color());
    });
    connect(d->ui.selectedEntityBackgroundColor, &KColorButton::changed, this, [this] {
        Preferences::setSelectedEntityBackgroundColor(d->ui.selectedEntityBackgroundColor->color());
    });
    connect(d->ui.chkSelectedEntityHasGradient,
            &QCheckBox::toggled,
            Preferences::self(),
            &Preferences::setEnableGradientOnMainNode);

    connect(d->ui.edgeColor, &KColorButton::changed, this, [this] {
        Preferences::setEdgeColor(d->ui.edgeColor->color());
    });
    connect(d->ui.highlightEdgeColor, &KColorButton::changed, this, [this] {
        Preferences::setHighlightEdgeColor(d->ui.highlightEdgeColor->color());
    });

    connect(d->ui.comboZoomModifier, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this] {
        Preferences::setZoomModifier(ModifierHelpers::stringToModifier(d->ui.comboZoomModifier->currentText()));
    });
    connect(d->ui.chkColorBlindness, &QCheckBox::toggled, Preferences::self(), &Preferences::setColorBlindMode);
    connect(d->ui.chkColorPattern, &QCheckBox::toggled, Preferences::self(), &Preferences::setUseColorBlindFill);

    connect(d->ui.entityNamePos, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this] {
        Preferences::setLakosEntityNamePos(stringToCorner(d->ui.entityNamePos->currentText()));
    });

    connect(d->ui.lakosianRules, &QCheckBox::toggled, Preferences::self(), &Preferences::setUseLakosianRules);
    connect(d->ui.showRedundantEdgesDefaultCheckbox,
            &QCheckBox::toggled,
            Preferences::self(),
            &Preferences::setShowRedundantEdgesDefault);
    connect(d->ui.hidePkgPrefixOnComponents,
            &QCheckBox::toggled,
            Preferences::self(),
            &Preferences::setHidePackagePrefixOnComponents);
    connect(d->ui.invertHorizontalLvlLayout,
            &QCheckBox::toggled,
            Preferences::self(),
            &Preferences::setInvertHorizontalLevelizationLayout);
    connect(d->ui.invertVerticalLvlLayout,
            &QCheckBox::toggled,
            Preferences::self(),
            &Preferences::setInvertVerticalLevelizationLayout);

    connect(d->ui.pkgGroupFont, &KFontRequester::fontSelected, Preferences::self(), &Preferences::setPkgGroupFont);
    connect(d->ui.pkgFont, &KFontRequester::fontSelected, Preferences::self(), &Preferences::setPkgFont);
    connect(d->ui.componentFont, &KFontRequester::fontSelected, Preferences::self(), &Preferences::setComponentFont);
    connect(d->ui.classFont, &KFontRequester::fontSelected, Preferences::self(), &Preferences::setClassFont);
    connect(d->ui.structFont, &KFontRequester::fontSelected, Preferences::self(), &Preferences::setStructFont);
    connect(d->ui.enumFont, &KFontRequester::fontSelected, Preferences::self(), &Preferences::setEnumFont);

    connect(d->ui.autoSaveBackupIntervalMsecs,
            QOverload<int>::of(&QSpinBox::valueChanged),
            Preferences::self(),
            &Preferences::setAutoSaveBackupIntervalMsecs);

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
            if (text == tr("Plugins")) {
                return d->ui.pluginsPage;
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

void ConfigurationDialog::updatePluginInformation(lvtplg::PluginManager& pluginManager)
{
    auto plugins = QVector<KPluginMetaData>{};
    QString categoryLabel = "Codevis Plugins";
    for (auto const& metadataFile : pluginManager.getPluginsMetadataFilePaths()) {
        plugins.push_back(KPluginMetaData::fromJsonFile(QString::fromStdString(metadataFile)));
    }

    d->pluginWidget->clear();
    d->pluginWidget->addPlugins(plugins, categoryLabel);
    connect(d->pluginWidget,
            &KPluginWidget::pluginEnabledChanged,
            [&pluginManager](const QString& pluginId, bool enabled) {
                auto plugin = pluginManager.getPluginById(pluginId.toStdString());
                if (!plugin) {
                    return;
                }
                plugin->get().setEnabled(enabled);
            });
}

void ConfigurationDialog::load()
{
    d->ui.debugContextMenu->setChecked(Preferences::enableSceneContextMenu());
    d->ui.enableDebugOutput->setChecked(Preferences::enableDebugOutput());
    d->ui.storeDebugOutput->setChecked(Preferences::storeDebugOutput());
    d->ui.showProviders->setChecked(Preferences::showProviders());
    d->ui.showClients->setChecked(Preferences::showClients());
    d->ui.isARelation->setChecked(Preferences::showIsARelation());
    d->ui.usesInTheImplementation->setChecked(Preferences::showUsesInTheImplementationRelation());
    d->ui.usesInTheInterface->setChecked(Preferences::showUsesInTheInterfaceRelation());
    d->ui.minimap->setChecked(Preferences::showMinimap());
    d->ui.toolBox->setChecked(Preferences::showLegend());
    d->ui.classLimit->setValue(Preferences::classLimit());
    d->ui.relationLimit->setValue(Preferences::relationLimit());
    d->ui.zoomLevel->setValue(Preferences::zoomLevel());

    d->ui.chkColorBlindness->setChecked(Preferences::colorBlindMode());
    d->ui.chkColorPattern->setChecked(Preferences::useColorBlindFill());
    d->ui.showLevelNumbers->setChecked(Preferences::showLevelNumbers());

    d->ui.comboPanModifier->setCurrentText(
        ModifierHelpers::modifierToText(static_cast<Qt::KeyboardModifier>(Preferences::panModifier())));
    d->ui.comboZoomModifier->setCurrentText(
        ModifierHelpers::modifierToText(static_cast<Qt::KeyboardModifier>(Preferences::zoomModifier())));

    d->ui.backgroundColor->setColor(Preferences::backgroundColor());
    d->ui.entityBackgroundColor->setColor(Preferences::entityBackgroundColor());
    d->ui.selectedEntityBackgroundColor->setColor(Preferences::selectedEntityBackgroundColor());
    d->ui.edgeColor->setColor(Preferences::edgeColor());
    d->ui.highlightEdgeColor->setColor(Preferences::highlightEdgeColor());

    const Qt::Corner cnr = static_cast<Qt::Corner>(Preferences::lakosEntityNamePos());
    d->ui.entityNamePos->setCurrentText(cnr == Qt::TopLeftCorner          ? tr("Top Left")
                                            : cnr == Qt::TopRightCorner   ? tr("Top Right")
                                            : cnr == Qt::BottomLeftCorner ? tr("Bottom Left")
                                                                          : tr("Bottom Right"));

    d->ui.pkgGroupFont->setFont(Preferences::pkgGroupFont());
    d->ui.pkgFont->setFont(Preferences::pkgFont());
    d->ui.componentFont->setFont(Preferences::componentFont());
    d->ui.classFont->setFont(Preferences::classFont());
    d->ui.structFont->setFont(Preferences::structFont());
    d->ui.enumFont->setFont(Preferences::enumFont());

    d->ui.autoSaveBackupIntervalMsecs->setValue(Preferences::autoSaveBackupIntervalMsecs());

    d->ui.lakosianRules->setChecked(Preferences::useLakosianRules());
    d->ui.showRedundantEdgesDefaultCheckbox->setChecked(Preferences::showRedundantEdgesDefault());
    d->ui.hidePkgPrefixOnComponents->setChecked(Preferences::hidePackagePrefixOnComponents());
    d->ui.hidePkgPrefixOnComponents->setChecked(Preferences::invertHorizontalLevelizationLayout());
    d->ui.hidePkgPrefixOnComponents->setChecked(Preferences::invertVerticalLevelizationLayout());
}

void ConfigurationDialog::save()
{
    Preferences::self()->save();
}

void ConfigurationDialog::restoreDefaults()
{
    // TODO: Figure out how to load defaults.
    Preferences::self()->setDefaults();
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
