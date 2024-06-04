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

#ifndef CT_LVTPRJ_PROJECT_FILE_H
#define CT_LVTPRJ_PROJECT_FILE_H

#include <filesystem>
#include <memory>
#include <string>

#include <lvtprj_export.h>

#include <result/result.hpp>

#include <QObject>

namespace Codethink::lvtprj {

struct ProjectFileError {
    std::string errorMessage;
};

class LVTPRJ_EXPORT ProjectFile : public QObject {
    // Represents a Project on disk.
    // the project is a tar file, it needs to be
    // uncompressed to be used. When open() is called,
    // it will uncompress in $TEMP, to get the open location
    // call openLocation(). All changes done in the folder
    // will be compressed again when the project is saved,
    // or closed.
    Q_OBJECT

  public:
    ProjectFile();
    // Create an empty project file.

    ~ProjectFile();
    // Closes the project file.

    [[nodiscard]] static std::filesystem::path backupFolder();

    [[nodiscard]] bool isOpen() const;

    [[nodiscard]] auto createEmpty() -> cpp::result<void, ProjectFileError>;
    // creates a new project that can be saved later.

    [[nodiscard]] auto open(const std::filesystem::path& path) -> cpp::result<void, ProjectFileError>;
    // Extract the contents of path into $TEMP, and sets up the project workspace.

    [[nodiscard]] auto save() -> cpp::result<void, ProjectFileError>;
    // Compresses the contents of $TEMP into the project file.

    enum class BackupFileBehavior { Keep, Discard };

    [[nodiscard]] auto saveAs(const std::filesystem::path& path, BackupFileBehavior behavior)
        -> cpp::result<void, ProjectFileError>;
    // save a copy of this project on the new folder.

    [[nodiscard]] auto close() -> cpp::result<void, ProjectFileError>;

    auto setProjectName(std::string name) -> void;
    // sets the name of the project

    [[nodiscard]] std::string projectName() const;
    // returns the name of the project

    void setProjectInformation(std::string projectInfo);
    // detailed explanation of what the project is.

    [[nodiscard]] std::string projectInformation() const;
    // returns the detailed information of the project

    [[nodiscard]] std::filesystem::path location() const;
    // return the path of where this is saved on disk.

    [[nodiscard]] std::filesystem::path openLocation() const;
    // return the full path of the workspace in $TEMP

    [[nodiscard]] std::filesystem::path cadDatabasePath() const;
    // return the path of the cad database on the open workspace.

    [[nodiscard]] static std::string_view cadDatabaseFilename();

    [[nodiscard]] bool hasCadDatabase() const;
    // does this project has a cad database yet?

    [[nodiscard]] bool isDirty() const;
    // does the project have changes that are not saved in disk yet?

    void setDirty();

    [[nodiscard]] cpp::result<void, ProjectFileError> saveBackup();
    [[nodiscard]] std::filesystem::path backupPath() const;

    void requestAutosave(int msec);

    void setSourceCodePath(std::filesystem::path sourceCodePath);
    [[nodiscard]] virtual std::filesystem::path sourceCodePath() const;

    Q_SIGNAL void communicateError(const Codethink::lvtprj::ProjectFileError& error);
    // This "feeds" the application with error codes when something happen during the Qt event loop.

    [[nodiscard]] std::vector<QJsonDocument> leftPanelTab();
    [[nodiscard]] std::vector<QJsonDocument> rightPanelTab();

    enum BookmarkType { LeftPane, RightPane, Bookmark };
    [[nodiscard]] cpp::result<void, ProjectFileError> saveBookmark(const QJsonDocument& doc, BookmarkType bookmarkType);

    QList<QString> bookmarks() const;
    QJsonDocument getBookmark(const QString& name) const;
    void removeBookmark(const QString& name);
    Q_SIGNAL void bookmarksChanged();

    void prepareSave();

  private:
    auto dumpProjectMetadata() const -> cpp::result<void, ProjectFileError>;
    void loadProjectMetadata();

    struct Private;
    std::unique_ptr<Private> d;
};

} // namespace Codethink::lvtprj

#endif
