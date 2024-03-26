// ct_lvtclp_cpp_tool.h                                                   -*-C++-*-

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

#ifndef INCLUDED_CT_LVTCLP_TOOL
#define INCLUDED_CT_LVTCLP_TOOL

//@PURPOSE: Wrap create_codebase_db in a convenient interface
//
//@CLASSES:
//  lvtclp::Tool Wraps create_codebase_db in an interface independent of
//    command line arguments

#include <lvtclp_export.h>

#include <ct_lvtclp_filesystemscanner.h>
#include <ct_lvtclp_headercallbacks.h>

#include <ct_lvtmdb_objectstore.h>

#include <clang/Tooling/CompilationDatabase.h>

#include <filesystem>
#include <initializer_list>
#include <string>
#include <vector>

#include <QLoggingCategory>
#include <QObject>
#include <QString>

namespace Codethink::lvtclp {

// =======================
// class CppTool
// =======================

class LVTCLP_EXPORT CppTool : public QObject {
    Q_OBJECT

  private:
    // TYPES
    struct Private;

    // DATA
    std::unique_ptr<Private> d;

  public:
    enum class UseSystemHeaders { e_Yes, e_No, e_Query };

    // CREATORS
    CppTool(std::filesystem::path sourcePath,
            const std::vector<std::filesystem::path>& compileCommandsJsons,
            const std::filesystem::path& databasePath,
            unsigned numThreads = 1,
            const std::vector<std::string>& ignoreList = {},
            const std::vector<std::filesystem::path>& nonLakosianDirs = {},
            std::vector<std::pair<std::string, std::string>> thirdPartyDirs = {},
            std::vector<std::string> const& userProvidedExtraCompileCommandsArgs = {},
            bool enableLakosianRules = true,
            bool printToConsole = false);

    CppTool(std::filesystem::path sourcePath,
            const clang::tooling::CompileCommand& compileCommand,
            const std::filesystem::path& databasePath,
            const std::vector<std::string>& ignoreList = {},
            const std::vector<std::filesystem::path>& nonLakosianDirs = {},
            std::vector<std::pair<std::string, std::string>> thirdPartyDirs = {},
            std::vector<std::string> const& userProvidedExtraCompileCommandsArgs = {},
            bool enableLakosianRules = true,
            bool printToConsole = false);

    CppTool(std::filesystem::path sourcePath,
            const clang::tooling::CompilationDatabase& db,
            const std::filesystem::path& databasePath,
            unsigned numThreads = 1,
            const std::vector<std::string>& ignoreList = {},
            const std::vector<std::filesystem::path>& nonLakosianDirs = {},
            std::vector<std::pair<std::string, std::string>> thirdPartyDirs = {},
            std::vector<std::string> const& userProvidedExtraCompileCommandsArgs = {},
            bool enableLakosianRules = true,
            bool printToConsole = false);

    ~CppTool() noexcept override;

    // MANIPULATORS
    lvtmdb::ObjectStore& getObjectStore();
    void setSharedMemDb(std::shared_ptr<lvtmdb::ObjectStore> const& sharedMemDb);

    // Get a reference to the output database.
    // The reference is valid so long as this Tool instance remains valid

    void setUseSystemHeaders(UseSystemHeaders value);
    // defines if we should use, not use, or query if we need to use the system headers.
    // This needed because some applications have a --query-system-headers argument
    // that returns true/false, so we can use the cached result instead of
    // creating a clang tool always, and filling the stdout with bogus information.
    // The default value is UseSystemHeaders::e_Query

    bool findPhysicalEntities(bool doIncremental = true);
    // Find files, components, packages, and package groups
    // Returns true on success, false otherwise.
    // If doIncremental is false, everything is re-generated no matter what
    // incremental updates things needs to be done

    bool runPhysical(bool skipScan = false);
    // Run only the steps needed to get the physical dependency graph
    // Returns true on success, false otherwise.
    // If skipScan is true, findPhysicalEntities will not be re-run. It is
    // the caller's responsibility to make sure it has already been run in
    // this case

    bool runFull(bool skipPhysical = false);
    // Build a full database
    // Returns true on success, false otherwise.

    void setShowDatabaseErrors(bool value);
    // enables sending database error information to the consumer

    void setPrintToConsole(bool b);

    void cancelRun();
    // Cancel mid-way through a runPhysical() or runFull(). If one of these
    // are not running, this function has no effect. It is safe to call this
    // from a different thread than that running run*()

    [[nodiscard]] bool finalizingThreads() const;
    // when we cancelRun, this is true, false otherwise.
    // used to ignore some elements on the UI.

    // ACCESSORS
    [[nodiscard]] bool lastRunMadeChanges() const;
    // for testing: true only if there was work to do on the most recent run

    void setHeaderLocationCallback(HeaderCallbacks::HeaderLocationCallback_f const& headerLocationCallback);

    void setHandleCppCommentsCallback(
        std::function<void(
            const std::string& filename, const std::string& briefText, unsigned startLine, unsigned endLine)> const&
            handleCppCommentsCallback);

    Q_SIGNAL void processingFileNotification(QString path);
    // notifies when we have a clang::FrontendAction::BeginSourceFileAction

    Q_SIGNAL void aboutToCallClangNotification(QString notificationMessage, int size);
    // publishes the size of the compilation database before we hand it to
    // clang

    Q_SIGNAL void messageFromThread(const QString& message, long threadId);
    // The thread got some information and want us to display it.

    void messageCallback(const std::string& message, long threadId);
    // emits messageFromThread

  private:
    // PRIVATE MANIPULATORS
    bool ensureSetup();
    // Make sure the code and compilation databases are ready.
    // Returns true on success, false otherwise.
    bool processCompilationDatabase();
    bool ensureDatabaseIsOpen();

    void setupIncrementalUpdate(FilesystemScanner::IncrementalResult& res, bool doIncremental);

    void setDbReadiness(const std::string& readiness);
};

} // namespace Codethink::lvtclp

#endif // INCLUDED_CT_LVTCLP_TOOL
