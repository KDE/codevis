// ct_lvtclp_componentutil.cpp                                        -*-C++-*-

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

#include <ct_lvtclp_componentutil.h>

#include <ct_lvtmdb_componentobject.h>
#include <ct_lvtmdb_objectstore.h>
#include <ct_lvtmdb_packageobject.h>

#include <filesystem>
#include <memory>

namespace Codethink::lvtclp {

lvtmdb::ComponentObject *ComponentUtil::addComponent(const std::filesystem::path& filePath,
                                                     lvtmdb::PackageObject *parent,
                                                     lvtmdb::ObjectStore& memDb)
// add or look up component for file
{
    // special case for a mysterious file called "" that clang invents:
    if (filePath.empty()) {
        return nullptr;
    }

    const std::string name = filePath.stem().string();
    const std::string qualifiedName = [&]() {
        // Temporary solution to recognize lakosian packages
        {
            auto parentQualName = std::string{};
            parent->withROLock([&]() {
                parentQualName = parent->qualifiedName();
            });

            auto contains = [](std::string const& str1, std::string const& str2) {
                return str1.find(str2) != std::string::npos;
            };

            if (contains(parentQualName, "standalone/") || contains(parentQualName, "groups/")) {
                return parentQualName + "/" + filePath.stem().string();
            }
        }

        return (filePath.parent_path() / name).generic_string();
    }();

    return memDb.getOrAddComponent(qualifiedName, name, parent);
}

} // namespace Codethink::lvtclp
