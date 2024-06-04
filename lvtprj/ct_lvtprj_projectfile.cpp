// ct_lvtprj_project_file.h                                         -*-C++-*-

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

#include <ct_lvtmdb_soci_writer.h>
#include <ct_lvtprj_projectfile.h>
#include <ct_lvtshr_stringhelpers.h>

#include <KZip>

// Std
#include <algorithm>
#include <iostream>
#include <random>
#include <string_view>

// Qt, Json
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QTimer>

using namespace Codethink::lvtprj;
namespace fs = std::filesystem;

namespace {
constexpr std::string_view CAD_DB = "cad_database.db";
constexpr std::string_view METADATA = "metadata.json";
constexpr std::string_view LEFT_PANEL_HISTORY = "left_panel";
constexpr std::string_view RIGHT_PANEL_HISTORY = "right_panel";
constexpr std::string_view BOOKMARKS_FOLDER = "bookmarks";

bool compressDir(QFileInfo const& saveTo, QDir const& dirToCompress)
{
    if (!QDir{}.exists(saveTo.absolutePath()) && !QDir{}.mkdir(saveTo.absolutePath())) {
        qDebug() << "[compressDir] Could not prepare path to save.";
        return false;
    }

    auto zipFile = KZip(saveTo.absoluteFilePath());
    if (!zipFile.open(QIODevice::WriteOnly)) {
        qDebug() << "[compressDir] Could not open file to compress:" << saveTo;
        qDebug() << zipFile.errorString();
        return false;
    }

    auto r = zipFile.addLocalDirectory(dirToCompress.path(), "");
    if (!r) {
        qDebug() << "[compressDir] Could not add files to project:" << dirToCompress;
        qDebug() << zipFile.errorString();
        return false;
    }

    return true;
}

bool extractDir(QFileInfo const& projectFile, QDir const& openLocation)
{
    auto zipFile = KZip(projectFile.absoluteFilePath());
    if (!zipFile.open(QIODevice::ReadOnly)) {
        qDebug() << "[compressDir] Could not open file to read contents:" << projectFile;
        qDebug() << zipFile.errorString();
        return false;
    }
    const KArchiveDirectory *dir = zipFile.directory();
    return dir->copyTo(openLocation.path());
}

// return a random string, for the unique project folder in $temp.
[[nodiscard]] std::string random_string(size_t length)
{
    constexpr std::string_view charset =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    std::random_device rd;
    std::mt19937 mt(rd());

    // doc says that the range is [lower, upper], and not [lower, upper) as I'd expect.
    std::uniform_int_distribution<size_t> dist(0, charset.size() - 1);

    std::string str(length, 0);
    std::generate_n(str.begin(), length, [&dist, &mt, &charset]() -> char {
        return charset[dist(mt)];
    });
    return str;
}

// creates a non-exisitng 16 alphanumeric folder for the project.
[[nodiscard]] std::pair<std::error_code, fs::path> createNonExistingFolder(const std::filesystem::path& tmp)
{
    fs::path openLocation;
    for (;;) {
        openLocation = tmp / random_string(16);
        bool openLocationExists = fs::exists(fs::status(openLocation));
        if (openLocationExists) {
            continue;
        }

        std::error_code ec;
        fs::create_directories(openLocation, ec);
        return {ec, openLocation};
    }

    assert(false && "unreachable");
    return {std::error_code{}, fs::path{}};
}

[[nodiscard]] auto createUniqueTempFolder() -> cpp::result<std::filesystem::path, ProjectFileError>
{
    std::error_code ec;
    const fs::path tmp = fs::temp_directory_path(ec);
    if (ec) {
        return cpp::fail(ProjectFileError{ec.message()});
    }

    auto [ec2, folderName] = createNonExistingFolder(tmp);
    if (ec2) {
        return cpp::fail(ProjectFileError{ec2.message()});
    }

    return folderName;
}

} // namespace

struct ProjectFile::Private {
    bool isOpen = false;
    // is the project open?

    bool isDirty = false;
    // are there changes in the project that are not saved?

    bool hasGraphDatabase = false;
    // do we have the graph database? (the db that stores the positions and metadata)

    bool allowedAutoSave = true;
    // Will we autosave?

    std::filesystem::path backupLocation;
    // location in the disk where to open and save the backup files.

    std::filesystem::path location;
    // location in disk where to open and save the contents of the project.

    std::filesystem::path openLocation;
    // where this project is open on disk. this is a unzipped folder in $TEMP

    std::filesystem::path sourceCodePath;
    // Location to the actual source code for parsed projects

    std::string projectName;
    // name of the project

    std::string projectInformation;
    // longer explanation of the project

    QTimer projectSaveBackupTimer;
    // when the timer is over, we save the backup file.
};

ProjectFile::ProjectFile(): d(std::make_unique<ProjectFile::Private>())
{
    std::filesystem::path backupF = backupFolder();
    std::filesystem::directory_entry backupDir(backupF);
    if (!backupDir.exists()) {
        try {
            if (!std::filesystem::create_directories(backupDir)) {
                std::cerr << "Could not create directories for the project backup.";
                std::cerr << "Disabling autosave features";
                d->allowedAutoSave = false;
            }
        } catch (std::filesystem::filesystem_error const&) {
            std::cerr << "Could not create directories for the project backup.";
            std::cerr << "Disabling autosave features";
            d->allowedAutoSave = false;
        }
    }
}

ProjectFile::~ProjectFile()
{
    (void) close();
}

std::filesystem::path ProjectFile::backupFolder()
{
    const QString appName = qApp ? qApp->applicationName() : QStringLiteral("CodeVis");
    const QString docPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + QDir::separator()
        + appName + QDir::separator() + "Backup";
    return docPath.toStdString();
}

void ProjectFile::requestAutosave(int msec)
{
    if (!d->allowedAutoSave) {
        return;
    }

    // initialize the autosave timer only once.
    static bool unused = [this]() -> bool {
        d->projectSaveBackupTimer.setSingleShot(true);
        connect(&d->projectSaveBackupTimer, &QTimer::timeout, this, [this] {
            auto res = saveBackup();
            if (res.has_error()) {
                Q_EMIT communicateError(res.error());
            }
        });
        return true;
    }();
    (void) unused;

    if (msec == 0) {
        auto res = saveBackup();
        if (res.has_error()) {
            Q_EMIT communicateError(res.error());
        }
    } else {
        d->projectSaveBackupTimer.start(std::chrono::milliseconds{msec});
    }
}

std::filesystem::path ProjectFile::backupPath() const
{
    return d->backupLocation;
}

auto ProjectFile::close() -> cpp::result<void, ProjectFileError>
{
    if (!isOpen()) {
        return {}; // closing a closed project is a noop.
    }

    try {
        if (!fs::remove_all(d->openLocation)) {
            return cpp::fail(ProjectFileError{"Error removing temporary folder"});
        }
    } catch (std::filesystem::filesystem_error& error) {
        return cpp::fail(ProjectFileError{error.what()});
    }

    d = std::make_unique<ProjectFile::Private>();
    return {};
}

bool ProjectFile::isOpen() const
{
    return d->isOpen;
}

auto ProjectFile::createEmpty() -> cpp::result<void, ProjectFileError>
{
    cpp::result<fs::path, ProjectFileError> tmpFolder = createUniqueTempFolder();
    if (tmpFolder.has_error()) {
        return cpp::fail(tmpFolder.error());
    }

    d->openLocation = tmpFolder.value();
    d->isOpen = true;

    lvtmdb::SociWriter writer;
    if (!writer.createOrOpen(cadDatabasePath().string())) {
        return cpp::fail(ProjectFileError{
            "Couldnt create database. Tip: Make sure the database spec folder is installed properly."});
    }

    return {};
}

auto ProjectFile::open(const std::filesystem::path& path) -> cpp::result<void, ProjectFileError>
{
    // std::string lacks .endsWith - cpp20 fixes this but we are on 17.
    const QString tmp = QString::fromStdString(path.string());
    const std::string filePath = tmp.endsWith(".lks") ? tmp.toStdString() : tmp.toStdString() + ".lks";

    const bool exists = fs::exists(fs::status(filePath));
    if (!exists) {
        return cpp::fail(ProjectFileError{"Project file does not exist on disk"});
    }

    cpp::result<fs::path, ProjectFileError> tmpFolder = createUniqueTempFolder();
    if (tmpFolder.has_error()) {
        return cpp::fail(ProjectFileError{tmpFolder.error()});
    }

    d->openLocation = tmpFolder.value();
    const auto projectFile = QFileInfo{QString::fromStdString(filePath)};
    const auto openLocation = QDir{QString::fromStdString(d->openLocation.string())};
    if (!extractDir(projectFile, openLocation)) {
        return cpp::fail(ProjectFileError{"Failed to extract project contents"});
    }

    d->location = filePath;
    d->isOpen = true;
    loadProjectMetadata();

    return {};
}

auto ProjectFile::saveAs(const fs::path& path, BackupFileBehavior behavior) -> cpp::result<void, ProjectFileError>
{
    fs::path projectPath = path;
    if (!projectPath.string().ends_with(".lks")) {
        projectPath += ".lks";
    }

    // create a backup file the project already exists.
    // if any error happens, the user can revert to the backup.
    try {
        if (fs::exists(fs::status(projectPath))) {
            // fs path has no operator `+`, so I can't initialize correctly.
            // but it has operator +=. :|
            fs::path bkpPath = projectPath;
            bkpPath += ".bpk";
            fs::rename(projectPath, bkpPath);
        }
    } catch (fs::filesystem_error& error) {
        return cpp::fail(ProjectFileError{error.what()});
    }

    d->location = projectPath;

    auto dumped = dumpProjectMetadata();
    if (dumped.has_error()) {
        return cpp::fail(dumped.error());
    }

    // save new file.
    const auto saveWhat = QDir{QString::fromStdString(d->openLocation.string())};
    const auto saveTo = QFileInfo{QString::fromStdString(projectPath.string())};
    if (!compressDir(saveTo, saveWhat)) {
        qDebug() << "Failed to generate project file";
        return cpp::fail(ProjectFileError{"Failed to generate project file"});
    }

    // We correctly saved the file, we can delete the auto backup file.
    if (behavior == BackupFileBehavior::Discard) {
        if (!std::filesystem::remove(d->backupLocation)) {
            // error deleting the backup file, but the project file was correctly saved.
            // this is not a hard error, just complain on the terminal.
            qDebug() << "Error removing backup file";
        }
    }

    return {};
}

cpp::result<void, ProjectFileError> ProjectFile::saveBackup()
{
    return saveAs(d->backupLocation, BackupFileBehavior::Keep);
}

auto ProjectFile::save() -> cpp::result<void, ProjectFileError>
{
    auto ret = saveAs(d->location, BackupFileBehavior::Discard);
    if (ret.has_error()) {
        return ret;
    }

    d->isDirty = false;
    return ret;
}

void ProjectFile::setProjectName(std::string name)
{
    d->projectName = std::move(name);
    d->backupLocation = backupFolder().string() + "/" + d->projectName + ".lks";
}

void ProjectFile::setProjectInformation(std::string projectInfo)
{
    d->projectInformation = std::move(projectInfo);
}

fs::path ProjectFile::openLocation() const
{
    assert(d->isOpen);
    return d->openLocation;
}

std::string_view ProjectFile::cadDatabaseFilename()
{
    return CAD_DB;
}

fs::path ProjectFile::cadDatabasePath() const
{
    return d->openLocation / CAD_DB;
}

bool ProjectFile::hasCadDatabase() const
{
    const fs::path cadDb = cadDatabasePath();
    return fs::exists(fs::status(cadDb));
}

std::string ProjectFile::projectInformation() const
{
    return d->projectInformation;
}

std::string ProjectFile::projectName() const
{
    return d->projectName;
}

auto ProjectFile::dumpProjectMetadata() const -> cpp::result<void, ProjectFileError>
{
    QJsonObject obj{{"name", QString::fromStdString(d->projectName)},
                    {"description", QString::fromStdString(d->projectInformation)},
                    {"sourcePath", QString::fromStdString(d->sourceCodePath.string())}};
    QJsonDocument doc(obj);

    const fs::path metadata_file = d->openLocation / METADATA;
    auto saveFileName = QString::fromStdString(metadata_file.string());
    QFile saveFile(saveFileName);
    if (!saveFile.open(QIODevice::WriteOnly)) {
        return cpp::fail(ProjectFileError{saveFile.errorString().toStdString()});
    }

    qint64 bytesSaved = saveFile.write(doc.toJson());
    if (bytesSaved == 0) {
        return cpp::fail(ProjectFileError{"Error saving metadata file, zero bytes size."});
    }
    return {};
}

void ProjectFile::loadProjectMetadata()
{
    const fs::path metadata_file = d->openLocation / METADATA;
    const auto loadFileName = QString::fromStdString(metadata_file.string());

    QFile loadFile(loadFileName);
    loadFile.open(QIODevice::ReadOnly);

    const QByteArray fileData = loadFile.readAll();
    QJsonObject obj = QJsonDocument::fromJson(fileData).object();

    if (obj.contains("name")) {
        d->projectName = obj.value("name").toString().toStdString();
    }
    if (obj.contains("description")) {
        d->projectInformation = obj.value("description").toString().toStdString();
    }
    if (obj.contains("sourcePath")) {
        d->sourceCodePath = obj.value("sourcePath").toString().toStdString();
    }
}

fs::path ProjectFile::location() const
{
    return d->location;
}

void ProjectFile::setSourceCodePath(std::filesystem::path sourceCodePath)
{
    d->sourceCodePath = std::move(sourceCodePath);
}

std::filesystem::path ProjectFile::sourceCodePath() const
{
    return d->sourceCodePath;
}

void ProjectFile::setDirty()
{
    d->isDirty = true;
}

bool ProjectFile::isDirty() const
{
    return d->isDirty;
}

std::vector<QJsonDocument> jsonDocInFolder(const QString& folder)
{
    // get the list of files on the left panel tab
    QDir dir(folder);
    const auto dirEntries = dir.entryList(QDir::Filter::NoDotAndDotDot | QDir::Filter::Files);

    std::vector<QJsonDocument> ret;
    ret.reserve(dirEntries.size());

    for (const QString& filePath : dirEntries) {
        QFile file(folder + QDir::separator() + filePath);
        const bool opened = file.open(QIODevice::ReadOnly);
        if (!opened) {
            continue;
        }
        ret.push_back(QJsonDocument::fromJson(file.readAll()));
    }

    return ret;
}

std::vector<QJsonDocument> ProjectFile::leftPanelTab()
{
    const auto folder = QString::fromStdString((d->openLocation / LEFT_PANEL_HISTORY).string());
    return jsonDocInFolder(folder);
}

std::vector<QJsonDocument> ProjectFile::rightPanelTab()
{
    const auto folder = QString::fromStdString((d->openLocation / RIGHT_PANEL_HISTORY).string());
    return jsonDocInFolder(folder);
}

void ProjectFile::prepareSave()
{
    for (const auto folder : {LEFT_PANEL_HISTORY, RIGHT_PANEL_HISTORY}) {
        QDir full_folder(QString::fromStdString((d->openLocation / folder).string()));
        const auto dirEntries = full_folder.entryList(QDir::Filter::NoDotAndDotDot | QDir::Filter::Files);
        for (const QString& filePath : dirEntries) {
            QFile file(full_folder.absolutePath() + QDir::separator() + filePath);
            file.remove();
        }
    }
}

QList<QString> ProjectFile::bookmarks() const
{
    const auto folderPath = QString::fromStdString((d->openLocation / BOOKMARKS_FOLDER).string());
    QDir dir(folderPath);

    auto dirEntries = dir.entryList(QDir::Filter::NoDotAndDotDot | QDir::Filter::Files);
    for (auto& dirEntry : dirEntries) {
        dirEntry.replace(".json", "");
    }

    qDebug() << dirEntries;
    return dirEntries;
}

QJsonDocument ProjectFile::getBookmark(const QString& name) const
{
    const auto folderPath = QString::fromStdString((d->openLocation / BOOKMARKS_FOLDER).string());
    QFile bookmark(folderPath + QDir::separator() + name + ".json");
    if (!bookmark.exists()) {
        qDebug() << "Json document doesn't exists";
    }
    if (!bookmark.open(QIODevice::ReadOnly)) {
        qDebug() << "Impossible to load json file";
    }

    return QJsonDocument::fromJson(bookmark.readAll());
}

void ProjectFile::removeBookmark(const QString& name)
{
    const auto folderPath = QString::fromStdString((d->openLocation / BOOKMARKS_FOLDER).string());
    QFile bookmark(folderPath + QDir::separator() + name + ".json");
    if (!bookmark.exists()) {
        qDebug() << "Json document doesn't exists" << bookmark.fileName();
        return;
    }

    if (!bookmark.remove()) {
        qDebug() << "Couldn't remove bookmark";
        return;
    }
    setDirty();

    Q_EMIT bookmarksChanged();
}

[[nodiscard]] cpp::result<void, ProjectFileError> ProjectFile::saveBookmark(const QJsonDocument& doc,
                                                                            ProjectFile::BookmarkType bookmarkType)
{
    const auto file_id = doc.object()["id"].toString();
    const auto folder = [bookmarkType]() -> std::string_view {
        switch (bookmarkType) {
        case LeftPane:
            return LEFT_PANEL_HISTORY;
        case RightPane:
            return RIGHT_PANEL_HISTORY;
        case Bookmark:
            return BOOKMARKS_FOLDER;
        }

        // there's a bug on compilers (at least g++) and it does not
        // realizes I'm actually returning something always from the
        // switch case, requiring me to also add a return afterwards.
        assert(false && "should never hit here");
        return BOOKMARKS_FOLDER;
    }();

    QDir dir; // just to call mkpath, this is not a static method.
    const auto folderPath = QString::fromStdString((d->openLocation / folder).string());
    dir.mkpath(folderPath);

    QFile saveFile(folderPath + QDir::separator() + (file_id + ".json"));
    if (!saveFile.open(QIODevice::WriteOnly)) {
        return cpp::fail(ProjectFileError{saveFile.errorString().toStdString()});
    }

    qint64 bytesSaved = saveFile.write(doc.toJson());
    if (bytesSaved == 0) {
        return cpp::fail(ProjectFileError{"Error saving bookmark, zero bytes saved."});
    }

    if (folder == BOOKMARKS_FOLDER) {
        Q_EMIT bookmarksChanged();
    }

    setDirty();
    return {};
}
