/*
 * // Copyright 2023 Codethink Ltd <codethink@codethink.co.uk>
 * // SPDX-License-Identifier: Apache-2.0
 * //
 * // Licensed under the Apache License, Version 2.0 (the "License");
 * // you may not use this file except in compliance with the License.
 * // You may obtain a copy of the License at
 * //
 * //     http://www.apache.org/licenses/LICENSE-2.0
 * //
 * // Unless required by applicable law or agreed to in writing, software
 * // distributed under the License is distributed on an "AS IS" BASIS,
 * // WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * // See the License for the specific language governing permissions and
 * // limitations under the License.
 */

#include <filesystem>
#include <merge_project_databases.h>

#include <ct_lvtmdb_soci_reader.h>
#include <ct_lvtmdb_soci_writer.h>
#include <ct_lvtprj_projectfile.h>

#include <QTemporaryDir>

using namespace Codethink;

cpp::result<void, MergeProjects::MergeProjectError>
MergeProjects::mergeDatabases(std::vector<std::filesystem::path> databases,
                              std::filesystem::path resultingProject,
                              std::optional<MergeProjects::MergeDbProgressReportCallback> progressReportCallback)
{
    using Codethink::lvtprj::ProjectFile;
    using Codethink::lvtprj::ProjectFileError;

    int idx = 0;

    // First, fail early if we can't prepare the temporary project.
    // Because we are using Temporary files / directories via RAII,
    // all the temporaries must be alive through this
    // function
    auto projectFile = ProjectFile{};
    {
        auto err = projectFile.createEmpty();
        if (err.has_error()) {
            return cpp::fail(MergeProjectError{MergeProjectErrorEnum::Error, err.error().errorMessage});
        }
    }
    try {
        std::filesystem::remove(projectFile.codeDatabasePath());
        std::filesystem::remove(projectFile.cadDatabasePath());
    } catch (...) {
        return cpp::fail(MergeProjectError{
            MergeProjectErrorEnum::Error,
            "Error preparing temporary project, check if you have permission to write in the temp directory"});
    }

    std::vector<std::filesystem::path> inner_databases;
    // we might pass databases or projects on the database string, we need
    // to handle each one diferently.
    // if it's a database, we don't need to do anything, but if it's
    // a project, we need to open, get the database data, and only close
    // the project when we finish merging, since it's on a temporary folder
    // that wil be removed when the project closes.
    std::vector<std::unique_ptr<ProjectFile>> projectsFromPath;
    for (const auto& prjUrl : databases) {
        std::string thisDb = prjUrl.string();
        if (thisDb.ends_with(".db")) {
            inner_databases.push_back(prjUrl);
        } else if (thisDb.ends_with(".lks")) {
            auto prjFromPath = std::make_unique<ProjectFile>();
            std::filesystem::path prjPath = prjUrl;
            std::cout << "trying to open " << prjPath << "\n";
            cpp::result<void, ProjectFileError> res = prjFromPath->open(prjPath);
            if (res.has_error()) {
                return cpp::fail(MergeProjectError{MergeProjectErrorEnum::OpenProjectError, res.error().errorMessage});
            }
            inner_databases.push_back(prjFromPath->codeDatabasePath());

            // keeps the project open untill this closes.
            projectsFromPath.push_back(std::move(prjFromPath));
        } else {
            return cpp::fail(MergeProjectError{MergeProjectErrorEnum::InvalidFileExtensionError,
                                               thisDb + " is not a database or project file"});
        }
    }

    // Saves the new database on a temporary path.
    QTemporaryDir dir;
    const std::filesystem::path temporary_db = (dir.path() + QDir::separator() + "code_database.db").toStdString();

    for (const std::filesystem::path& db : std::as_const(inner_databases)) {
        Codethink::lvtmdb::SociReader dbReader;
        Codethink::lvtmdb::SociWriter dbWriter;

        Codethink::lvtmdb::ObjectStore inMemoryDb;

        auto loaded = inMemoryDb.readFromDatabase(dbReader, db.string());
        if (loaded.has_error()) {
            return cpp::fail(MergeProjectError{MergeProjectErrorEnum::RemoveDatabaseError, loaded.error().what});
        }

        if (progressReportCallback.has_value()) {
            progressReportCallback.value()(idx, inner_databases.size(), db.string());
        }

        idx += 1;
        if (!dbWriter.createOrOpen(temporary_db.string())) {
            return cpp::fail(
                MergeProjectError{MergeProjectErrorEnum::RemoveDatabaseError, "Could not open resulting database"});
        }

        inMemoryDb.writeToDatabase(dbWriter);
    }

    // TODO: Create missing tables for CAD database
    try {
        std::filesystem::copy(temporary_db, projectFile.codeDatabasePath());
        std::filesystem::copy(temporary_db, projectFile.cadDatabasePath());
    } catch (...) {
        return cpp::fail(MergeProjectError{
            MergeProjectErrorEnum::Error,
            "Error saving merged database, check if you have permission to write in the temp directory"});
    }

    {
        auto err = projectFile.saveAs(resultingProject, ProjectFile::BackupFileBehavior::Keep);
        if (err.has_error()) {
            return cpp::fail(MergeProjectError{MergeProjectErrorEnum::Error, err.error().errorMessage});
        }
    }

    return {};
}
