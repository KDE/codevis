// ct_lvtldr_physicalloader.m.cpp                                     -*-C++-*-

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

#include <ct_lvtldr_lakosiannode.h>
#include <ct_lvtldr_nodestorage.h>
#include <ct_lvtldr_physicalloader.h>

#include <ct_lvtshr_graphenums.h>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QString>

using namespace Codethink;

namespace {

struct CommandLineArgs {
    std::filesystem::path dbPath;
    lvtshr::DiagramType type = lvtshr::DiagramType::NoneType;
    std::string qualifiedName;
    bool fwdDeps = false;
    bool revDeps = false;
    bool extDeps = false;
};

enum class CommandLineParseResult {
    Ok,
    Error,
    Help,
};

CommandLineParseResult parseCommandLine(QCommandLineParser& parser, CommandLineArgs& args, std::string& errorMessage)
{
    parser.setApplicationDescription("Run physical loader");

    parser.addPositionalArgument("CODE_DATABASE", "Path to the code database (required)");

    const QCommandLineOption helpOption = parser.addHelpOption();

    const QCommandLineOption component("component", "Component graph to load", "QUALIFIED_NAME");
    const QCommandLineOption package("package", "Package graph to load", "QUALIFIED_NAME");
    const QCommandLineOption group("group", "Package group graph to load", "QUALIFIED_NAME");

    const QCommandLineOption fwdDeps("fwdDeps", "Enable forward dependencies");
    const QCommandLineOption revDeps("revDeps", "Enable reverse dependencies");
    const QCommandLineOption extDeps("extDeps", "Enable external dependencies");

    parser.addOptions({component, package, group, fwdDeps, revDeps, extDeps});

    if (!parser.parse(QCoreApplication::arguments())) {
        errorMessage = qPrintable(parser.errorText());
        return CommandLineParseResult::Error;
    }

    if (parser.isSet(helpOption)) {
        return CommandLineParseResult::Help;
    }

    // positional arguments
    const QStringList posArgs = parser.positionalArguments();
    if (posArgs.empty()) {
        errorMessage = "Missing CODE_DATABASE";
        return CommandLineParseResult::Error;
    }
    if (posArgs.size() > 1) {
        errorMessage = "Too many positional arguments";
        return CommandLineParseResult::Error;
    }

    args.dbPath = posArgs.at(0).toStdString();
    if (!std::filesystem::is_regular_file(args.dbPath)) {
        errorMessage = args.dbPath.string() + " is not a file";
        return CommandLineParseResult::Error;
    }

    // type + qualifiedName
    // we can only have one of --component, --package, --group
    if ((parser.isSet(component) && parser.isSet(package)) || (parser.isSet(component) && parser.isSet(group))
        || (parser.isSet(package) && parser.isSet(group))) {
        errorMessage = "Only one of --component, --package and --group can be given";
        return CommandLineParseResult::Error;
    }

    if (parser.isSet(component)) {
        args.type = lvtshr::DiagramType::ComponentType;
        args.qualifiedName = parser.value(component).toStdString();
    } else if (parser.isSet(package)) {
        args.type = lvtshr::DiagramType::PackageType;
        args.qualifiedName = parser.value(package).toStdString();
    } else if (parser.isSet(group)) {
        args.type = lvtshr::DiagramType::PackageType;
        args.qualifiedName = parser.value(group).toStdString();
    } else {
        errorMessage = "Please specify exactly one of --component, --package or --group";
        return CommandLineParseResult::Error;
    }

    args.fwdDeps = parser.isSet(fwdDeps);
    args.revDeps = parser.isSet(revDeps);
    args.extDeps = parser.isSet(extDeps);

    return CommandLineParseResult::Ok;
}

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    QCommandLineParser parser;
    CommandLineArgs args;
    std::string errorMessage;

    switch (parseCommandLine(parser, args, errorMessage)) {
    case CommandLineParseResult::Ok:
        break;
    case CommandLineParseResult::Error:
        if (!errorMessage.empty()) {
            std::cerr << errorMessage << std::endl;
            std::cerr << std::endl;
        }
        std::cerr << qPrintable(parser.helpText());
        return EXIT_FAILURE;
    case CommandLineParseResult::Help:
        parser.showHelp();
        Q_UNREACHABLE();
    }

    // TODO 695
    /*
    lvtcdb::CodebaseDb db;
    db.setPath(args.dbPath.string());
    if (!db.open(Codethink::lvtcdb::BaseDb::OpenType::ExistingDatabase)) {
        std::cerr << "Error opening code database " << args.dbPath << std::endl;
    }
*/

    lvtldr::NodeStorage storage;
    //    storage.setDatabaseSourcePath(db.path());

    lvtldr::PhysicalLoader loader(storage); // debug=true
    lvtldr::LakosianNode *node = storage.findByQualifiedName(args.type, args.qualifiedName);
    loader.clear();
    loader.setMainNode(node);
    loader.setExtDeps(args.extDeps);

    if (!loader.load(node, lvtldr::NodeLoadFlags{})) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
