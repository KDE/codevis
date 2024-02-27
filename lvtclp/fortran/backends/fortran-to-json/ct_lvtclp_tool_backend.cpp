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

#include <fortran/ct_lvtclp_fortran_dbutils.h>
#include <fortran/ct_lvtclp_fortran_tool.h>

#include <ct_lvtmdb_fileobject.h>
#include <ct_lvtmdb_functionobject.h>

#include <clang/Tooling/JSONCompilationDatabase.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>

#include <iostream>

using namespace clang::tooling;

namespace {

using namespace Codethink::lvtclp::fortran;
using namespace Codethink::lvtmdb;

struct FortranParsingContext {
    std::string activeFile;
    ComponentObject *activeDbComponentObject;
    FunctionObject *activeFunction = nullptr;
};

ComponentObject *findComponentForFunction(ObjectStore& memDb, FunctionObject *f)
{
    // TODO: Create a "component()" method in the FileObject class to avoid the loop below.
    // TODO: Create a "file()" method in the FunctionObject class to avoid the loop below.
    for (auto const& maybeCalleeFile : memDb.files()) {
        auto const& functions = maybeCalleeFile.second->globalFunctions();
        if (std::find_if(functions.cbegin(),
                         functions.cend(),
                         [&f](auto const& f2) {
                             return f->qualifiedName() == f2->qualifiedName();
                         })
            != functions.cend()) {
            return maybeCalleeFile.second->component();
        }
    }
    return nullptr;
};

void recursiveParseJsonASTNode(QJsonObject const& jsonASTNode, FortranParsingContext const& context, ObjectStore& memDb)
{
    auto visitAllChildrenWithContext = [&](FortranParsingContext const& context) {
        for (auto const& k : jsonASTNode.keys()) {
            if (jsonASTNode[k].isArray()) {
                auto children = jsonASTNode[k].toArray();
                for (auto e : children) {
                    recursiveParseJsonASTNode(e.toObject(), context, memDb);
                }
            }
            if (jsonASTNode[k].isObject()) {
                recursiveParseJsonASTNode(jsonASTNode[k].toObject(), context, memDb);
            }
        }
    };

    if (!jsonASTNode.contains("tag")) {
        visitAllChildrenWithContext(context);
    }

    auto tag = jsonASTNode["tag"].toString();
    if (tag == "comment") {
        // Ignore comments.
        return;
    } else if (tag == "subroutine" || tag == "function") {
        auto functionName = jsonASTNode["name"].isString()
            ? jsonASTNode["name"].toString().toStdString()
            : jsonASTNode["name"]["value"]["value"].toString().toStdString();

        if (functionName.empty()) {
            std::cout << "++ WARNING: Unexpected empty function name. Ignoring!\n";
            std::cout << "++ [Debug] Tag='" << tag.toStdString() << "'\n";
            std::cout << "++ [Debug] ActiveFile='" << context.activeFile << "'\n";
            std::cout << "++ [Debug] Span='" << jsonASTNode["span"].toString().toStdString() << "'\n";
            return;
        }

        FunctionObject *function = nullptr;
        memDb.withRWLock([&]() {
            function = memDb.getOrAddFunction(
                /*qualifiedName=*/functionName,
                /*name=*/functionName,
                /*signature=*/"",
                /*returnType=*/"",
                /*templateParameters=*/"",
                /*parent=*/nullptr);
            auto *file = memDb.getFile(context.activeFile);
            file->withRWLock([&] {
                file->addGlobalFunction(function);
            });

            // It may be the case that the function was already known, but not it's definition.
            // It is necessary to search for the related component and make sure the dependencies are updated.
            for (auto const& callee : function->callees()) {
                auto calleeComponent = findComponentForFunction(memDb, callee);
                if (calleeComponent) {
                    recursiveAddComponentDependency(context.activeDbComponentObject, calleeComponent);
                }
            }
            for (auto const& caller : function->callers()) {
                auto callerComponent = findComponentForFunction(memDb, caller);
                if (callerComponent) {
                    recursiveAddComponentDependency(callerComponent, context.activeDbComponentObject);
                }
            }
        });
        auto subroutineContext = context;
        subroutineContext.activeFunction = function;
        visitAllChildrenWithContext(subroutineContext);
    } else if (tag == "call" || tag == "function_call") {
        if (context.activeFunction == nullptr) {
            std::cout << "++ WARNING: found a function call without caller context. Will skip.\n";
            std::cout << "++ [Debug] Tag='" << tag.toStdString() << "'\n";
            std::cout << "++ [Debug] ActiveFile='" << context.activeFile << "'\n";
            std::cout << "++ [Debug] Span='" << jsonASTNode["span"].toString().toStdString() << "'\n";
            return;
        }
        auto calleeName = jsonASTNode["function"]["value"]["value"].toString().toStdString();

        memDb.withRWLock([&]() {
            auto *callee = memDb.getOrAddFunction(
                /*qualifiedName=*/calleeName,
                /*name=*/calleeName,
                /*signature=*/"",
                /*returnType=*/"",
                /*templateParameters=*/"",
                /*parent=*/nullptr);
            FunctionObject::addDependency(context.activeFunction, callee);

            // If the component where the function is defined is known, then update the dependency graph
            auto calleeComponent = findComponentForFunction(memDb, callee);
            if (calleeComponent) {
                recursiveAddComponentDependency(context.activeDbComponentObject, calleeComponent);
            }
        });
        visitAllChildrenWithContext(context);
    } else if (tag == "include") {
        auto inclusionPath = jsonASTNode["path"]["value"]["value"].toString().toStdString();

        // TODO: Search the inclusion taking in consideration the known inclusion list from compilation commands
        inclusionPath = "FortranIncludes/" + inclusionPath;

        auto inclusionComponentDbObject = addComponentForFile(memDb, inclusionPath);
        recursiveAddComponentDependency(context.activeDbComponentObject, inclusionComponentDbObject);

        // Update the context so that child `blocks` will be affected
        auto inclusionContext = context;
        inclusionContext.activeFile = inclusionPath;
        inclusionContext.activeDbComponentObject = inclusionComponentDbObject;
        visitAllChildrenWithContext(inclusionContext);
    } else {
        visitAllChildrenWithContext(context);
    }
}

QJsonDocument runFortranToJsonAndGetOutputs(std::string const& targetDirectory,
                                            std::string const& targetFile,
                                            std::vector<std::string> const& includePaths)
{
    static auto const FORTRAN_TO_JSON_EXECUTABLE = QString::fromStdString("fortran-to-json");

    auto *fortranToJsonTool = new QProcess{};
    auto jsonRawData = QString{};
    auto onFinish = [fortranToJsonTool, &jsonRawData](int exitCode, QProcess::ExitStatus) {
        if (exitCode != 0) {
            std::cout << "Unexpected exit code (exitCode='" << exitCode << "')\n";
            return;
        }
        jsonRawData = fortranToJsonTool->readAllStandardOutput();
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

    QObject::connect(fortranToJsonTool,
                     QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                     fortranToJsonTool,
                     onFinish);
    QObject::connect(fortranToJsonTool, &QProcess::errorOccurred, fortranToJsonTool, onErrorOccurred);

    fortranToJsonTool->setWorkingDirectory(QString::fromStdString(targetDirectory));
    auto args = QStringList({"-v77l", QString::fromStdString(targetFile)});
    args.append("-I" + QString::fromStdString(std::filesystem::path{targetFile}.parent_path().string()));
    args.append("-I" + QString::fromStdString(targetDirectory));
    for (auto const& includePath : includePaths) {
        args.append(QString::fromStdString(includePath));
    }

    std::cout << "++ Command: " << FORTRAN_TO_JSON_EXECUTABLE.toStdString() << " ";
    for (auto const& arg : args) {
        std::cout << arg.toStdString() << " ";
    }
    std::cout << "\n";

    fortranToJsonTool->start(FORTRAN_TO_JSON_EXECUTABLE, args);
    std::cout << "..." << std::flush;
    fortranToJsonTool->waitForFinished();
    std::cout << "!\n";

    delete fortranToJsonTool;
    return QJsonDocument::fromJson(jsonRawData.toUtf8());
}

void runFullOnCommand(CompileCommand const& cmd, ObjectStore& memDb)
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
    auto dbComponentObject = addComponentForFile(memDb, cmd.Filename);
    auto jsonAST = runFortranToJsonAndGetOutputs(cmd.Directory, cmd.Filename, includePaths);
    std::cout << "++ Meta info filename = " << jsonAST["meta_info"]["filename"].toString().toStdString() << "\n";
    if (!jsonAST.isEmpty() && jsonAST.isObject()) {
        auto context = FortranParsingContext{cmd.Filename, dbComponentObject};

        auto programArray = jsonAST["program"].toArray();
        for (auto e : programArray) {
            recursiveParseJsonASTNode(e.toObject(), context, memDb);
        }
    }
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
        runFullOnCommand(cmd, this->memDb());
    }
    return true;
}

} // namespace Codethink::lvtclp::fortran
