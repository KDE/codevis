// ct_lvtcgn_generatecode.cpp                                       -*-C++-*-

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

#include <ct_lvtcgn_generatecode.h>

#include <ct_lvtcgn_app_adapter.h>
#include <ct_lvtcgn_js_file_wrapper.h>

#include <QFile>
#include <QJSEngine>
#include <QTextStream>
#include <QtGlobal>
#include <qjsengine.h>

// the Q_*_RESOURCE calls can't be called inside of a namespace.'
void initResource()
{
    Q_INIT_RESOURCE(codegen);
}
void cleanupResource()
{
    Q_INIT_RESOURCE(codegen);
}

namespace Codethink::lvtcgn::mdl {

IPhysicalEntityInfo::~IPhysicalEntityInfo() = default;
ICodeGenerationDataProvider::~ICodeGenerationDataProvider() = default;

namespace {
void printError(QJSValue& v)
{
    qDebug() << "result: " << v.property("lineNumber").toInt() << ":" << v.toString();
}
} // namespace

cpp::result<void, CodeGenerationError>
CodeGeneration::generateCodeFromjS(const QString& scriptPath,
                                   const QString& outputDir,
                                   ICodeGenerationDataProvider& dataProvider,
                                   std::optional<std::function<void(CodeGenerationStep const&)>> callback)
{
    initResource();

    QFile f(scriptPath);
    QJSEngine myEngine;

    myEngine.installExtensions(QJSEngine::ConsoleExtension);
    // import our custom classes to handle IO.
    QJSValue jsMetaObject = myEngine.newQMetaObject(&FileIO::staticMetaObject);
    myEngine.globalObject().setProperty("FileIO", jsMetaObject);

    // Load Needed Modules:
    QJSValue _userModule = myEngine.importModule(scriptPath);
    if (_userModule.isError()) {
        printError(_userModule);
        return cpp::fail(CodeGenerationError{CodeGenerationError::Kind::ScriptDefinitionError,
                                             "Error Loading Code Generation Script"});
    }

    QFile ejs(":/codegen/ejs.min.js");
    ejs.open(QIODevice::ReadOnly);
    const QString data = ejs.readAll();
    auto _ejsModule = myEngine.evaluate(data, "ejs.min.js");
    if (_ejsModule.isError()) {
        printError(_ejsModule);
        return cpp::fail(
            CodeGenerationError{CodeGenerationError::Kind::ScriptDefinitionError, "Error Loading Template Engine"});
    }

    QJSValue beforeProcessing = _userModule.property("beforeProcessEntities");
    QJSValue buildPhysicalEntity = _userModule.property("buildPhysicalEntity");
    QJSValue afterProcessing = _userModule.property("afterProcessEntities");

    if (!buildPhysicalEntity.isCallable()) {
        return cpp::fail(CodeGenerationError{CodeGenerationError::Kind::ScriptDefinitionError,
                                             "Expected function named buildPhysicalEntity"});
    }

    if (!beforeProcessing.isCallable()) {
        return cpp::fail(CodeGenerationError{CodeGenerationError::Kind::ScriptDefinitionError,
                                             "Error: beforeProcessEntities is not a function."});
    }

    if (!afterProcessing.isCallable()) {
        return cpp::fail(CodeGenerationError{CodeGenerationError::Kind::ScriptDefinitionError,
                                             "Error: afterProcessing is not a function."});
    }

    auto res = beforeProcessing.call({QJSValue(outputDir)});
    using InfoVec = QVector<IPhysicalEntityInfo *>;
    std::function<void(InfoVec const&)> recursiveBuild = [&](InfoVec const& entities) -> void {
        for (auto *entity : entities) {
            if (!entity->selectedForCodeGeneration()) {
                continue;
            }
            if (callback) {
                (*callback)(ProcessEntityStep{entity->name()});
            }

            QJSValue _output(outputDir);
            QJSValue _entity = myEngine.newQObject(entity);
            myEngine.setObjectOwnership(entity, QJSEngine::ObjectOwnership::CppOwnership);
            auto res = buildPhysicalEntity.call({_entity, outputDir});
            auto children = entity->children();
            recursiveBuild(children);
            qDebug() << "Recursive Loop";
        }
    };

    recursiveBuild(dataProvider.topLevelEntities());
    qDebug() << "After Loop";

    res = afterProcessing.call({QJSValue(outputDir)});
    qDebug() << "After Processing";

    cleanupResource();
    qDebug() << "After Cleaup Resources";

    return {};
}

} // namespace Codethink::lvtcgn::mdl
