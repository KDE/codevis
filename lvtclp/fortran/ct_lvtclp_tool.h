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

#ifndef CODEVIS_CT_LVTCLP_TOOL_H
#define CODEVIS_CT_LVTCLP_TOOL_H

#include <clang/Tooling/CompilationDatabase.h>
#include <ct_lvtmdb_objectstore.h>
#include <filesystem>
#include <lvtclp_export.h>

namespace Codethink::lvtclp::fortran {

class LVTCLP_EXPORT Tool {
  public:
    explicit Tool(std::unique_ptr<clang::tooling::CompilationDatabase> compilationDatabase);

    static std::unique_ptr<Tool> fromCompileCommands(std::filesystem::path const& compileCommandsJson);

    bool runPhysical(bool skipScan = false);
    bool runFull(bool skipPhysical = false);

    lvtmdb::ObjectStore& getObjectStore();
    void setSharedMemDb(std::shared_ptr<lvtmdb::ObjectStore> const& sharedMemDb);

  private:
    std::unique_ptr<clang::tooling::CompilationDatabase> compilationDatabase;

    // The tool can be used either with a local memory database or with a
    // shared one. Only one can be used at a time. The default is to use
    // localMemDb. If setMemDb(other) is called, will ignore the local one.
    lvtmdb::ObjectStore localMemDb;
    std::shared_ptr<lvtmdb::ObjectStore> sharedMemDb = nullptr;
    [[nodiscard]] inline lvtmdb::ObjectStore& memDb()
    {
        return this->sharedMemDb ? *this->sharedMemDb : this->localMemDb;
    }
};

} // namespace Codethink::lvtclp::fortran

#endif // CODEVIS_CT_LVTCLP_TOOL_H
