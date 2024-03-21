// ct_lvtprj_create_prj_from_db.m.cpp                               -*-C++-*--

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

#include <ct_lvtldr_nodestorage.h>
#include <ct_lvtprj_projectfile.h>

#include <cstdlib>
#include <filesystem>

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QString>
#include <QStringList>

using namespace Codethink::lvtprj;
using namespace Codethink::lvtldr;

namespace {

struct CommandLineArgs {
    std::filesystem::path sourcePath;
    std::filesystem::path dbPath;
    std::filesystem::path output;
};

enum class CommandLineParseResult {
    Ok,
    Error,
    Help,
};

CommandLineParseResult parseCommandLine(QCommandLineParser& parser, CommandLineArgs& args)
{
    parser.setApplicationDescription("Create a project file from a database file");
    parser.addPositionalArgument("DATABASE", "Path to the database (required)");
    parser.addPositionalArgument("OUTPUT", "Path to the output project file (required)");
    const QCommandLineOption helpOption = parser.addHelpOption();

    const QCommandLineOption sourcePath("source-path", "Path for source code", "SOURCE_PATH", "");
    parser.addOptions({sourcePath});

    if (!parser.parse(QCoreApplication::arguments())) {
        return CommandLineParseResult::Error;
    }

    if (parser.isSet(helpOption)) {
        return CommandLineParseResult::Help;
    }

    // positional arguments
    const QStringList posArgs = parser.positionalArguments();
    if (posArgs.size() < 2) {
        return CommandLineParseResult::Error;
    }
    args.dbPath = posArgs.at(0).toStdString();
    args.output = posArgs.at(1).toStdString();
    args.sourcePath = parser.value(sourcePath).toStdString();
    return CommandLineParseResult::Ok;
}

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QCommandLineParser parser;
    CommandLineArgs args;

    switch (parseCommandLine(parser, args)) {
    case CommandLineParseResult::Ok:
        break;
    case CommandLineParseResult::Error:
        parser.showHelp();
        Q_UNREACHABLE();
    case CommandLineParseResult::Help:
        parser.showHelp();
        Q_UNREACHABLE();
    }

    auto projectFile = ProjectFile{};
    projectFile.createEmpty().expect("Unexpected error preparing new project file");

    // TODO: Properly create project from Code Database file instead of manually copying them
    std::filesystem::remove(projectFile.codeDatabasePath());
    std::filesystem::remove(projectFile.cadDatabasePath());
    std::filesystem::copy(args.dbPath, projectFile.codeDatabasePath());
    // TODO: Create missing tables for CAD database
    std::filesystem::copy(args.dbPath, projectFile.cadDatabasePath());

    projectFile.saveAs(args.output, ProjectFile::BackupFileBehavior::Keep)
        .expect("Unexpected error saving the file to disk");

    return EXIT_SUCCESS;
}
