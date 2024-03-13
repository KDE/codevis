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
                              MergeProjects::MergeDbProgressReportCallback progressReportCallback)
{
    using Codethink::lvtprj::ProjectFile;
    using Codethink::lvtprj::ProjectFileError;

    int idx = 0;

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
                return cpp::fail(
                    MergeProjectError{MergeProjectErrorEnum::RemoveDatabaseError, res.error().errorMessage});
            }
            inner_databases.push_back(prjFromPath->codeDatabasePath());

            // keeps the project open untill this closes.
            projectsFromPath.push_back(std::move(prjFromPath));
        }
    }

    // Saves the new dataase on a temporary path.
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

        progressReportCallback(idx, inner_databases.size(), db.string());

        idx += 1;
        if (!dbWriter.createOrOpen(temporary_db.string())) {
            return cpp::fail(
                MergeProjectError{MergeProjectErrorEnum::RemoveDatabaseError, "Could not open resulting database"});
        }

        inMemoryDb.writeToDatabase(dbWriter);
    }

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
        return cpp::fail(MergeProjectError{MergeProjectErrorEnum::Error, "Error preparing temporary project"});
    }

    std::filesystem::copy(temporary_db, projectFile.codeDatabasePath());

    // TODO: Create missing tables for CAD database
    std::filesystem::copy(temporary_db, projectFile.cadDatabasePath());

    {
        auto err = projectFile.saveAs(resultingProject, ProjectFile::BackupFileBehavior::Keep);
        if (err.has_error()) {
            return cpp::fail(MergeProjectError{MergeProjectErrorEnum::Error, err.error().errorMessage});
        }
    }

    return {};
}
