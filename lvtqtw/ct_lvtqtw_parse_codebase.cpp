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
#include <ct_lvtqtw_parse_codebase.h>

#include <ct_lvtclp_cpp_tool.h>
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

#include <KNotification>

#include <clang/Tooling/JSONCompilationDatabase.h>
#include <preferences.h>
#include <soci/soci.h>

using namespace Codethink::lvtqtw;

namespace {
constexpr const char *COMPILE_COMMANDS = "compile_commands.json";
constexpr const char *NON_LAKOSIAN_DIRS_SETTING = "non_lakosian_dirs";

bool compressFiles(QFileInfo const& saveTo, QList<QFileInfo> const& files)
{
    if (!QDir{}.exists(saveTo.absolutePath()) && !QDir{}.mkdir(saveTo.absolutePath())) {
        qDebug() << "[compressFiles] Could not prepare path to save.";
        return false;
    }

    auto zipFile = KZip(saveTo.absoluteFilePath());
    if (!zipFile.open(QIODevice::WriteOnly)) {
        qDebug() << "[compressFiles] Could not open file to compress:" << saveTo;
        qDebug() << zipFile.errorString();
        return false;
    }

    for (auto const& fileToCompress : qAsConst(files)) {
        auto r = zipFile.addLocalFile(fileToCompress.path(), "");
        if (!r) {
            qDebug() << "[compressFiles] Could not add files to project:" << fileToCompress;
            qDebug() << zipFile.errorString();
            return false;
        }
    }

    return true;
}

QString createSysinfoFileAt(const QString& lPath, const QString& ignorePattern)
{
    QFile systemInformation(lPath + QDir::separator() + "system_information.txt");
    if (!systemInformation.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "Error opening the sys info file.";
        return {};
    }

    QString systemInfoData;

    // this string should not be called with "tr", we do not want to
    // translate this to other languages, I have no intention on reading
    // a log file in russian.
    systemInfoData += "CPU: " + QSysInfo::currentCpuArchitecture() + "\n"
        + "Operating System: " + QSysInfo::productType() + "\n" + "Version " + QSysInfo::productVersion() + "\n"
        + "Ignored File Information: " + ignorePattern + "\n" + "CodeVis version:" + QString(__DATE__);

    systemInformation.write(systemInfoData.toLocal8Bit());
    systemInformation.close();

    return lPath + QDir::separator() + "system_information.txt";
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

    std::optional<std::reference_wrapper<Codethink::lvtplg::PluginManager>> pluginManager = std::nullopt;
    QElapsedTimer parseTimer;

    bool enableLakosianRules = true;
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

    connect(ui->btnSaveOutput, &QPushButton::clicked, this, &ParseCodebaseDialog::saveOutput);

    connect(ui->searchCompileCommands, &QPushButton::clicked, this, &ParseCodebaseDialog::searchForBuildFolder);

    connect(ui->nonLakosiansSearch, &QPushButton::clicked, this, &ParseCodebaseDialog::searchForNonLakosianDir);
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
            // endParse will emit parseFinished
        } else {
            Q_EMIT parseFinished(State::Idle);
        }
    });

    ui->progressBar->setMinimum(0);

    connect(ui->compileCommandsFolder, &QLineEdit::textChanged, this, [this] {
        validate();
    });

    connect(ui->sourceFolder, &QLineEdit::textChanged, this, [this] {
        validate();
    });

    ui->projectBuildFolderError->setVisible(false);
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

    validate();
}

void ParseCodebaseDialog::validate()
{
    // a QValidator will not allow the string to be set, but we need to tell the user the reason that
    // the string was not set. So instead of using the `setValidator` calls on QLineEdit, we *accept*
    // the wrong string, and if the validator is invalid, we display an error message, while also blocking
    // the Parse button.
    const auto emptyErrorMsg = tr("This field can't be empty");
    const auto wslErrorMsg = tr("The software does not support wsl, use the native linux build.");
    const auto errorCss = QString("border: 1px solid red");
    const auto missingCompileCommands = tr("The specified folder does not contains compile_commands.json");
    const auto wslStr = std::string{"wsl://"};

    bool disableParse = false;
    QFileInfo inf(ui->compileCommandsFolder->text() + QDir::separator() + "compile_commands.json");
    if (ui->compileCommandsFolder->text().isEmpty()) {
        ui->projectBuildFolderError->setVisible(true);
        ui->projectBuildFolderError->setText(emptyErrorMsg);
        ui->compileCommandsFolder->setStyleSheet(errorCss);
        disableParse = true;
    } else if (!inf.exists()) {
        ui->projectBuildFolderError->setVisible(true);
        ui->projectBuildFolderError->setText(missingCompileCommands);
        ui->compileCommandsFolder->setStyleSheet(errorCss);
        disableParse = true;
    } else if (ui->compileCommandsFolder->text().startsWith(wslStr.c_str())) {
        ui->projectBuildFolderError->setVisible(true);
        ui->projectBuildFolderError->setText(wslErrorMsg);
        ui->compileCommandsFolder->setStyleSheet(errorCss);
        disableParse = true;
    } else {
        ui->projectBuildFolderError->setVisible(false);
        ui->compileCommandsFolder->setStyleSheet(QString());
    }

    if (ui->sourceFolder->text().isEmpty()) {
        ui->projectSourceFolderError->setVisible(true);
        ui->projectSourceFolderError->setText(emptyErrorMsg);
        ui->sourceFolder->setStyleSheet(errorCss);
        disableParse = true;
    } else if (ui->sourceFolder->text().startsWith(wslStr.c_str())) {
        ui->projectSourceFolderError->setVisible(true);
        ui->projectSourceFolderError->setText(wslErrorMsg);
        ui->sourceFolder->setStyleSheet(errorCss);
        disableParse = true;
    } else {
        ui->projectSourceFolderError->setVisible(false);
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
    const auto dbFilename = std::string(lvtprj::ProjectFile::codebaseDbFilename());
    const auto qDbFilename = QString::fromStdString(dbFilename);
    return d->codebasePath + QDir::separator() + qDbFilename;
}

void ParseCodebaseDialog::searchForBuildFolder()
{
    auto openDir = [&]() {
        auto lastDir = QDir{ui->compileCommandsFolder->text()};
        if (!lastDir.isEmpty() && lastDir.exists()) {
            return lastDir.canonicalPath();
        }
        return QDir::homePath();
    }();

    const QString buildDirectory = QFileDialog::getExistingDirectory(this, tr("Project Build Directory"), openDir);

    if (buildDirectory.isEmpty()) {
        return;
    }

    ui->compileCommandsFolder->setText(buildDirectory);

    // Tries to determine the source folder automatically
    auto sourceFolderGuess = std::filesystem::canonical(std::filesystem::path(buildDirectory.toStdString()) / "..");
    ui->sourceFolder->setText(QString::fromStdString(sourceFolderGuess.string()));
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

void ParseCodebaseDialog::saveOutput()
{
    const QUrl directory = QFileDialog::getExistingDirectoryUrl(this);
    if (!directory.isValid()) {
        return;
    }

    const QString lPath = directory.toLocalFile();

    const std::filesystem::path compile_commands_orig =
        (ui->compileCommandsFolder->text() + QDir::separator() + COMPILE_COMMANDS).toStdString();
    const std::filesystem::path compile_commands_dest = (lPath + QDir::separator() + COMPILE_COMMANDS).toStdString();
    try {
        std::filesystem::copy_file(compile_commands_orig, compile_commands_dest);
    } catch (std::filesystem::filesystem_error& e) {
        qDebug() << "Could not copy compile_commands.json to the save folder" << e.what();
        return;
    }

    const QString sysInfoFile = createSysinfoFileAt(lPath, ui->ignorePattern->text());
    const QString compileCommandsFile = lPath + QDir::separator() + COMPILE_COMMANDS;

    QList<QFileInfo> textFiles;
    textFiles.append(QFileInfo{compileCommandsFile});
    textFiles.append(QFileInfo{sysInfoFile});
    for (int i = 0; i < ui->tabWidget->count(); i++) {
        auto *textEdit = qobject_cast<TextView *>(ui->tabWidget->widget(i));
        QString saveFilePath = ui->tabWidget->tabText(i);
        saveFilePath.replace(' ', '_');
        saveFilePath.append(".txt");
        saveFilePath = lPath + QDir::separator() + saveFilePath;
        textEdit->saveFileTo(saveFilePath);
        textFiles.append(QFileInfo{saveFilePath});
    }

    const QFileInfo outputFile =
        QFileInfo{directory.toLocalFile() + QDir::separator() + "codevis_dump_"
                  + QString::number(QDateTime::currentDateTime().toMSecsSinceEpoch()) + ".zip"};

    if (compressFiles(outputFile, textFiles)) {
        QMessageBox::information(this,
                                 tr("Export Debug File"),
                                 tr("File saved successfully at \n%1").arg(outputFile.fileName()));
    } else {
        QMessageBox::critical(this, tr("Export Debug File"), tr("Error exporting the build data."));
    }

    for (const auto& textFile : qAsConst(textFiles)) {
        std::filesystem::remove(textFile.absoluteFilePath().toStdString());
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

    if (ui->tabWidget->count() != 0) {
        // we already have some debug output. Don't close it, allow saving it.
        ui->btnSaveOutput->setEnabled(true);
        ui->stackedWidget->setCurrentIndex(1);
    } else {
        // no debug output in memory. Don't show it.
        ui->btnSaveOutput->setEnabled(false);
        ui->stackedWidget->setCurrentIndex(0);
    }
    validate();
}

void ParseCodebaseDialog::initParse()
{
    // initParse() is called twice, once for the Physical, and again for the Logical parses.
    assert(d->dialogState == State::Idle || d->dialogState == State::RunAllPhysical);

    // re-enable cancel button if it was disabled (e.g. because it was used on
    // the last run)
    ui->btnCancelParse->setEnabled(true);

    // We can't remove the tabs on ::reset, because the user might want to
    // save the tab information on disk. We can't remove the tabs on ::close
    // because the user can close and reopen the dialog multiple times while
    // the parse is running, so the only time I can safely remove the tabs
    // is when we start a new parse from scratch.
    removeParseMessageTabs();

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
    const auto compileCommandsDir = ui->compileCommandsFolder->text();
    const auto compileCommandsJson = (compileCommandsDir + QDir::separator() + COMPILE_COMMANDS).toStdString();
    const auto compileCommandsExists = QFileInfo::exists(QString::fromStdString(compileCommandsJson));
    const auto physicalRun = d->dialogState == State::RunPhysicalOnly || d->dialogState == State::RunAllPhysical;
    const auto mustGenerateCompileCommands = physicalRun && (!compileCommandsExists || ui->runCmake->checkState());
    const auto ignoreList = ignoredItemsAsStdVec();
    const auto nonLakosianDirs = nonLakosianDirsAsStdVec();
    if (mustGenerateCompileCommands) {
        runCMakeAndInitParse_Step2(compileCommandsJson, ignoreList, nonLakosianDirs);
    } else {
        initParse_Step2(compileCommandsJson, ignoreList, nonLakosianDirs);
    }
}

void ParseCodebaseDialog::runCMakeAndInitParse_Step2(const std::string& compileCommandsJson,
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

            initParse_Step2(compileCommandsJson, ignoreList, nonLakosianDirs);
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

void ParseCodebaseDialog::initParse_Step2(const std::string& compileCommandsJson,
                                          const std::vector<std::string>& ignoreList,
                                          const std::vector<std::filesystem::path>& nonLakosianDirs)
{
    const bool catchCodeAnalysisOutput = Preferences::enableCodeParseDebugOutput();

    if (!d->tool_p) {
        d->tool_p = std::make_unique<lvtclp::CppTool>(sourcePath(),
                                                      std::vector<std::filesystem::path>{compileCommandsJson},
                                                      codebasePath().toStdString(),
                                                      ui->threadCount->value(),
                                                      ignoreList,
                                                      nonLakosianDirs,
                                                      d->thirdPartyPathMapping,
                                                      d->enableLakosianRules,
                                                      catchCodeAnalysisOutput);
    }
#ifdef CT_ENABLE_FORTRAN_SCANNER
    if (!d->fortran_tool_p) {
        d->fortran_tool_p = lvtclp::fortran::Tool::fromCompileCommands(compileCommandsJson);
    }
    d->fortran_tool_p->setSharedMemDb(d->sharedMemDb);
#endif
    d->tool_p->setSharedMemDb(d->sharedMemDb);

    d->tool_p->setShowDatabaseErrors(ui->showDbErrors->isChecked());
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

    connect(d->tool_p.get(),
            &lvtclp::CppTool::messageFromThread,
            this,
            &ParseCodebaseDialog::receivedMessage,
            Qt::QueuedConnection);

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

    d->parseThread = QThread::create(threadFn);

    connect(d->parseThread, &QThread::finished, this, &ParseCodebaseDialog::readyForDbUpdate);

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

    // it is okay to close the window after the physical parse is completed and
    // allow the logical parse to continue in the background. Otherwise disable
    // closing while a parse is running.
    ui->btnClose->setEnabled(d->dialogState == State::RunAllLogical);

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
            // TODO: prompt user for somewhere else to write the database
            return;
        }
    }

    {
        lvtmdb::SociWriter writer;
        writer.createOrOpen(path);
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

    endParse();
}

void ParseCodebaseDialog::endParse()
{
    assert(d->dialogState != State::Idle);

    ui->btnParse->setEnabled(true);
    ui->progressBarText->setVisible(false);
    ui->progressBar->setValue(0);

    if (d->dialogState == State::Killed) {
        ui->errorText->setText(tr("Parsing operation killed."));
        ui->errorText->show();
        d->dialogState = State::Idle;
        Q_EMIT parseFinished(State::Killed);
        d->sharedMemDb->withRWLock([&] {
            d->sharedMemDb->clear();
        });
        d->tool_p = nullptr;
#ifdef CT_ENABLE_FORTRAN_SCANNER
        d->fortran_tool_p = nullptr;
#endif
        return;
    }

    if (!d->threadSuccess) {
        ui->errorText->setText(tr("Error parsing codebase with clang"));
        ui->errorText->show();
        d->dialogState = State::Idle;
        Q_EMIT parseFinished(State::Idle);
        d->sharedMemDb->withRWLock([&] {
            d->sharedMemDb->clear();
        });
        d->tool_p = nullptr;
#ifdef CT_ENABLE_FORTRAN_SCANNER
        d->fortran_tool_p = nullptr;
#endif
        return;
    }

    if (d->dialogState == State::RunAllPhysical) {
        // move on to RunAllLogical
        ui->errorText->setText(tr("Physical parsing done. Continuing with logical parse"));
        ui->errorText->show();
        Q_EMIT parseFinished(State::RunAllPhysical);

        QTime time(0, 0);
        time = time.addMSecs(d->parseTimer.elapsed());

        KNotification *notification = new KNotification("parserFinished");
        notification->setText(
            tr("Physical Parse finished with: %1<br/>Starting Logical Parse.").arg(time.toString("mm:ss.zzz")));
        notification->sendEvent();
        d->parseTimer.restart();
        initParse();
        return;
    }

    if (d->dialogState == State::RunPhysicalOnly) {
        Q_EMIT parseFinished(d->dialogState);

        QTime time(0, 0);
        time = time.addMSecs(d->parseTimer.elapsed());
        KNotification *notification = new KNotification("parserFinished");
        notification->setText(tr("Physical Parse finished with: %1.").arg(time.toString("mm:ss.zzz")));
        notification->sendEvent();
    } else if (d->dialogState == State::RunAllLogical) {
        QTime time(0, 0);
        time = time.addMSecs(d->parseTimer.elapsed());

        KNotification *notification = new KNotification("parserFinished");
        notification->setText(tr("Logical Parse finished with: %1.").arg(time.toString("mm:ss.zzz")));
        notification->sendEvent();
        Q_EMIT parseFinished(d->dialogState);
    }
    d->dialogState = State::Idle;
    d->sharedMemDb->withRWLock([&] {
        d->sharedMemDb->clear();
    });
    d->tool_p = nullptr;
#ifdef CT_ENABLE_FORTRAN_SCANNER
    d->fortran_tool_p = nullptr;
#endif
    d->parseTimer.invalidate();

    if (d->pluginManager) {
        soci::session db;
        std::string path = codebasePath().toStdString();
        db.open(*soci::factory_sqlite3(), path);

        auto& pm = (*d->pluginManager).get();
        auto runQueryOnDatabase = [&](std::string const& dbQuery) -> std::vector<std::vector<RawDBData>> {
            return lvtmdb::SociHelper::runSingleQuery(db, dbQuery);
        };
        pm.callHooksOnParseCompleted(runQueryOnDatabase);
    }

    close();
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

void ParseCodebaseDialog::aboutToCallClangNotification(int size)
{
    d->progress = 0;

    ui->progressBar->setMaximum(size);
}

void ParseCodebaseDialog::receivedMessage(const QString& message, long threadId)
{
    // index 0 - help message, 1 - tab widget.
    if (ui->stackedWidget->currentIndex() == 0) {
        ui->stackedWidget->setCurrentIndex(1);
    }

    auto it = d->threadIdToWidget.find(threadId);
    if (it == std::end(d->threadIdToWidget)) {
        const int nr = ui->tabWidget->count() + 1;
        auto *textView = new TextView(nr);
        d->threadIdToWidget[threadId] = textView;

        textView->setAcceptRichText(false);
        textView->setReadOnly(true);
        textView->appendText(message);

        const QString tabText = [this, nr] {
            switch (d->dialogState) {
            case State::RunPhysicalOnly:
                [[fallthrough]];
            case State::RunAllPhysical:
                return tr("Physical Analysis %1").arg(nr);
            case State::RunAllLogical:
                return tr("Logical Analysis %1").arg(nr);
            default:
                return tr("Unknown State %1").arg(nr);
            }
        }();

        ui->tabWidget->addTab(textView, tabText);
    }
    TextView *textView = d->threadIdToWidget[threadId];
    textView->appendText(message);
}

std::filesystem::path ParseCodebaseDialog::buildPath() const
{
    return ui->compileCommandsFolder->text().toStdString();
}

std::filesystem::path ParseCodebaseDialog::sourcePath() const
{
    return ui->sourceFolder->text().toStdString();
}

void ParseCodebaseDialog::removeParseMessageTabs()
{
    for (int i = 0; i < ui->tabWidget->count(); i++) {
        ui->tabWidget->removeTab(0);
    }
    ui->stackedWidget->setCurrentIndex(0);
    for (auto [_, view] : d->threadIdToWidget) {
        delete view;
    }
    d->threadIdToWidget.clear();
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
