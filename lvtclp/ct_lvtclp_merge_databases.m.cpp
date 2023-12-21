// ct_lvtclp_codebase_db.m.cpp                                      -*-C++-*--

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

#include <ct_lvtclp_compilerutil.h>
#include <ct_lvtmdb_objectstore.h>
#include <ct_lvtmdb_soci_reader.h>
#include <ct_lvtmdb_soci_writer.h>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include <QtGlobal>

#ifdef Q_OS_WINDOWS
#include <stdio.h>
#include <windows.h>
#else
#include <csignal>
#endif

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDebug>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
#include <QStringList>

namespace {

struct CommandLineArgs {
    QList<QString> databases;
    QString resultingDb;
    bool silent = false;
    bool force = false;
};

enum CommandLineParseResult { Ok, Error, Help };

CommandLineParseResult parseCommandLine(QCommandLineParser& parser, CommandLineArgs& args, std::string& errorMessage)
{
    parser.setApplicationDescription("Build code database");

    const QCommandLineOption outputFile({"output", "o"}, "Output database file", "OUTPUT_FILE", "");
    const QCommandLineOption databases("database",
                                       "Path to a single database to be merged. specify it multiple times.",
                                       "DATABASES",
                                       "");
    const QCommandLineOption silent("silent", "supress output");
    const QCommandLineOption force("force", "overwrite the output file if it exists.");
    const QCommandLineOption helpOption = parser.addHelpOption();

    parser.addOptions({outputFile, databases, force, silent, helpOption});

    if (!parser.parse(QCoreApplication::arguments())) {
        errorMessage = qPrintable(parser.errorText());
        return CommandLineParseResult::Error;
    }

    if (parser.isSet(force)) {
        args.force = true;
    }

    if (!parser.isSet(outputFile)) {
        errorMessage = "Missing output file";
        return CommandLineParseResult::Error;
    }

    if (!parser.isSet(databases)) {
        errorMessage = "Missing database parameter, please specify with --database <filename.db>";
        return CommandLineParseResult::Error;
    }

    const QString oFile = parser.value(outputFile);
    if (!oFile.endsWith("db") || oFile.size() <= 3) {
        errorMessage = "output database must be a in the format file.db";
        return CommandLineParseResult::Error;
    }

    if (parser.isSet(helpOption)) {
        return CommandLineParseResult::Help;
    }

    args.databases = parser.values(databases);
    for (const auto& db : std::as_const(args.databases)) {
        QFileInfo info(db);
        if (!info.exists()) {
            errorMessage = (db + " is not a file").toStdString();
        }
    }

    args.resultingDb = oFile;
    args.silent = parser.isSet("silent");

    return CommandLineParseResult::Ok;
}

} // namespace

#ifdef Q_OS_WINDOWS
BOOL WINAPI CtrlHandler(DWORD fdwCtrlType)
{
    switch (fdwCtrlType) {
    case CTRL_C_EVENT:
        exit(1);

    case CTRL_CLOSE_EVENT:
        exit(1);
    default:
        return false;
    }
    return false;
}
#else
void signal_callback_handler(int signum)
{
    exit(signum);
}
#endif

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
#ifdef Q_OS_WINDOWS
    SetConsoleCtrlHandler(CtrlHandler, TRUE);
#else
    (void) signal(SIGINT, signal_callback_handler);
#endif
    QCommandLineParser parser;
    CommandLineArgs args;
    std::string errorMessage;

    switch (parseCommandLine(parser, args, errorMessage)) {
    case CommandLineParseResult::Ok:
        break;
    case CommandLineParseResult::Error:
        if (!errorMessage.empty()) {
            std::cerr << errorMessage << "\n\n";
        }
        parser.showHelp();
        Q_UNREACHABLE();
    case CommandLineParseResult::Help:
        parser.showHelp();
        Q_UNREACHABLE();
    }

    std::filesystem::path outputDbPath = args.resultingDb.toStdString();
    if (!args.force && std::filesystem::exists(outputDbPath)) {
        qInfo() << "Resulting file already exists and --force was not set. not continuing.\n";
        return 1;
    }

    const int database_size = args.databases.count();
    int idx = 0;

    if (args.force) {
        if (std::filesystem::exists(outputDbPath)) {
            std::filesystem::remove(outputDbPath);
        }
    }

    for (const QString& db : std::as_const(args.databases)) {
        Codethink::lvtmdb::SociReader dbReader;
        Codethink::lvtmdb::SociWriter dbWriter;

        Codethink::lvtmdb::ObjectStore inMemoryDb;

        auto loaded = inMemoryDb.readFromDatabase(dbReader, db.toStdString());
        if (loaded.has_error()) {
            std::cout << "Error opening database " << loaded.error().what << "\n";
            return EXIT_FAILURE;
        }

        if (!args.silent) {
            idx += 1;
            std::cout << "[" << idx << " of " << database_size << ": Loading data from" << db.toStdString() << "\n";
        }

        if (!dbWriter.createOrOpen(outputDbPath.string())) {
            std::cerr << "Could not open resulting database\n";
            return EXIT_FAILURE;
        }

        inMemoryDb.writeToDatabase(dbWriter);
    }

    std::cout << "Database file saved correctly in " << outputDbPath << "\n";
    return EXIT_SUCCESS;
}
