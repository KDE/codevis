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

#include <fortran/ct_lvtclp_fortran_tool.h>

#include <clang/Tooling/JSONCompilationDatabase.h>

#include <iostream>
#include <memory>

using namespace clang::tooling;

namespace Codethink::lvtclp::fortran {

Tool::Tool(std::unique_ptr<CompilationDatabase> compilationDatabase):
    compilationDatabase(std::move(compilationDatabase))
{
}

std::unique_ptr<Tool> Tool::fromCompileCommands(std::filesystem::path const& compileCommandsJson)
{
    auto errorMessage = std::string{};
    auto jsonDb = JSONCompilationDatabase::loadFromFile(compileCommandsJson.string(),
                                                        errorMessage,
                                                        clang::tooling::JSONCommandLineSyntax::AutoDetect);
    // TODO: Proper error management
    if (!errorMessage.empty()) {
        std::cout << "Tool::fromCompileCommands error: " << errorMessage;
        return nullptr;
    }

    return std::make_unique<Tool>(std::move(jsonDb));
}

lvtmdb::ObjectStore& Tool::getObjectStore()
{
    return this->memDb();
}

void Tool::setSharedMemDb(std::shared_ptr<lvtmdb::ObjectStore> const& sharedMemDb)
{
    this->sharedMemDb = sharedMemDb;
}

} // namespace Codethink::lvtclp::fortran
