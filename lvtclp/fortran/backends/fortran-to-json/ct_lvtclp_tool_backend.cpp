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

#include <fortran/ct_lvtclp_tool.h>

#include <clang/Tooling/JSONCompilationDatabase.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>

#include <iostream>

using namespace clang::tooling;

namespace {

void recursiveParseJsonASTNode(QJsonObject const& jsonASTNode)
{
    if (jsonASTNode.contains("tag")) {
        auto tag = jsonASTNode["tag"].toString();
        if (tag == "subroutine") {
            std::cout << "!> Found subroutine " << jsonASTNode["name"].toString().toStdString() << "\n";
        } else if (tag == "call") {
            std::cout << "!> Found CALL (unhandled)\n";
        } else if (tag == "include") {
            std::cout << "!> Found INCLUDE (unhandled)\n";
        } else if (tag == "statement") {
            recursiveParseJsonASTNode(jsonASTNode["statement"].toObject());
        }
    }

    if (jsonASTNode.contains("blocks")) {
        auto children = jsonASTNode["blocks"].toArray();
        for (auto e : children) {
            recursiveParseJsonASTNode(e.toObject());
        }
    }
}

QJsonDocument runFortranToJsonAndGetOutputs(std::string const& targetDirectory,
                                            std::string const& targetFile,
                                            std::vector<std::string> const& includePaths)
{
    static auto const FORTRAN_TO_JSON_EXECUTABLE = "fortran-to-json";

    auto fortranToJsonTool = QProcess();
    auto jsonRawData = QString{};
    auto onFinish = [&fortranToJsonTool, &jsonRawData](int exitCode, QProcess::ExitStatus) {
        if (exitCode != 0) {
            std::cout << "Unexpected exit code (exitCode='" << exitCode << "')\n";
            return;
        }
        jsonRawData = fortranToJsonTool.readAllStandardOutput();
    };

    auto onErrorOccurred = [](QProcess::ProcessError error) {
        switch (error) {
        case QProcess::ProcessError::FailedToStart:
            std::cout << "Fortran to json executable not found! Giving up.\n";
            return;
        case QProcess::ProcessError::Crashed:
            std::cout << "Subprocess crashed.\n";
            return;
        case QProcess::ProcessError::Timedout:
            std::cout << "Timeout reached - Will not persist all data.\n";
            return;
        case QProcess::ProcessError::ReadError:
            std::cout << "Unhandled 'ReadError'\n";
            return;
        case QProcess::ProcessError::WriteError:
            std::cout << "Unhandled 'WriteError'\n";
            return;
        case QProcess::ProcessError::UnknownError:
            std::cout << "Unknown error\n";
            return;
        }
    };

    QObject::connect(&fortranToJsonTool, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), onFinish);
    QObject::connect(&fortranToJsonTool, &QProcess::errorOccurred, onErrorOccurred);

    fortranToJsonTool.setWorkingDirectory(QString::fromStdString(targetDirectory));
    auto args = QStringList({"-v77l", QString::fromStdString(targetFile)});
    for (auto const& includePath : includePaths) {
        args.append(QString::fromStdString(includePath));
    }
    fortranToJsonTool.start(FORTRAN_TO_JSON_EXECUTABLE, args);
    fortranToJsonTool.waitForFinished();

    return QJsonDocument::fromJson(jsonRawData.toUtf8());
}

void runFullOnCommand(CompileCommand const& cmd)
{
    auto ext = std::filesystem::path{cmd.Filename}.extension().string();
    if (ext != ".f" && ext != ".for" && ext != ".f90" && ext != ".inc") {
        return;
    }

    auto includePaths = std::vector<std::string>{};
    for (auto const& cmdLine : cmd.CommandLine) {
        if (cmdLine.starts_with("-I")) {
            includePaths.push_back(cmdLine.c_str());
        }
    }

    std::cout << "+ " << cmd.Filename << "\n";
    auto jsonAST = runFortranToJsonAndGetOutputs(cmd.Directory, cmd.Filename, includePaths);
    if (!jsonAST.isEmpty() && jsonAST.isObject()) {
        auto programArray = jsonAST["program"].toArray();
        for (auto e : programArray) {
            recursiveParseJsonASTNode(e.toObject());
        }
    }
    std::cout << "++ " << jsonAST["meta_info"]["filename"].toString().toStdString();
}
} // namespace

namespace Codethink::lvtclp::fortran {

bool Tool::runPhysical(bool skipScan)
{
    // Currently only runFull is supported.
    return runFull();
}

bool Tool::runFull(bool skipPhysical)
{
    for (auto const& cmd : this->compilationDatabase->getAllCompileCommands()) {
        runFullOnCommand(cmd);
    }
    return true;
}

} // namespace Codethink::lvtclp::fortran
