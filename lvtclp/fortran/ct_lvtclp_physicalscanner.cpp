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

#include <ct_lvtmdb_componentobject.h>
#include <ct_lvtmdb_packageobject.h>
#include <flang/Frontend/CompilerInstance.h>
#include <fortran/ct_lvtclp_physicalscanner.h>

#include <filesystem>
#include <unordered_set>

namespace Codethink::lvtclp::fortran {

using namespace Fortran::parser;
const char *const NON_LAKOSIAN_GROUP_NAME = "non-lakosian group";

PhysicalParseAction::PhysicalParseAction(lvtmdb::ObjectStore& memDb): memDb(memDb)
{
}

lvtmdb::ComponentObject *addComponentForFile(lvtmdb::ObjectStore& memDb, std::filesystem::path const& filePath)
{
    lvtmdb::ComponentObject *currentComponent = nullptr;
    memDb.withRWLock([&]() {
        auto qName = filePath.parent_path().stem();

        // TODO: Proper package handling. Currently assume all Fortran packages are "non-lakosian"
        auto *grp = memDb.getOrAddPackage(
            /*qualifiedName=*/NON_LAKOSIAN_GROUP_NAME,
            /*name=*/NON_LAKOSIAN_GROUP_NAME,
            /*diskPath=*/"",
            /*parent=*/nullptr,
            /*repository=*/nullptr);
        auto *package = memDb.getOrAddPackage(
            /*qualifiedName=*/qName,
            /*name=*/qName,
            /*diskPath=*/"",
            /*parent=*/grp,
            /*repository=*/nullptr);
        auto component = memDb.getOrAddComponent(
            /*qualifiedName=*/qName.string() + "/" + filePath.stem().string(),
            /*name=*/filePath.stem(),
            /*package=*/package);
        auto *file = memDb.getOrAddFile(
            /*qualifiedName=*/filePath.string(),
            /*name=*/filePath.string(),
            /*isHeader=*/false,
            /*hash=*/"", // TODO: Properly generate hash, if ever necessary
            /*package=*/package,
            /*component=*/component);
        assert(file);

        component->withRWLock([&] {
            component->addFile(file);
        });
        package->withRWLock([&] {
            package->addComponent(component);
        });

        currentComponent = component;
    });
    assert(currentComponent);
    return currentComponent;
}

void PhysicalParseAction::executeAction()
{
    auto currentInputPath = std::filesystem::path{getCurrentFileOrBufferName().str()};
    auto currentComponent = addComponentForFile(memDb, currentInputPath);

    auto& allSources = getInstance().getAllCookedSources().allSources();
    auto processSourceFileFrom = [&](Provenance const& p) {
        auto srcFile = allSources.GetSourceFile(p);
        if (srcFile == nullptr) {
            return; // Doesn't have associated source file
        }

        auto dependencyPath = std::filesystem::path{srcFile->path()};
        auto targetComponent = addComponentForFile(memDb, dependencyPath);
        lvtmdb::ComponentObject::addDependency(currentComponent, targetComponent);
    };

    auto interval = *allSources.GetFirstFileProvenance();
    auto currProvenance = interval.start();
    auto alreadyProcessed = std::unordered_set<const SourceFile *>{};
    while (allSources.IsValid(currProvenance)) {
        auto provenance = interval.start();
        auto srcFile = allSources.GetSourceFile(provenance);

        if (!alreadyProcessed.contains(srcFile)) {
            processSourceFileFrom(provenance);
            alreadyProcessed.insert(srcFile);
        }

        // TODO: NextAfter doesn't properly get the next interval, just the next position
        //       with size 1. This means after the first interval, positions are incremented
        //       by 1, and thus this algorithm is extremely dummy.
        interval = interval.NextAfter();
        currProvenance = interval.start();
    }
}

} // namespace Codethink::lvtclp::fortran
