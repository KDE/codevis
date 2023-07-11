// ct_lvtqtw_parse_codebase.h                                             -*-C++-*-

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

#ifndef INCLUDED_LVTQTW_PARSECODEBASE
#define INCLUDED_LVTQTW_PARSECODEBASE

#include <lvtqtw_export.h>

#include <QDialog>
#include <QProcess>
#include <QString>

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace Ui {
class ParseCodebaseDialog;
}

namespace Codethink::lvtqtw {

class LVTQTW_EXPORT ParseCodebaseDialog : public QDialog {
    Q_OBJECT
  public:
    enum class State {
        Idle, // before or after any parse has run
        Killed, // we are in the process of cancelling a parse
        RunPhysicalOnly, // we are doing a physical only parse
        RunAllPhysical, // we are doing the physical stage of a full parse
        RunAllLogical, // we are doing the logical stage of a full parse
    };

    explicit ParseCodebaseDialog(QWidget *parent = nullptr);
    ~ParseCodebaseDialog() override;

    Q_SIGNAL void parseStarted(Codethink::lvtqtw::ParseCodebaseDialog::State currentState);
    Q_SIGNAL void
    parseStep(Codethink::lvtqtw::ParseCodebaseDialog::State currentState, int currentProgress, int maxProgress);
    Q_SIGNAL void parseFinished(Codethink::lvtqtw::ParseCodebaseDialog::State state);
    Q_SIGNAL void readyForDbUpdate();

    void setCodebasePath(const QString& path);
    [[nodiscard]] QString codebasePath() const;
    [[nodiscard]] std::filesystem::path buildPath() const;
    [[nodiscard]] std::filesystem::path sourcePath() const;

    Q_SLOT void updateDatabase();

    bool isLoadAllowedDependenciesChecked() const;

  protected:
    void showEvent(QShowEvent *event) override;

    void initParse();
    // Sets up everything needed for the parse.

    void runCMakeAndInitParse_Step2(const std::string& compileCommandsJson,
                                    const std::vector<std::string>& ignoreList,
                                    const std::vector<std::filesystem::path>& nonLakosianDirs);
    // Runs CMake to generate compile_commands.json file before starting parse step 2

    void initParse_Step2(std::string compileCommandsJson,
                         const std::vector<std::string>& ignoreList,
                         const std::vector<std::filesystem::path>& nonLakosianDirs);
    // Actually starts the parsing process.

    void endParse();
    void validate();

    Q_SLOT void processingFileNotification(const QString& path);
    Q_SLOT void aboutToCallClangNotification(int size);
    Q_SLOT void receivedMessage(const QString& message, long threadId);
    Q_SLOT void saveOutput();
    // creates a zip file containing all the information from the system
    // plus all the texts from the thread-tabs.

    Q_SLOT void searchForBuildFolder();
    // called when one clicks on the `search` button for the build folder.

    Q_SLOT void searchForNonLakosianDir();
    // called when one clicks on the 'search' button for the non-lakosian dirs

    Q_SLOT void searchForSourceFolder();
    // called when one clicks on the `search` button for the source folder.

    void selectThirdPartyPkgMapping();

    [[nodiscard]] static QString getNonLakosianDirSettings(const QString& buildDir);
    // Get the non-lakosian dir information from QSettings

    static void setNonLakosianDirSettings(const QString& buildDir, const QString& nonLakosianDirs);
    // Sore this information in QSettings

    void removeParseMessageTabs();

    void reset();

    std::vector<std::string> ignoredItemsAsStdVec();
    std::vector<std::filesystem::path> nonLakosianDirsAsStdVec();

    struct Private;
    std::unique_ptr<Private> d;
    std::unique_ptr<Ui::ParseCodebaseDialog> ui;
};

} // namespace Codethink::lvtqtw

#endif
