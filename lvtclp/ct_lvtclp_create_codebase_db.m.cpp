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
#include <ct_lvtclp_tool.h>
#ifdef CT_ENABLE_FORTRAN_SCANNER
#include <fortran/ct_lvtclp_tool.h>
#endif

#include <clang/Tooling/JSONCompilationDatabase.h>
#include <ct_lvtmdb_functionobject.h>
#include <ct_lvtmdb_soci_reader.h>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
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
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
#include <QStringList>

#include <ct_lvtmdb_objectstore.h>
#include <ct_lvtmdb_soci_writer.h>

#pragma push_macro("slots")
#undef slots
#include <pybind11/embed.h>
#include <pybind11/pybind11.h>
#pragma pop_macro("slots")

namespace py = pybind11;
struct PyDefaultGilReleasedContext {
    py::scoped_interpreter pyInterp;
    py::gil_scoped_release pyGilDefaultReleased;
};

namespace {

struct CommandLineArgs {
    std::filesystem::path sourcePath;
    std::filesystem::path dbPath;
    std::vector<std::filesystem::path> compilationDbPaths;
    QJsonDocument compilationCommand;
    unsigned numThreads = 1;
    std::vector<std::string> ignoreList;
    std::vector<std::pair<std::string, std::string>> packageMappings;
    std::vector<std::filesystem::path> nonLakosianDirs;
    bool update = false;
    bool physicalOnly = false;
    bool silent = false;
    Codethink::lvtclp::Tool::UseSystemHeaders useSystemHeaders = Codethink::lvtclp::Tool::UseSystemHeaders::e_Query;
};

enum class CommandLineParseResult {
    Ok,
    Error,
    Help,
    Query,
};

CommandLineParseResult parseCommandLine(QCommandLineParser& parser, CommandLineArgs& args, std::string& errorMessage)
{
    parser.setApplicationDescription("Build code database");

    const QCommandLineOption outputFile({"output", "o"}, "Output database file", "OUTPUT_FILE", "");
    const QCommandLineOption compileCommandsJson(
        "compile-commands-json",
        "Path to the compile_commands.json file from cmake. incompatible with compile-command option.",
        "COMPILE_COMMANDS",
        "");
    const QCommandLineOption compileCommand(
        "compile-command",
        "A single json-object of the compile_commands.json file, generating a single "
        "object file. Incompatible with compile-commands-json option."
        "It must contain the keys directory, command, file and output.",
        "COMPILE_COMMANDS_OBJ",
        "");

    const QCommandLineOption helpOption = parser.addHelpOption();
    const QCommandLineOption sourcePath("source-path", "Path for source code", "SOURCE_PATH", "");
    const QCommandLineOption numThreads("j", "Number of threads to use", "NUM_THREADS", "1");
    const QCommandLineOption ignoreList(
        "ignore",
        "Ignore file paths matching this glob pattern. This may be specified more than once.",
        "IGNORES",
        {"*.t.cpp"});
    const QCommandLineOption pkgmap("pkgmap",
                                    "Maps regex of places to package names so that they have meaningful packages in "
                                    "the generated database. e.g.: \"/llvm*/\":\"LLVM\"",
                                    "PKGMAP",
                                    {});
    const QCommandLineOption nonlakosianDirs("non-lakosian",
                                             "Treat any package inside this directory as part of the \"non-lakosian\" "
                                             "group. This may be specified more than once.",
                                             "NON-LAKOSIANS",
                                             {});
    const QCommandLineOption update("update", "updates an existing database file");
    const QCommandLineOption replace("replace", "replaces an existing database file");
    const QCommandLineOption physicalOnly("physical-only", "Only look for physical entities and relationships");
    const QCommandLineOption silent("silent", "supress stdout");
    const QCommandLineOption queryHeaders(
        "query-system-headers",
        "Query if we need system headers. the return code will be 0 for no and 1 for yes.");

    const QCommandLineOption useSystemHeaders(
        {QStringLiteral("use-system-headers")},
        "Asks clang to look for system headers. Must be checked beforehand with the --query-system-headers call. "
        "defaults to `Query`, meaning that we don't know if we have the system headers and we will look for them in "
        "the system. Possible values are yes/no/query",
        "USE_SYSTEM_HEADERS",
        "query");

    parser.addOptions({outputFile,
                       compileCommandsJson,
                       compileCommand,
                       sourcePath,
                       numThreads,
                       ignoreList,
                       pkgmap,
                       nonlakosianDirs,
                       update,
                       replace,
                       physicalOnly,
                       silent,
                       queryHeaders,
                       useSystemHeaders});

    if (!parser.parse(QCoreApplication::arguments())) {
        errorMessage = qPrintable(parser.errorText());
        return CommandLineParseResult::Error;
    }

    if (parser.isSet(helpOption)) {
        return CommandLineParseResult::Help;
    }

    if (parser.isSet(queryHeaders)) {
        return CommandLineParseResult::Query;
    }

    args.dbPath = parser.value(outputFile).toStdString();
    const auto compileCommands = parser.values(compileCommandsJson);

    args.compilationDbPaths.reserve(compileCommands.size());
    std::transform(compileCommands.begin(),
                   compileCommands.end(),
                   std::back_inserter(args.compilationDbPaths),
                   [](const QString& str) {
                       return std::filesystem::path(str.toStdString());
                   });

    for (const std::filesystem::path& compDbPath : args.compilationDbPaths) {
        if (!std::filesystem::is_regular_file(compDbPath)) {
            errorMessage = compDbPath.string() + " is not a file";
            return CommandLineParseResult::Error;
        }
    }

    // number of threads
    QString numThreadsStr = parser.value(numThreads);
    bool convOk;
    args.numThreads = numThreadsStr.toUInt(&convOk);
    if (!convOk) {
        errorMessage = '\'' + numThreadsStr.toStdString() + "' is not an unsigned integer";
        return CommandLineParseResult::Error;
    }
    if (args.numThreads < 1) {
        args.numThreads = 1;
    }

    // ignore list
    QStringList ignores = parser.values(ignoreList);
    std::transform(ignores.begin(), ignores.end(), std::back_inserter(args.ignoreList), [](const QString& ignore) {
        return ignore.toStdString();
    });

    // Package mapping
    QStringList pkgmaps = parser.values(pkgmap);
    for (auto const& p : pkgmaps) {
        auto mapAsString = p.toStdString();
        auto separatorPos = mapAsString.find(':');
        if (separatorPos == std::string::npos) {
            errorMessage =
                "Unexpected package mapping: '" + mapAsString + R"('. Expected format = "path_regex":"PkgName".)";
            return CommandLineParseResult::Error;
        }

        auto pathRegex = mapAsString.substr(0, separatorPos);
        auto pkgName = mapAsString.substr(separatorPos + 1, mapAsString.size());
        args.packageMappings.emplace_back(std::make_pair(pathRegex, pkgName));
    }

    // non-lakosian dirs
    QStringList nonlakosians = parser.values(nonlakosianDirs);
    std::transform(nonlakosians.begin(),
                   nonlakosians.end(),
                   std::back_inserter(args.nonLakosianDirs),
                   [](const QString& dir) {
                       return dir.toStdString();
                   });

    // incremental update
    args.update = parser.isSet(update);
    if (args.update && parser.isSet(replace)) {
        errorMessage = "--update and --replace cannot be set together";
        return CommandLineParseResult::Error;
    }

    if (std::filesystem::is_regular_file(args.dbPath)) {
        if (parser.isSet(replace)) {
            if (!std::filesystem::remove(args.dbPath)) {
                errorMessage = "Error removing " + args.dbPath.string();
                return CommandLineParseResult::Error;
            }
        } else if (!args.update) {
            // database exists but we aren't overwriting
            errorMessage = args.dbPath.string() + " already exists. Try --update or --replace";
            return CommandLineParseResult::Error;
        }
    }

    if (parser.isSet(compileCommand)) {
        QJsonParseError possibleError{};
        args.compilationCommand = QJsonDocument::fromJson(parser.value(compileCommand).toLocal8Bit(), &possibleError);
        if (possibleError.error != QJsonParseError::NoError) {
            errorMessage = possibleError.errorString().toStdString();
            return CommandLineParseResult::Error;
        }
    }

    if (!parser.isSet(useSystemHeaders)) {
        args.useSystemHeaders = Codethink::lvtclp::Tool::UseSystemHeaders::e_Query;
    } else {
        const QString val = parser.value(useSystemHeaders).toLower();
        args.useSystemHeaders = val == "yes" ? Codethink::lvtclp::Tool::UseSystemHeaders::e_Yes
            : val == "no"                    ? Codethink::lvtclp::Tool::UseSystemHeaders::e_No
                                             : Codethink::lvtclp::Tool::UseSystemHeaders::e_Query;
    }

    // physicalOnly
    args.physicalOnly = parser.isSet(physicalOnly);
    args.silent = parser.isSet(silent);
    args.sourcePath = parser.value(sourcePath).toStdString();

    return CommandLineParseResult::Ok;
}

} // namespace

static void setupPath(char **argv)
{
    const std::filesystem::path argv0(argv[0]);
    const std::filesystem::path appimagePath = argv0.parent_path();

    qputenv("CT_LVT_BINDIR", QByteArray::fromStdString(appimagePath.string()));
}

cpp::result<clang::tooling::CompileCommand, std::string> fromJson(const QJsonDocument& doc)
{
    const auto obj = doc.object();

    // validate the keys;
    if (!obj.contains("directory")) {
        return cpp::fail("Missing directory entry on the json field");
    }
    if (!obj.contains("command")) {
        return cpp::fail("Missing command entry on the json field");
    }
    if (!obj.contains("file")) {
        return cpp::fail("Missing file entry on the json field");
    }
    if (!obj.contains("output")) {
        return cpp::fail("Missing output entry on the json field");
    }

    std::string dir = obj["directory"].toString().toStdString();
    std::string file = obj["file"].toString().toStdString();
    std::string output = obj["output"].toString().toStdString();

#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    QStringList commands = obj["command"].toString().split(" ", Qt::SkipEmptyParts);
#else
    QStringList commands = obj["command"].toString().split(" ", QString::SkipEmptyParts);
#endif

    std::vector<std::string> commandLine;
    for (const auto& command : commands) {
        commandLine.push_back(command.toStdString());
    }

    clang::tooling::CompileCommand cmd;
    cmd.Directory = dir;
    cmd.CommandLine = commandLine;
    cmd.Filename = file;
    cmd.Output = output;

    return cmd;
}

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

    setupPath(argv);
    PyDefaultGilReleasedContext _pyDefaultGilReleasedContext;

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
    case CommandLineParseResult::Query:
        const bool useSystemHeaders = Codethink::lvtclp::CompilerUtil::weNeedSystemHeaders();
        std::cout << "We need system headers? " << (useSystemHeaders ? "yes" : "no") << "\n";
        return useSystemHeaders ? 1 : 0;
    }

    // using QString for the `.endsWith`.
    // TODO: remove this when we move to c++20.
    const QString dbPath = QString::fromStdString(args.dbPath.string());
    if (!dbPath.endsWith(".db")) {
        args.dbPath += ".db";
    }
    if (!args.sourcePath.empty()) {
        if (!exists(args.sourcePath)) {
            std::cerr << "Given source path doesn't exist: '" << args.sourcePath.string() << "'\n";
            return EXIT_FAILURE;
        }
        args.sourcePath = std::filesystem::canonical(args.sourcePath).string();
    }

    if (!args.packageMappings.empty()) {
        std::cout << "Using the following mapping regexes:\n";
        for (auto const& [k, v] : args.packageMappings) {
            std::cout << "- " << k << " => " << v << "\n";
        }
    }

    if (args.compilationCommand.isObject() && !args.compilationDbPaths.empty()) {
        std::cerr << "Choose only a compile command or the compile-commands.json file.\n";
        return EXIT_FAILURE;
    }

    if (args.compilationCommand.isObject() && args.numThreads != 1) {
        std::cout << "Multiple threads are ignored in this run.\n";
        return EXIT_FAILURE;
    }

    auto compileCommand = fromJson(args.compilationCommand);
    if (compileCommand.has_error() && args.compilationDbPaths.empty()) {
        std::cerr << "Invalid compile commands passed.\n";
        return EXIT_FAILURE;
    }

    auto clang_tool = !args.compilationDbPaths.empty()
        ? std::make_unique<Codethink::lvtclp::Tool>(args.sourcePath,
                                                    args.compilationDbPaths,
                                                    args.dbPath,
                                                    args.numThreads,
                                                    args.ignoreList,
                                                    args.nonLakosianDirs,
                                                    args.packageMappings,
                                                    !args.silent)
        : std::make_unique<Codethink::lvtclp::Tool>(args.sourcePath,
                                                    compileCommand.value(),
                                                    args.dbPath,
                                                    args.ignoreList,
                                                    args.nonLakosianDirs,
                                                    args.packageMappings,
                                                    !args.silent);

    clang_tool->setUseSystemHeaders(args.useSystemHeaders);

#ifdef CT_ENABLE_FORTRAN_SCANNER
    auto flang_tool = std::make_unique<Codethink::lvtclp::fortran::Tool>(args.compilationDbPaths[0]);
    const bool success = [&]() {
        if (args.physicalOnly) {
            auto clang_result = clang_tool->runPhysical();
            auto flang_result = flang_tool->runPhysical();
            return clang_result && flang_result;
        }
        auto clang_result = clang_tool->runFull();
        auto flang_result = flang_tool->runFull();
        return clang_result && flang_result;
    }();
#else
    const bool success = [&]() {
        if (args.physicalOnly) {
            auto clang_result = clang_tool->runPhysical();
            return clang_result;
        }
        auto clang_result = clang_tool->runFull();
        return clang_result;
    }();
#endif

    if (!success) {
        std::cerr << "Error generating database\n";
        return EXIT_FAILURE;
    }

    // Currently the call to `tool->runPhysical` and `tool->runFull` are
    // already saving data to a DB, and because of that acessing the DataWiter
    // as we are doing right now is way more slow than it should.
    // The database stored by the tool is in memory, and we need to dump
    // to disk, so we can ignore it - and use the DataWriter to fetch
    // information from the tool and dump *that* info to disk.
    {
        Codethink::lvtmdb::SociWriter writer;
        if (!writer.createOrOpen(args.dbPath.string())) {
            std::cerr << "Error saving database file to disk\n";
            return EXIT_FAILURE;
        }
        {
            auto& mdb = clang_tool->getObjectStore();
            mdb.writeToDatabase(writer);
        }
#ifdef CT_ENABLE_FORTRAN_SCANNER
        {
            auto& mdb = flang_tool->getObjectStore();
            mdb.writeToDatabase(writer);
        }
#endif
    }
    {
        using namespace Codethink;
        auto path = args.dbPath.string();

        // Heuristically find bindings from/to C/Fortran code
        // TODO: Code duplicated with lvtqtw/ct_lvtqtw_parse_codebase.cpp - Move to somewhere else.
        // TODO: Check where this should be moved to.

        // TODO: We don't have a proper merge, so we need to get the merged data from disk.
        lvtmdb::ObjectStore mergedStore;
        {
            lvtmdb::SociReader reader;
            mergedStore.readFromDatabase(reader, path).expect("Unexpected error loading database.");
        }

        auto& all_functions = mergedStore.functions();
        auto bindDependencies = std::vector<std::pair<lvtmdb::FunctionObject *, lvtmdb::FunctionObject *>>{};
        for (auto const& [_, ownedCFuncObject] : all_functions) {
            auto *cFunc = ownedCFuncObject.get();
            auto cFuncLock = cFunc->readOnlyLock();
            if (!cFunc->name().ends_with('_')) {
                continue;
            }

            // Check for possible Fortran binding
            // TODO: Only take in consideration Fortran functions. There may be a C function that matches the rules
            //       below, and the output would be wrong.
            auto fortranName = cFunc->name().substr(0, cFunc->name().size() - 1);
            auto it = std::find_if(std::cbegin(all_functions), std::cend(all_functions), [&fortranName](auto const& f) {
                return f.second->name() == fortranName;
            });
            if (it == std::end(all_functions)) {
                continue; // Couldn't find.
            }

            auto *fortranFunc = it->second.get();
            auto fortranFuncLock = fortranFunc->readOnlyLock();

            bindDependencies.emplace_back(std::make_pair(cFunc, fortranFunc));
        }

        mergedStore.withRWLock([&]() {
            for (auto& [from, to] : bindDependencies) {
                lvtmdb::FunctionObject::addDependency(from, to);
            }
        });
        lvtmdb::SociWriter writer;
        writer.createOrOpen(path);
        mergedStore.writeToDatabase(writer);
    }

    return EXIT_SUCCESS;
}
