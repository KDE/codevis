/*
 * // Copyright 2023 Codethink Ltd <codethink@codethink.co.uk>
 * // SPDX-License-Identifier: Apache-2.0
 * //
 * // Licensed under the Apache License, Version 2.0 (the "License");
 * // you may not use this file except in compliance with the License.
 * // You may obtain a copy of the License at
 * //
 * //     http://www.apache.org/licenses/LICENSE-2.0
 * //
 * // Unless required by applicable law or agreed to in writing, software
 * // distributed under the License is distributed on an "AS IS" BASIS,
 * // WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * // See the License for the specific language governing permissions and
 * // limitations under the License.
 */

#ifndef MERGE_PROJECT_DATABASES_H
#define MERGE_PROJECT_DATABASES_H

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include <result/result.hpp>

#include <codevis_project_helpers_export.h>

namespace Codethink {

class CODEVIS_PROJECT_HELPERS_EXPORT MergeProjects {
  public:
    enum class MergeProjectErrorEnum {
        NoError,
        Error,
        RemoveDatabaseError,
        DatabaseAccessError,
        OpenProjectError,
        InvalidFileExtensionError,
    };
    struct MergeProjectError {
        const MergeProjectErrorEnum errorVal;
        const std::string what;
    };

    using MergeDbProgressReportCallback = std::function<void(int idx, int database_size, const std::string& db_name)>;

    static cpp::result<void, MergeProjectError>
    mergeDatabases(std::vector<std::filesystem::path> project_or_databases,
                   std::filesystem::path resultingProject,
                   std::optional<MergeDbProgressReportCallback> progressReportCallback);
};

} // namespace Codethink
#endif
