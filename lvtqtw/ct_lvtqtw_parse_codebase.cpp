// ct_lvtqtd_parse_codebase.cpp                               -*-C++-*-

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
#include "ct_lvtclp_clputil.h"
#include <ct_lvtqtw_parse_codebase.h>

#include <ct_lvtclp_clputil.h>
#include <ct_lvtclp_cpp_tool.h>
#include <ct_lvtclp_parse_error_handler.h>

#include <ct_lvtmdb_functionobject.h>
#include <ct_lvtmdb_soci_helper.h>
#include <ct_lvtmdb_soci_reader.h>
#include <ct_lvtmdb_soci_writer.h>
#include <ct_lvtprj_projectfile.h>
#include <ct_lvtqtw_textview.h>
#include <ct_lvtshr_iterator.h>
#ifdef CT_ENABLE_FORTRAN_SCANNER
#include <fortran/ct_lvtclp_fortran_c_interop.h>
#include <fortran/ct_lvtclp_fortran_tool.h>
#endif

#include <ui_ct_lvtqtw_parse_codebase.h>

#include <KZip>

#include <QDir>
#include <QElapsedTimer>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QMessageBox>
#include <QMovie>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>
#include <QSysInfo>
#include <QTabBar>
#include <QTableWidget>
#include <QThread>
#include <QVariant>

#include <thirdparty/result/result.hpp>

#include <KNotification>

#include <clang/Tooling/JSONCompilationDatabase.h>
#include <preferences.h>
#include <soci/soci.h>

using namespace Codethink::lvtqtw;

namespace {
constexpr const char *COMPILE_COMMANDS = "compile_commands.json";
constexpr const char *NON_LAKOSIAN_DIRS_SETTING = "non_lakosian_dirs";

bool is_wsl_string(const QString& text)
{
    return text.startsWith("wsl://");
}

} // namespace

struct PkgMappingDialog : public QDialog {
  public:
    PkgMappingDialog()
    {
        setupUi();

        connect(m_addLineBtn, &QPushButton::clicked, this, &PkgMappingDialog::addTableWdgLine);
        connect(m_okBtn, &QPushButton::clicked, this, &PkgMappingDialog::acceptChanges);
        connect(m_cancelBtn, &QPushButton::clicked, this, &PkgMappingDialog::cancelChanges);
    }

    PkgMappingDialog(PkgMappingDialog const&) = delete;

    void populateTable(std::vector<std::pair<std::string, std::string>> const& thirdPartyPathMapping)
    {
        using Codethink::lvtshr::enumerate;

        for (auto&& [i, mapping] : enumerate(thirdPartyPathMapping)) {
            auto&& [k, v] = mapping;
            m_tableWdg->insertRow(static_cast<int>(i));

            auto *pathItem = new QTableWidgetItem();
            pathItem->setText(QString::fromStdString(k));
            m_tableWdg->setItem(static_cast<int>(i), 0, pathItem);

            auto *pkgNameItem = new QTableWidgetItem();
            pkgNameItem->setText(QString::fromStdString(v));
            m_tableWdg->setItem(static_cast<int>(i), 1, pkgNameItem);
        }
    }

    [[nodiscard]] bool changesAccepted() const
    {
        return m_acceptChanges;
    }

    [[nodiscard]] std::vector<std::pair<std::string, std::string>> pathMapping()
    {
        // Remove unexpected/unwanted characters in a given table item text
        auto filterText = [](QString&& txt) -> std::string {
            txt.replace(",", "");
            txt.replace("=", "");
            return txt.toStdString();
        };

        std::vector<std::pair<std::string, std::string>> pathMapping;
        for (auto i = 0; i < m_tableWdg->rowCount(); ++i) {
            auto pathText = filterText(m_tableWdg->item(i, 0)->text());
            auto pkgText = filterText(m_tableWdg->item(i, 1)->text());
            if (pathText.empty() || pkgText.empty()) {
                continue;
            }
            pathMapping.emplace_back(pathText, pkgText);
        }
        return pathMapping;
    }

  private:
    void addTableWdgLine()
    {
        int row = m_tableWdg->rowCount();
        m_tableWdg->insertRow(row);
        m_tableWdg->setItem(row, 0, new QTableWidgetItem());
        m_tableWdg->setItem(row, 1, new QTableWidgetItem());
    }

    void acceptChanges()
    {
        m_acceptChanges = true;
        close();
    }

    void cancelChanges()
    {
        m_acceptChanges = false;
        close();
    }

    void setupUi()
    {
        setWindowModality(Qt::ApplicationModal);
        setWindowTitle("Third party packages mapping");
        auto *layout = new QVBoxLayout{this};
        m_tableWdg = new QTableWidget{this};
        m_tableWdg->setColumnCount(2);
        m_tableWdg->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        m_tableWdg->setHorizontalHeaderLabels({"Path", "Package name"});
        layout->addWidget(m_tableWdg);
        m_addLineBtn = new QPushButton("+");
        layout->addWidget(m_addLineBtn);
        auto *okCancelBtnWdg = new QWidget{this};
        auto *okCancelBtnLayout = new QHBoxLayout{okCancelBtnWdg};
        auto *okCancelSpacer = new QSpacerItem{0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed};
        m_okBtn = new QPushButton("Ok");
        m_cancelBtn = new QPushButton("Cancel");
        okCancelBtnLayout->addItem(okCancelSpacer);
        okCancelBtnLayout->addWidget(m_okBtn);
        okCancelBtnLayout->addWidget(m_cancelBtn);
        layout->addWidget(okCancelBtnWdg);
        setLayout(layout);
    }

    QTableWidget *m_tableWdg = nullptr;
    QPushButton *m_okBtn = nullptr;
    QPushButton *m_cancelBtn = nullptr;
    QPushButton *m_addLineBtn = nullptr;
    bool m_acceptChanges = false;
};

struct ParseCodebaseDialog::Private {
    State dialogState = State::Idle;
    std::shared_ptr<lvtmdb::ObjectStore> sharedMemDb = nullptr;
    std::unique_ptr<lvtclp::CppTool> tool_p = nullptr;
#ifdef CT_ENABLE_FORTRAN_SCANNER
    std::unique_ptr<lvtclp::fortran::Tool> fortran_tool_p = nullptr;
#endif
    QThread *parseThread = nullptr;
    bool threadSuccess = false;
    int progress = 0;

    std::map<long, TextView *> threadIdToWidget;
    QString codebasePath;

    using ThirdPartyPath = std::string;
    using ThirdPartyPackageName = std::string;
    std::vector<std::pair<ThirdPartyPath, ThirdPartyPackageName>> thirdPartyPathMapping;
    std::vector<std::string> userProvidedExtraCompileCommandsArgs;

    std::optional<std::reference_wrapper<Codethink::lvtplg::PluginManager>> pluginManager = std::nullopt;
    QElapsedTimer parseTimer;
    lvtclp::ParseErrorHandler parseErrorHandler;
};

ParseCodebaseDialog::ParseCodebaseDialog(QWidget *parent):
    QDialog(parent),
    d(std::make_unique<ParseCodebaseDialog::Private>()),
    ui(std::make_unique<Ui::ParseCodebaseDialog>())
{
    d->sharedMemDb = std::make_shared<lvtmdb::ObjectStore>();
    ui->setupUi(this);

    // TODO: Remove those things / Fix them when we finish the presentation.
    ui->runCmake->setVisible(false);
    ui->runCmake->setChecked(false);
    ui->refreshDb->setVisible(false);
    ui->updateDb->setVisible(false);
    ui->updateDb->setChecked(true);

    ui->ignorePattern->setText(Preferences::lastIgnorePattern());
    ui->compileCommandsFolder->setText(Preferences::lastConfigureJson());
    ui->sourceFolder->setText(Preferences::lastSourceFolder());
    ui->showDbErrors->setVisible(false);

    ui->nonLakosians->setText(getNonLakosianDirSettings(Preferences::lastConfigureJson()));

    connect(this, &ParseCodebaseDialog::parseFinished, this, [this] {
        ui->btnSaveOutput->setEnabled(true);
        ui->btnClose->setEnabled(true);
    });

    connect(ui->threadCount, QOverload<int>::of(&QSpinBox::valueChanged), this, [this] {
        Preferences::setThreadCount(ui->threadCount->value());
    });

    ui->threadCount->setValue(Preferences::threadCount());
    ui->threadCount->setMaximum(QThread::idealThreadCount() + 1);

    //    connect(ui->btnSaveOutput, &QPushButton::clicked, this, &ParseCodebaseDialog::saveOutput);

    connect(ui->searchCompileCommands, &QPushButton::clicked, this, &ParseCodebaseDialog::searchForCompileCommands);
    connect(ui->nonLakosiansSearch, &QPushButton::clicked, this, &ParseCodebaseDialog::searchForNonLakosianDir);
    connect(ui->buildFolderSearch, &QPushButton::clicked, this, &ParseCodebaseDialog::searchForBuildFolder);
    connect(ui->sourceFolderSearch, &QPushButton::clicked, this, &ParseCodebaseDialog::searchForSourceFolder);
    connect(ui->thirdPartyPkgMappingBtn, &QPushButton::clicked, this, &ParseCodebaseDialog::selectThirdPartyPkgMapping);

    connect(ui->ignorePattern, &QLineEdit::textChanged, this, [this] {
        Preferences::setLastIgnorePattern(ui->ignorePattern->text());
    });

    connect(ui->compileCommandsFolder, &QLineEdit::textChanged, this, [this] {
        Preferences::setLastConfigureJson(ui->compileCommandsFolder->text());

        ui->nonLakosians->setText(getNonLakosianDirSettings(ui->compileCommandsFolder->text()));
    });

    connect(ui->sourceFolder, &QLineEdit::textChanged, this, [this] {
        Preferences::setLastSourceFolder(ui->sourceFolder->text());
    });

    connect(ui->nonLakosians, &QLineEdit::textChanged, this, [this] {
        setNonLakosianDirSettings(ui->compileCommandsFolder->text(), ui->nonLakosians->text());
    });

    connect(ui->btnClose, &QPushButton::clicked, this, [this] {
        // the close button should just hide the dialog. we display the dialog with show()
        // so it does not block the event loop. The only correct time to properly close()
        // the dialog is when the parse process finishes.
        hide();
    });

    connect(ui->btnParse, &QPushButton::clicked, this, [this] {
        if (d->dialogState == State::Idle) {
            initParse();
        }
        if (d->dialogState == State::RunAllLogical) {
            close();
        }
    });

    connect(ui->btnCancelParse, &QPushButton::clicked, this, [this] {
        if (d->parseThread) {
            d->dialogState = State::Killed;
            ui->btnCancelParse->setEnabled(false);
            ui->progressBarText->setText(tr("Cancelling parse threads, this might take a few seconds."));
            if (d->tool_p) {
                d->tool_p->cancelRun();
            }
            // endParseStage will emit parseFinished
        } else {
            Q_EMIT parseFinished(State::Idle);
        }
    });

    ui->progressBar->setMinimum(0);

    connect(ui->compileCommandsFolder, &QLineEdit::textChanged, this, [this] {
        validateUserInputFolders();
    });

    connect(ui->sourceFolder, &QLineEdit::textChanged, this, [this] {
        validateUserInputFolders();
    });

    connect(ui->btnSaveOutput, &QPushButton::clicked, this, [this] {
        if (!d->parseErrorHandler.hasErrors()) {
            QMessageBox::warning(this, tr("Nothing to save"), tr("There are no errors to save"));
            return;
        }

        const QString outputPath = QFileDialog::getSaveFileName();
        if (outputPath.isEmpty()) {
            return;
        }

        Codethink::lvtclp::ParseErrorHandler::SaveOutputInputArgs args{
            .compileCommands = ui->compileCommandsFolder->text().toStdString(),
            .outputPath = outputPath.toStdString(),
            .ignorePattern = ui->ignorePattern->text()};

        auto res = d->parseErrorHandler.saveOutput(args);
        if (!res) {
            ui->errorText->setText(tr("File saved but some errors occoured:\n%1").arg(res.error()));
        } else {
            ui->errorText->setText(tr("File %1 saved successfully").arg(outputPath));
        }
    });

    ui->projectCompileCommandsError->setVisible(false);
    ui->projectSourceFolderError->setVisible(false);

    QFile markdownFile(":/md/codebase_gen_doc");
    markdownFile.open(QIODevice::ReadOnly);
    const QString data = markdownFile.readAll();

// Qt on Appimage is 5.13 aparently.
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    ui->textBrowser->setText(data);
#else
    ui->textBrowser->setMarkdown(data);
#endif

    Qt::WindowFlags flags;
    flags =
        windowFlags() & ~(Qt::WindowCloseButtonHint | Qt::WindowMinMaxButtonsHint | Qt::WindowContextHelpButtonHint);
    setWindowFlags(flags);

    validateUserInputFolders();
}

ParseCodebaseDialog::SourceFolderValidationResult ParseCodebaseDialog::validateSourceFolder(const QString& sourceFolder)
{
    if (sourceFolder.isEmpty()) {
        return SourceFolderValidationResult::NoSourceFolderProvided;
    }

    if (is_wsl_string(sourceFolder)) {
        return SourceFolderValidationResult::WslSourceFolder;
    }

    return SourceFolderValidationResult::SourceFolderOk;
}

ParseCodebaseDialog::CompileCommandsValidationResult
ParseCodebaseDialog::validateCompileCommandsFolder(const QString& compileCommands)
{
    if (compileCommands.isEmpty()) {
        return CompileCommandsValidationResult::NoBuildFolderProvided;
    }

    if (is_wsl_string(compileCommands)) {
        return CompileCommandsValidationResult::WslBuildFolder;
    }

    QFileInfo info(compileCommands);
    if (info.isDir()) {
        return CompileCommandsValidationResult::CompileCommandsJsonNotFound;
    }

    if (!info.exists()) {
        return CompileCommandsValidationResult::CompileCommandsJsonNotFound;
    }

    return CompileCommandsValidationResult::BuildFolderOk;
}

void ParseCodebaseDialog::validateUserInputFolders()
{
    const auto emptyErrorMsg = tr("This field can't be empty");
    const auto wslErrorMsg = tr("The software does not support wsl, use the native linux build.");
    const auto errorCss = QString("border: 1px solid red");
    const auto missingCompileCommands = tr("The specified folder does not contain compile_commands.json");
    bool disableParse = false;

    const auto build_folder_validation = validateCompileCommandsFolder(ui->compileCommandsFolder->text());

    if (build_folder_validation == CompileCommandsValidationResult::NoBuildFolderProvided) {
        ui->projectCompileCommandsError->setVisible(true);
        ui->projectCompileCommandsError->setText(emptyErrorMsg);
        ui->compileCommandsFolder->setStyleSheet(errorCss);
        disableParse = true;
    }

    if (build_folder_validation == CompileCommandsValidationResult::WslBuildFolder) {
        ui->projectCompileCommandsError->setVisible(true);
        ui->projectCompileCommandsError->setText(wslErrorMsg);
        ui->compileCommandsFolder->setStyleSheet(errorCss);
        disableParse = true;
    }

    if (build_folder_validation == CompileCommandsValidationResult::CompileCommandsJsonNotFound) {
        ui->projectCompileCommandsError->setVisible(true);
        ui->projectCompileCommandsError->setText(missingCompileCommands);
        ui->compileCommandsFolder->setStyleSheet(errorCss);
        disableParse = true;
    }

    if (build_folder_validation == CompileCommandsValidationResult::BuildFolderOk) {
        ui->projectCompileCommandsError->setText("");
        ui->projectCompileCommandsError->setVisible(false);
        ui->compileCommandsFolder->setStyleSheet(QString());
    }

    const auto source_folder_validation = validateSourceFolder(ui->sourceFolder->text());

    if (source_folder_validation == SourceFolderValidationResult::NoSourceFolderProvided) {
        ui->projectSourceFolderError->setVisible(true);
        ui->projectSourceFolderError->setText(emptyErrorMsg);
        ui->sourceFolder->setStyleSheet(errorCss);
        disableParse = true;
    }

    if (source_folder_validation == SourceFolderValidationResult::WslSourceFolder) {
        ui->projectSourceFolderError->setVisible(true);
        ui->projectSourceFolderError->setText(wslErrorMsg);
        ui->sourceFolder->setStyleSheet(errorCss);
        disableParse = true;
    }

    if (source_folder_validation == SourceFolderValidationResult::SourceFolderOk) {
        ui->projectSourceFolderError->setVisible(false);
        ui->projectSourceFolderError->setText("");
        ui->sourceFolder->setStyleSheet(QString());
    }

    ui->btnParse->setDisabled(disableParse);
}

ParseCodebaseDialog::~ParseCodebaseDialog()
{
    Preferences::self()->save();
}

QString ParseCodebaseDialog::getNonLakosianDirSettings(const QString& buildDir)
{
    QSettings settings;

    // QMap<QString, QString>: buildDir -> nonLakosianDirSettings
    QMap<QString, QVariant> nonLakosianDirMap = settings.value(NON_LAKOSIAN_DIRS_SETTING).toMap();

    // if it is not in the map or if the variant is not a string, we return ""
    return nonLakosianDirMap.value(buildDir).toString();
}

void ParseCodebaseDialog::setNonLakosianDirSettings(const QString& buildDir, const QString& nonLakosianDirs)
{
    QSettings settings;

    // QMap<QString, QString>: buildDir -> nonLakosianDirSettings
    QMap<QString, QVariant> nonLakosianDirMap = settings.value(NON_LAKOSIAN_DIRS_SETTING).toMap();

    nonLakosianDirMap.insert(buildDir, QVariant(nonLakosianDirs));

    settings.setValue(NON_LAKOSIAN_DIRS_SETTING, QVariant(nonLakosianDirMap));
}

void ParseCodebaseDialog::setCodebasePath(const QString& path)
{
    d->codebasePath = path;
}

QString ParseCodebaseDialog::codebasePath() const
{
    // conversion dance. Qt has no conversion from std::string_view. :|
    const auto dbFilename = std::string(lvtprj::ProjectFile::databaseFilename());
    const auto qDbFilename = QString::fromStdString(dbFilename);
    return d->codebasePath + QDir::separator() + qDbFilename;
}

void ParseCodebaseDialog::searchForBuildFolder()
{
    auto openDir = [&]() {
        auto lastDir = QDir{ui->buildFolder->text()};
        if (!lastDir.isEmpty() && lastDir.exists()) {
            return lastDir.canonicalPath();
        }
        return QDir::homePath();
    }();

    const QString buildDirectory = QFileDialog::getExistingDirectory(this, tr("Project Build Directory"), openDir);

    if (buildDirectory.isEmpty()) {
        return;
    }

    ui->buildFolder->setText(buildDirectory);
}

void ParseCodebaseDialog::searchForSourceFolder()
{
    auto openDir = [&]() {
        auto lastDir = QDir{ui->sourceFolder->text()};
        if (!lastDir.isEmpty() && lastDir.exists()) {
            return lastDir.canonicalPath();
        }
        return QDir::homePath();
    }();

    const QString dir = QFileDialog::getExistingDirectory(this, tr("Project Source Directory"), openDir);
    if (dir.isEmpty()) {
        // User hits cancel
        return;
    }
    ui->sourceFolder->setText(dir);
}

void ParseCodebaseDialog::searchForCompileCommands()
{
    auto openDir = [&]() {
        auto lastDir = QDir{ui->compileCommandsFolder->text()};
        if (!lastDir.isEmpty() && lastDir.exists()) {
            return lastDir.canonicalPath();
        }
        return QDir::homePath();
    }();

    const QString buildDirectory =
        QFileDialog::getOpenFileName(this, tr("Compile Commands"), openDir, "compile_commands.json");

    if (buildDirectory.isEmpty()) {
        return;
    }

    ui->compileCommandsFolder->setText(buildDirectory);
}

void ParseCodebaseDialog::searchForNonLakosianDir()
{
    QString compileCommandsFolder = ui->compileCommandsFolder->text();
    if (compileCommandsFolder.isEmpty()) {
        compileCommandsFolder = QDir::homePath();
    }

    const QString nonLakosianDir =
        QFileDialog::getExistingDirectory(this, tr("Non-lakosian directory"), compileCommandsFolder);
    QFileInfo dir(nonLakosianDir);
    if (!dir.exists()) {
        return;
    }

    if (ui->nonLakosians->text().isEmpty()) {
        ui->nonLakosians->setText(nonLakosianDir);
    } else {
        ui->nonLakosians->setText(ui->nonLakosians->text() + "," + nonLakosianDir);
    }
}

void ParseCodebaseDialog::selectThirdPartyPkgMapping()
{
    auto pkgMappingWindow = PkgMappingDialog{};
    pkgMappingWindow.populateTable(d->thirdPartyPathMapping);
    pkgMappingWindow.show();
    pkgMappingWindow.exec();

    if (pkgMappingWindow.changesAccepted()) {
        d->thirdPartyPathMapping = pkgMappingWindow.pathMapping();

        auto newText = QString{};
        for (auto&& [k, v] : d->thirdPartyPathMapping) {
            newText += QString::fromStdString(k) + "=" + QString::fromStdString(v) + ",";
        }
        newText.chop(1);
        ui->thirdPartyPkgMapping->setText(newText);
    }
}

void ParseCodebaseDialog::showEvent(QShowEvent *event)
{
    if (d->dialogState != State::RunAllLogical) {
        // if the logical parse is currently running in the background
        // we should leave the window as it is so that it can be used to view
        // the progress
        reset();
    }
    QDialog::showEvent(event);
}

void ParseCodebaseDialog::reset()
{
    d->dialogState = State::Idle;
    ui->btnClose->setEnabled(true);
    ui->btnCancelParse->setEnabled(false);
    ui->errorText->setText(QString());
    ui->errorText->setVisible(false);
    ui->progressBar->setValue(0);
    ui->progressBarText->setVisible(false);

    validateUserInputFolders();
}

void ParseCodebaseDialog::initParse()
{
    // initParse() is called twice, once for the Physical, and again for the Logical parses.
    assert(d->dialogState == State::Idle || d->dialogState == State::RunAllPhysical);

    // re-enable cancel button if it was disabled (e.g. because it was used on
    // the last run)
    ui->btnCancelParse->setEnabled(true);

    if (ui->refreshDb->isChecked()) {
        if (QFileInfo::exists(codebasePath()) && d->dialogState == State::Idle) {
            QFile dbFile(codebasePath());
            const bool removed = dbFile.remove();
            if (!removed) {
                ui->errorText->setText(
                    tr("Error removing the database file, check if you have permissions to do that"));
                ui->errorText->setVisible(true);
                ui->btnClose->setEnabled(true);
                ui->btnSaveOutput->setEnabled(true);
                ui->progressBarText->setVisible(false);
                return;
            }
        }
    }

    if (ui->physicalOnly->checkState() != Qt::Unchecked && d->dialogState == State::Idle) {
        d->dialogState = State::RunPhysicalOnly;
    } else {
        if (d->dialogState == State::Idle) {
            d->dialogState = State::RunAllPhysical;
        } else if (d->dialogState == State::RunAllPhysical) {
            d->dialogState = State::RunAllLogical;
        }
    }

    d->parseTimer.restart();
    const auto compileCommandsJson = ui->compileCommandsFolder->text();
    const auto compileCommandsExists = QFileInfo::exists(compileCommandsJson);
    const auto physicalRun = d->dialogState == State::RunPhysicalOnly || d->dialogState == State::RunAllPhysical;
    const auto mustGenerateCompileCommands = physicalRun && (!compileCommandsExists || ui->runCmake->checkState());
    const auto ignoreList = ignoredItemsAsStdVec();
    const auto nonLakosianDirs = nonLakosianDirsAsStdVec();
    if (mustGenerateCompileCommands) {
        runCMakeAndStartParse(compileCommandsJson.toStdString(), ignoreList, nonLakosianDirs);
    } else {
        prepareParse(compileCommandsJson.toStdString(), ignoreList, nonLakosianDirs);
    }
}

void ParseCodebaseDialog::runCMakeAndStartParse(const std::string& compileCommandsJson,
                                                const std::vector<std::string>& ignoreList,
                                                const std::vector<std::filesystem::path>& nonLakosianDirs)
{
    const QString cmakeExecutable = QStandardPaths::findExecutable("cmake");
    if (cmakeExecutable.isEmpty()) {
        ui->errorText->setText(tr("CMake executable not found, please install it and add to the PATH"));
        ui->btnParse->setEnabled(true);
        ui->btnCancelParse->setEnabled(false);
        return;
    }

    // Force a refresh of the `compile_commands.json` file.
    auto *refreshCompileCommands = new QProcess();
    auto onFinishCMakeRun =
        [this, compileCommandsJson, ignoreList, nonLakosianDirs, refreshCompileCommands](int exitCode,
                                                                                         QProcess::ExitStatus) {
            if (exitCode != 0) {
                const auto errorStr = QString(refreshCompileCommands->readAllStandardOutput());
                ui->errorText->setText(tr("Error generating the compile_commands.json file\n%1").arg(errorStr));
                ui->errorText->show();
                ui->btnParse->setEnabled(true);
                ui->btnCancelParse->setEnabled(false);
                return;
            }
            sender()->deleteLater();

            prepareParse(compileCommandsJson, ignoreList, nonLakosianDirs);
        };
    connect(refreshCompileCommands,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this,
            onFinishCMakeRun);

    ui->errorText->setText(tr("Generating compile_commands.json, this might take a few minutes."));
    ui->errorText->show();
    ui->btnParse->setEnabled(false);
    refreshCompileCommands->setWorkingDirectory(ui->compileCommandsFolder->text());
    refreshCompileCommands->start(cmakeExecutable, QStringList({".", "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"}));
}

void ParseCodebaseDialog::initTools(const std::string& compileCommandsJson,
                                    const std::vector<std::string>& ignoreList,
                                    const std::vector<std::filesystem::path>& nonLakosianDirs)
{
    const bool catchCodeAnalysisOutput = Preferences::enableCodeParseDebugOutput();
    if (!d->tool_p) {
        auto disableLakosianRules = (ui->disableLakosianRules->checkState() == Qt::Checked);

        const CppToolConstants constants{
            .prefix = sourcePath(),
            .buildPath = buildPath(),
            .databasePath = codebasePath().toStdString(),
            .nonLakosianDirs = Codethink::lvtclp::ClpUtil::ensureCanonical(nonLakosianDirs),
            .thirdPartyDirs = d->thirdPartyPathMapping,
            .ignoreGlobs = Codethink::lvtclp::ClpUtil::stringListToGlobPattern(ignoreList),
            .userProvidedExtraCompileCommandsArgs = d->userProvidedExtraCompileCommandsArgs,
            .numThreads = static_cast<uint>(ui->threadCount->value()),
            .enableLakosianRules = !disableLakosianRules,
            .printToConsole = catchCodeAnalysisOutput};

        d->tool_p =
            std::make_unique<lvtclp::CppTool>(constants, std::vector<std::filesystem::path>{compileCommandsJson});

        d->parseErrorHandler.setTool(d->tool_p.get());
    }
    d->tool_p->setSharedMemDb(d->sharedMemDb);
    d->tool_p->setShowDatabaseErrors(ui->showDbErrors->isChecked());

#ifdef CT_ENABLE_FORTRAN_SCANNER
    if (!d->fortran_tool_p) {
        d->fortran_tool_p = lvtclp::fortran::Tool::fromCompileCommands(compileCommandsJson);
    }
    d->fortran_tool_p->setSharedMemDb(d->sharedMemDb);
#endif

    connect(d->tool_p.get(),
            &lvtclp::CppTool::processingFileNotification,
            this,
            &ParseCodebaseDialog::processingFileNotification,
            Qt::QueuedConnection);

    connect(d->tool_p.get(),
            &lvtclp::CppTool::aboutToCallClangNotification,
            this,
            &ParseCodebaseDialog::aboutToCallClangNotification,
            Qt::QueuedConnection);
}

QThread *ParseCodebaseDialog::createThreadFnToRunParseTools()
{
#ifdef CT_ENABLE_FORTRAN_SCANNER
    auto threadFn = [this]() {
        assert(d->tool_p);
        assert(d->fortran_tool_p);
        if (d->dialogState == State::RunPhysicalOnly || d->dialogState == State::RunAllPhysical) {
            d->threadSuccess = d->tool_p->runPhysical();
            d->threadSuccess = d->fortran_tool_p->runPhysical();
        } else if (d->dialogState == State::RunAllLogical) {
            d->threadSuccess = d->tool_p->runFull(/*skipPhysical=*/true);
            d->threadSuccess = d->fortran_tool_p->runFull(/*skipPhysical=*/true);
            Codethink::lvtclp::fortran::solveFortranToCInteropDeps(*d->sharedMemDb);
        }
    };
#else
    auto threadFn = [this]() {
        assert(d->tool_p);
        if (d->dialogState == State::RunPhysicalOnly || d->dialogState == State::RunAllPhysical) {
            d->threadSuccess = d->tool_p->runPhysical();
        } else if (d->dialogState == State::RunAllLogical) {
            d->threadSuccess = d->tool_p->runFull(/*skipPhysical=*/true);
        }
    };
#endif
    return QThread::create(threadFn);
}

void ParseCodebaseDialog::prepareParse(const std::string& compileCommandsJson,
                                       const std::vector<std::string>& ignoreList,
                                       const std::vector<std::filesystem::path>& nonLakosianDirs)
{
    // for now a Cpp Tool and a fortran tool
    initTools(compileCommandsJson, ignoreList, nonLakosianDirs);

    ui->progressBar->setValue(0);
    ui->progressBarText->setVisible(true);
    if (d->dialogState == State::RunPhysicalOnly || d->dialogState == State::RunAllPhysical) {
        ui->progressBarText->setText(tr("Initialising physical parse..."));
        ui->errorText->setText(tr("Performing physical parse..."));
        ui->errorText->show();
    } else if (d->dialogState == State::RunAllLogical) {
        ui->progressBarText->setText(tr("Initialising logical parse..."));
        ui->errorText->setText(tr("Performing logical parse..."));
        ui->errorText->show();
    } else {
        assert(false && "Unreachable");
    }
    startParse();
}

void ParseCodebaseDialog::startParse()
{
    // it is okay to close the window after the physical parse is completed and
    // allow the logical parse to continue in the background. Otherwise disable
    // closing while a parse is running.
    ui->btnClose->setEnabled(d->dialogState == State::RunAllLogical);
    d->parseThread = createThreadFnToRunParseTools();
    connect(d->parseThread, &QThread::finished, this, &ParseCodebaseDialog::readyForDbUpdate);
    ui->btnParse->setEnabled(false);
    ui->btnSaveOutput->setEnabled(false);
    Q_EMIT parseStarted(d->dialogState);
    d->parseThread->start();
}

void ParseCodebaseDialog::updateDatabase()
// parseThread finished, we asked for a callback from the main window when
// it was ready to have its database replaced. That callback just happened
// so lets go! Delete the old database. Write the new database.
{
    d->parseThread->deleteLater();
    d->parseThread = nullptr;

    assert(d->tool_p);
    assert(d->dialogState != State::Idle);

    std::string path = codebasePath().toStdString();

    if (std::filesystem::exists(path)) {
        bool success = false;
        try {
            success = std::filesystem::remove(path);
        } catch (const std::exception& e) {
            std::cerr << __func__ << ": exception during delete: " << e.what() << std::endl;
            success = false;
        }

        if (!success) {
            qWarning() << "Failed to delete database at" << codebasePath();
            ui->errorText->setText(tr("Failed to delete old database"));
            ui->errorText->show();
            d->dialogState = State::Idle;
            Q_EMIT parseFinished(State::Idle);
            return;
        }
    }

    {
        lvtmdb::SociWriter writer;
        if (!writer.createOrOpen(path)) {
            ui->errorText->setText(tr("Failed to create database file"));
            ui->errorText->show();
            Q_EMIT parseFinished(State::Idle);
            return;
        }
        d->sharedMemDb->writeToDatabase(writer);
    }

    if (d->pluginManager) {
        auto& pm = (*d->pluginManager).get();

        d->tool_p->setHeaderLocationCallback(
            [&pm](std::string const& sourceFile, std::string const& includedFile, unsigned lineNo) {
                pm.callHooksPhysicalParserOnHeaderFound(
                    [&sourceFile]() {
                        return sourceFile;
                    },
                    [&includedFile]() {
                        return includedFile;
                    },
                    [&lineNo]() {
                        return lineNo;
                    });
            });

        d->tool_p->setHandleCppCommentsCallback(
            [&pm](const std::string& filename, const std::string& briefText, unsigned startLine, unsigned endLine) {
                pm.callHooksPluginLogicalParserOnCppCommentFoundHandler(
                    [&filename]() {
                        return filename;
                    },
                    [&briefText]() {
                        return briefText;
                    },
                    [&startLine]() {
                        return startLine;
                    },
                    [&endLine]() {
                        return endLine;
                    });
            });
    }

    endParseStage();
}

void ParseCodebaseDialog::cleanupTools()
{
    d->sharedMemDb->withRWLock([&] {
        d->sharedMemDb->clear();
    });
    d->tool_p = nullptr;
#ifdef CT_ENABLE_FORTRAN_SCANNER
    d->fortran_tool_p = nullptr;
#endif
}

void ParseCodebaseDialog::resetUiForNextParse()
{
    ui->btnParse->setEnabled(true);
    ui->progressBarText->setVisible(false);
    ui->progressBar->setValue(0);
}

void ParseCodebaseDialog::displayStopError()
{
    if (d->dialogState == State::Killed) {
        ui->errorText->setText(tr("Parsing operation killed."));
        ui->errorText->show();
        d->dialogState = State::Idle;
        Q_EMIT parseFinished(State::Killed);
        return;
    }

    if (!d->threadSuccess) {
        ui->errorText->setText(tr("Compiler Error while parsing the codebase."));
        ui->errorText->show();
        d->dialogState = State::Idle;
        Q_EMIT parseFinished(State::Idle);
        return;
    }
}

void ParseCodebaseDialog::endParseStage()
{
    assert(d->dialogState != State::Idle);
    resetUiForNextParse();
    auto parsingDidNotFinishSuccessfully = [this]() {
        return d->dialogState == State::Killed || !d->threadSuccess;
    }();
    if (parsingDidNotFinishSuccessfully) {
        displayStopError();
        return;
    }

    if (d->dialogState == State::RunAllPhysical) {
        // move on to RunAllLogical
        // the error text is also used to just notify the user, mb a diffrent name would be ideal
        ui->errorText->setText(tr("Physical parsing done. Continuing with logical parse"));
        ui->errorText->show();
        notifyUserForFinishedStage();
        d->parseTimer.restart();
        initParse();
        return;
    }

    // is there a specific reason the emit is in a different place? also the call is duplicated
    if (d->dialogState == State::RunPhysicalOnly) {
        notifyUserForFinishedStage();
    } else if (d->dialogState == State::RunAllLogical) {
        notifyUserForFinishedStage();
    }
    d->dialogState = State::Idle;
    d->parseTimer.invalidate();

    if (d->pluginManager) {
        soci::session db;
        std::string path = codebasePath().toStdString();
        db.open(*soci::factory_sqlite3(), path);

        auto& pm = (*d->pluginManager).get();
        auto runQueryOnDatabase =
            [&](std::string const& dbQuery) -> std::vector<std::vector<Codethink::lvtplg::RawDBData>> {
            return lvtmdb::SociHelper::runSingleQuery(db, dbQuery);
        };
        pm.callHooksOnParseCompleted(runQueryOnDatabase);
    }
    cleanupTools();

    close();
}

void ParseCodebaseDialog::notifyUserForFinishedStage()
{
// TODO: KNotification hangs on OSX. block this temporarely.
#ifndef __APPLE__
    auto getNotificationStringForState = [](State current) {
        if (current == State::RunPhysicalOnly) {
            return "Physical Parse finished with: %1";
        }

        if (current == State::RunAllLogical) {
            return "Logical Parse finished with: %1";
        }

        if (current == State::RunAllPhysical) {
            return "Physical Parse finished with: %1<br/>Starting Logical Parse.";
        }

        return "This Code path was not meant to be followed, with info: %1";
    };
    QTime time(0, 0);
    time = time.addMSecs(d->parseTimer.elapsed());
    auto *notification = new KNotification("parserFinished");

    const auto notificationText = tr(getNotificationStringForState(d->dialogState)).arg(time.toString("mm:ss.zzz"));
    notification->setText(notificationText);
    notification->sendEvent();
#endif
    Q_EMIT parseFinished(d->dialogState);
}

void ParseCodebaseDialog::processingFileNotification(const QString& path)
{
    QFileInfo info(path);

    if (d->tool_p && d->tool_p->finalizingThreads()) {
        return;
    }

    ui->progressBar->setValue(++d->progress);
    ui->progressBarText->setText(info.baseName());
    Q_EMIT parseStep(d->dialogState, ui->progressBar->value(), ui->progressBar->maximum());
}

void ParseCodebaseDialog::aboutToCallClangNotification(const QString& progressBarText, int size)
{
    Q_UNUSED(progressBarText)
    d->progress = 0;
    ui->progressBar->setMaximum(size);
}

std::filesystem::path ParseCodebaseDialog::compileCommandsPath() const
{
    return ui->compileCommandsFolder->text().toStdString();
}

std::filesystem::path ParseCodebaseDialog::sourcePath() const
{
    return ui->sourceFolder->text().toStdString();
}

std::filesystem::path ParseCodebaseDialog::buildPath() const
{
    return ui->buildFolder->text().toStdString();
}

std::vector<std::string> ParseCodebaseDialog::ignoredItemsAsStdVec()
{
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    auto splitBehavior = QString::SkipEmptyParts;
#else
    auto splitBehavior = Qt::SkipEmptyParts;
#endif
    QStringList ignoreItems = ui->ignorePattern->text().split(',', splitBehavior);
    std::vector<std::string> ignoreList;
    ignoreList.reserve(ignoreItems.size());
    std::transform(ignoreItems.begin(), ignoreItems.end(), std::back_inserter(ignoreList), [](const QString& qstr) {
        return qstr.toStdString();
    });
    return ignoreList;
}

std::vector<std::filesystem::path> ParseCodebaseDialog::nonLakosianDirsAsStdVec()
{
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    auto splitBehavior = QString::SkipEmptyParts;
#else
    auto splitBehavior = Qt::SkipEmptyParts;
#endif
    QStringList nonLakosianDirList = ui->nonLakosians->text().split(',', splitBehavior);
    std::vector<std::filesystem::path> nonLakosianDirs;
    nonLakosianDirs.reserve(nonLakosianDirList.size());
    std::transform(nonLakosianDirList.begin(),
                   nonLakosianDirList.end(),
                   std::back_inserter(nonLakosianDirs),
                   [](const QString& qstr) {
                       return qstr.toStdString();
                   });
    return nonLakosianDirs;
}

void ParseCodebaseDialog::setPluginManager(Codethink::lvtplg::PluginManager& pluginManager)
{
    d->pluginManager = pluginManager;
}
