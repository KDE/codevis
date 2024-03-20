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

// own
#include <ct_lvtplg_pluginmanager.h>
#include <ct_lvtqtw_configurationdialog.h>

#include <ct_lvtprj_projectfile.h>
#include <ct_lvtqtw_modifierhelpers.h>

// autogen
#include <preferences.h>
#ifdef KDE_FRAMEWORKS_IS_OLD
#include <ui_ct_lvtqtw_configurationdialog_oldkf5.h>
#else
#include <ui_ct_lvtqtw_configurationdialog.h>
#endif

// Qt
#include <QDebug>
#include <QDialogButtonBox>
#include <QPushButton>

// KDE
#include <KMessageBox>
#ifndef KDE_FRAMEWORKS_IS_OLD
#include <KPluginWidget>
#endif
#include <QCheckBox>
#include <ct_lvtshr_debug_categories.h>

// TODO: this is a workaround for enable/disable uncategorised qDebug, qInfo, etc. logs temporarily.
// As qDebug, qInfo, etc. logs has "default" category by default.
// PS: No need to declare this in every class that has uncategorised logs. This is added here once
// just to be able to make checkboxes and filtering for enabling/disabling them easily.
// Should be removed after every uncategorised qDebugs are sorted into a category.
CODEVIS_LOGGING_CATEGORIES(uncategorised, "default")

namespace Codethink::lvtqtw {

struct ConfigurationDialog::Private {
    Ui::ConfigurationDialog ui;
    Codethink::lvtplg::PluginManager *pluginManager;
    QStringList enabledDebugCategoryNames;
    QStringList enabledInfoCategoryNames;
    QStringList enabledWarningCategoryNames;
    QStringList enabledCriticalCategoryNames;
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

ConfigurationDialog::ConfigurationDialog(lvtplg::PluginManager *pluginManager, QWidget *parent):
    QDialog(parent), d(std::make_unique<ConfigurationDialog::Private>())
{
    d->ui.setupUi(this);
    d->pluginManager = pluginManager;
#ifndef KDE_FRAMEWORKS_IS_OLD
    d->ui.getNewPlugins->setConfigFile(QStringLiteral("codevis.knsrc"));
#endif

    setWindowTitle("Configure Software");

    populateMouseTabOptions();
    load();

#ifndef KDE_FRAMEWORKS_IS_OLD
    connect(d->ui.getNewPlugins,
            &KNSWidgets::Button::dialogFinished,
            this,
            &Codethink::lvtqtw::ConfigurationDialog::getNewScriptFinished);
#endif
    connect(d->ui.debugContextMenu, &QCheckBox::toggled, Preferences::self(), &Preferences::setEnableSceneContextMenu);
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
    connect(d->ui.comboZoomModifier, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this] {
        Preferences::setZoomModifier(ModifierHelpers::stringToModifier(d->ui.comboZoomModifier->currentText()));
    });
    connect(d->ui.comboMultiSelectModifier, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this] {
        Preferences::setMultiSelectModifier(
            ModifierHelpers::stringToModifier(d->ui.comboMultiSelectModifier->currentText()));
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
    connect(d->ui.entityHoverColor, &KColorButton::changed, this, [this] {
        Preferences::setEntityHoverColor(d->ui.entityHoverColor->color());
    });

    connect(d->ui.edgeColor, &KColorButton::changed, this, [this] {
        Preferences::setEdgeColor(d->ui.edgeColor->color());
    });
    connect(d->ui.highlightEdgeColor, &KColorButton::changed, this, [this] {
        Preferences::setHighlightEdgeColor(d->ui.highlightEdgeColor->color());
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

    connect(d->ui.spaceBetweenLevels,
            QOverload<int>::of(&QSpinBox::valueChanged),
            Preferences::self(),
            &Preferences::setSpaceBetweenLevels);
    connect(d->ui.spaceBetweenSublevels,
            QOverload<int>::of(&QSpinBox::valueChanged),
            Preferences::self(),
            &Preferences::setSpaceBetweenSublevels);
    connect(d->ui.spaceBetweenEntities,
            QOverload<int>::of(&QSpinBox::valueChanged),
            Preferences::self(),
            &Preferences::setSpaceBetweenEntities);
    connect(d->ui.maxEntitiesPerLevel,
            QOverload<int>::of(&QSpinBox::valueChanged),
            Preferences::self(),
            &Preferences::setMaxEntitiesPerLevel);

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

void ConfigurationDialog::showEvent(QShowEvent *ev)
{
    updatePluginInformation();
}

void ConfigurationDialog::updatePluginInformation()
{
#ifndef KDE_FRAMEWORKS_IS_OLD
    auto plugins = QVector<KPluginMetaData>{};
    QString categoryLabel = "Codevis Plugins";
    for (auto const& metadataFile : d->pluginManager->getPluginsMetadataFilePaths()) {
        plugins.push_back(KPluginMetaData::fromJsonFile(QString::fromStdString(metadataFile)));
    }

    d->ui.pluginWidget->clear();
    d->ui.pluginWidget->addPlugins(plugins, categoryLabel);
    connect(d->ui.pluginWidget,
            &KPluginWidget::pluginEnabledChanged,
            this,
            [this](const QString& pluginId, bool enabled) {
                auto plugin = d->pluginManager->getPluginById(pluginId.toStdString());
                if (!plugin) {
                    return;
                }
                plugin->get().setEnabled(enabled);
            });
#endif
}

void ConfigurationDialog::loadCategoryFilteringSettings()
{
    static constexpr int s_debugColumn = 0;
    static constexpr int s_infoColumn = 1;
    static constexpr int s_warningColumn = 2;
    static constexpr int s_criticalColumn = 3;
    static constexpr int s_categoryNameColumn = 4;

    static constexpr int s_dynamicRowsStart = 2;

    const QHash<int, QStringList *> columnsAndCategories = {{s_debugColumn, &d->enabledDebugCategoryNames},
                                                            {s_infoColumn, &d->enabledInfoCategoryNames},
                                                            {s_warningColumn, &d->enabledWarningCategoryNames},
                                                            {s_criticalColumn, &d->enabledCriticalCategoryNames}};

    const auto savedCategories = Codethink::lvtshr::CategoryManager::instance().getCategories();
    d->ui.gridLayoutLogFilters->setAlignment(Qt::AlignVCenter);

    for (int i = 0; i < savedCategories.size(); i++) {
        const auto cat = savedCategories.at(i);
        const auto categoryNameStr = QString::fromStdString(cat->categoryName());
        const auto categoryNameColumnLabel = new QLabel(categoryNameStr);
        d->ui.gridLayoutLogFilters->addWidget(categoryNameColumnLabel, s_dynamicRowsStart + i, s_categoryNameColumn);

        for (int j = 0; j < columnsAndCategories.size(); j++) {
            auto usedEnabledList = columnsAndCategories.value(j);
            auto checkBox = new QCheckBox();
            checkBox->setChecked(usedEnabledList->contains(categoryNameStr));

            connect(checkBox, &QCheckBox::toggled, this, [j, usedEnabledList, checkBox, categoryNameStr, this] {
                if (checkBox->isChecked()) {
                    *usedEnabledList << categoryNameStr;
                } else {
                    usedEnabledList->removeAll(categoryNameStr);
                }
                switch (j) {
                case 0: // Debug column
                    Preferences::setEnabledDebugCategories(*usedEnabledList);
                    break;
                case 1: // Info column
                    Preferences::setEnabledInfoCategories(*usedEnabledList);
                    break;
                case 2: // Warning column
                    Preferences::setEnabledWarningCategories(*usedEnabledList);
                    break;
                case 3: // Critical column
                    Preferences::setEnabledCriticalCategories(*usedEnabledList);
                    break;

                default:
                    break;
                }
            });
            d->ui.gridLayoutLogFilters->addWidget(checkBox, s_dynamicRowsStart + i, j);
        }

        const int savedCategoriesSize = savedCategories.size();
        const QHash<int, QCheckBox *> columnsAndAllCheckboxes = {{s_debugColumn, d->ui.allDebugsCheckbox},
                                                                 {s_infoColumn, d->ui.allInfosCheckbox},
                                                                 {s_warningColumn, d->ui.allWarningsCheckbox},
                                                                 {s_criticalColumn, d->ui.allCriticalCheckbox}};

        for (int j = 0; j < columnsAndAllCheckboxes.size(); j++) {
            connect(columnsAndAllCheckboxes.value(j),
                    &QCheckBox::toggled,
                    this,
                    [this, j, columnsAndAllCheckboxes, savedCategoriesSize] {
                        auto usedCheckbox = columnsAndAllCheckboxes.value(j);
                        switch (j) {
                        case 0: // Debug column
                            Preferences::setDebugGroupEnabled(usedCheckbox->isChecked());
                            break;
                        case 1: // Info column
                            Preferences::setInfoGroupEnabled(usedCheckbox->isChecked());
                            break;
                        case 2: // Warning column
                            Preferences::setWarningGroupEnabled(usedCheckbox->isChecked());
                            break;
                        case 3: // Critical column
                            Preferences::setCriticalGroupEnabled(usedCheckbox->isChecked());
                            break;

                        default:
                            break;
                        }

                        for (int i = 0; i < savedCategoriesSize; i++) {
                            auto *checkBox{qobject_cast<QCheckBox *>(
                                d->ui.gridLayoutLogFilters->itemAtPosition(i + s_dynamicRowsStart, j)->widget())};
                            if (checkBox != nullptr) {
                                checkBox->setChecked(usedCheckbox->isChecked());
                                checkBox->setEnabled(!usedCheckbox->isChecked());
                            }
                        }
                    });
        }
    }
    d->ui.allDebugsCheckbox->setChecked(Preferences::debugGroupEnabled());
    d->ui.allInfosCheckbox->setChecked(Preferences::infoGroupEnabled());
    d->ui.allWarningsCheckbox->setChecked(Preferences::warningGroupEnabled());
    d->ui.allCriticalCheckbox->setChecked(Preferences::criticalGroupEnabled());
    const int stretchRow = d->ui.gridLayoutLogFilters->rowCount() + 1;
    d->ui.gridLayoutLogFilters->setRowStretch(stretchRow, 1);
}
void ConfigurationDialog::load()
{
    d->enabledDebugCategoryNames = Preferences::enabledDebugCategories();
    d->enabledInfoCategoryNames = Preferences::enabledInfoCategories();
    d->enabledWarningCategoryNames = Preferences::enabledWarningCategories();
    d->enabledCriticalCategoryNames = Preferences::enabledCriticalCategories();
    loadCategoryFilteringSettings();
    d->ui.debugContextMenu->setChecked(Preferences::enableSceneContextMenu());
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
    d->ui.comboMultiSelectModifier->setCurrentText(
        ModifierHelpers::modifierToText(static_cast<Qt::KeyboardModifier>(Preferences::multiSelectModifier())));

    d->ui.backgroundColor->setColor(Preferences::backgroundColor());
    d->ui.entityBackgroundColor->setColor(Preferences::entityBackgroundColor());
    d->ui.selectedEntityBackgroundColor->setColor(Preferences::selectedEntityBackgroundColor());
    d->ui.edgeColor->setColor(Preferences::edgeColor());
    d->ui.highlightEdgeColor->setColor(Preferences::highlightEdgeColor());
    d->ui.entityHoverColor->setColor(Preferences::entityHoverColor());

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
    d->ui.invertHorizontalLvlLayout->setChecked(Preferences::invertHorizontalLevelizationLayout());
    d->ui.invertVerticalLvlLayout->setChecked(Preferences::invertVerticalLevelizationLayout());
    d->ui.spaceBetweenLevels->setValue(Preferences::spaceBetweenLevels());
    d->ui.spaceBetweenSublevels->setValue(Preferences::spaceBetweenSublevels());
    d->ui.spaceBetweenEntities->setValue(Preferences::spaceBetweenEntities());
    d->ui.maxEntitiesPerLevel->setValue(Preferences::maxEntitiesPerLevel());
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
        d->ui.comboMultiSelectModifier,
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

#if QT_VERSION > QT_VERSION_CHECK(6, 0, 0)
void ConfigurationDialog::getNewScriptFinished(const QList<KNSCore::Entry>& changedEntries)
#else
void ConfigurationDialog::getNewScriptFinished(const KNSCore::EntryInternal::List& changedEntries)
#endif
{
// Error build on Qt5 - the definitions of
// KNSCore changed and things got messy.
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#undef KNSCore
#define KNSCore KNS3
#endif

    bool installed = false;
    bool removed = false;
    // Our plugins are gzipped, and the installedFiles returns with `/*` in the end to denote
    // multiple files. we need to chop that off when we want to install it.
    QList<QString> installedPlugins;
    QList<QString> removedPlugins;
    for (const auto& entry : qAsConst(changedEntries)) {
        switch (entry.status()) {
        case KNSCore::Entry::Installed: {
            if (entry.installedFiles().count() == 0) {
                continue;
            }

            QString filename = entry.installedFiles()[0];
            // remove /* from the string.
            installedPlugins.append(filename.chopped(2));

            installed = true;
        } break;
        case KNSCore::Entry::Deleted: {
            if (entry.uninstalledFiles().count() == 0) {
                continue;
            }

            QString filename = entry.uninstalledFiles()[0];
            // remove /* from the string.
            removedPlugins.append(filename.chopped(2));

            removed = true;
        }
        case KNSCore::Entry::Invalid:
        case KNSCore::Entry::Installing:
        case KNSCore::Entry::Downloadable:
        case KNSCore::Entry::Updateable:
        case KNSCore::Entry::Updating:
            // Not interesting.
            break;
        }
    }

    // Refresh the plugins installed by GetNewStuff
    if (installed) {
        for (const auto& installedPlugin : installedPlugins) {
            d->pluginManager->reloadPlugin(installedPlugin);
        }
    }
    if (removed) {
        for (const auto& uninstalledFile : removedPlugins) {
            d->pluginManager->removePlugin(uninstalledFile);
        }
    }

    updatePluginInformation();
}
} // namespace Codethink::lvtqtw
