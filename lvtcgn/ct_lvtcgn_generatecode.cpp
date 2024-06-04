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

#include <QFile>
#include <QJSEngine>
#include <QTextStream>

namespace Codethink::lvtcgn::mdl {

IPhysicalEntityInfo::~IPhysicalEntityInfo() = default;
ICodeGenerationDataProvider::~ICodeGenerationDataProvider() = default;

cpp::result<void, CodeGenerationError>
CodeGeneration::generateCodeFromjS(const QString& scriptPath,
                                   const QString& outputDir,
                                   ICodeGenerationDataProvider& dataProvider,
                                   std::optional<std::function<void(CodeGenerationStep const&)>> callback)
{
    QJSEngine myEngine;

    // Load Needed Modules:
    QJSValue _module = myEngine.importModule("./math.mjs");
    QJSValue beforeProcessing = _module.property("beforeProcessEntities");
    QJSValue buildPhysicalEntity = _module.property("buildPhysicalEntity");
    QJSValue afterProcessing = _module.property("afterProcessEntities");

    if (buildPhysicalEntity.isUndefined()) {
        return cpp::fail(CodeGenerationError{CodeGenerationError::Kind::ScriptDefinitionError,
                                             "Expected function named buildPhysicalEntity"});
    }

    if (!buildPhysicalEntity.isCallable()) {
        return cpp::fail(CodeGenerationError{CodeGenerationError::Kind::ScriptDefinitionError,
                                             "Expected function named buildPhysicalEntity"});
    }

    if (beforeProcessing.isCallable()) {
        beforeProcessing.call({QJSValue(outputDir)});
    }

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
            QJSValue _entity = myEngine.newQObject(new QObject());

            buildPhysicalEntity.call({outputDir, _entity});
            auto children = entity->children();
            recursiveBuild(children);
        }
    };

    recursiveBuild(dataProvider.topLevelEntities());

    if (afterProcessing.isCallable()) {
        afterProcessing.call({QJSValue(outputDir)});
    }

    return {};
}

} // namespace Codethink::lvtcgn::mdl
