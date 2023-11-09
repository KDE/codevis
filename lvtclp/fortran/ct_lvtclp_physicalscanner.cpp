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
#include <flang/Frontend/CompilerInstance.h>
#include <fortran/ct_lvtclp_physicalscanner.h>

#include <filesystem>

namespace Codethink::lvtclp::fortran {

using namespace Fortran::parser;

PhysicalParseAction::PhysicalParseAction(lvtmdb::ObjectStore& memDb): memDb(memDb)
{
}

void PhysicalParseAction::executeAction()
{
    auto currentInputPath = std::filesystem::path{getCurrentFileOrBufferName().str()};
    lvtmdb::ComponentObject *currentComponent = nullptr;
    memDb.withRWLock([&]() {
        // TODO: Proper package handling (get package name instead of "Fortran")
        auto *package = memDb.getOrAddPackage(
            /*qualifiedName=*/"Fortran",
            /*name=*/"Fortran",
            /*diskPath=*/"",
            /*parent=*/nullptr,
            /*repository=*/nullptr);
        currentComponent = memDb.getOrAddComponent(
            /*qualifiedName=*/currentInputPath.stem(),
            /*name=*/currentInputPath.stem(),
            /*package=*/package);
        auto *file = memDb.getOrAddFile(
            /*qualifiedName=*/currentInputPath.string(),
            /*name=*/currentInputPath.string(),
            /*isHeader=*/false,
            /*hash=*/"", // TODO: Properly generate hash, if ever necessary
            /*package=*/package,
            /*component=*/currentComponent);
        (void) file;
        assert(file);
    });
    assert(currentComponent);

    auto& allSources = getInstance().getAllCookedSources().allSources();
    auto showSourceFileFrom = [&](Provenance const& p) {
        auto srcFile = allSources.GetSourceFile(p);
        if (srcFile == nullptr) {
            return; // Doesn't have associated source file
        }

        lvtmdb::ComponentObject *targetComponent = nullptr;
        memDb.withRWLock([&]() {
            // TODO: Proper package handling
            auto *package = memDb.getOrAddPackage(
                /*qualifiedName=*/"Fortran",
                /*name=*/"Fortran",
                /*diskPath=*/"",
                /*parent=*/nullptr,
                /*repository=*/nullptr);
            auto dependencyPath = std::filesystem::path{srcFile->path()};
            targetComponent = memDb.getOrAddComponent(
                /*qualifiedName=*/dependencyPath.stem(),
                /*name=*/dependencyPath.stem(),
                /*package=*/package);
        });
        assert(targetComponent);
        lvtmdb::ComponentObject::addDependency(currentComponent, targetComponent);
    };

    auto interval = *allSources.GetFirstFileProvenance();
    auto currProvenance = interval.start();
    while (allSources.IsValid(currProvenance)) {
        showSourceFileFrom(interval.start());
        // TODO: NextAfter doesn't properly get the next interval, just the next position
        //       with size 1. This means after the first interval, positions are incremented
        //       by 1, and thus this algorithm is extremely dummy.
        interval = interval.NextAfter();
        currProvenance = interval.start();
    }
}

} // namespace Codethink::lvtclp::fortran
