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

#ifndef CODEVIS_CT_LVTCLP_FORTRAN_DBUTILS_H
#define CODEVIS_CT_LVTCLP_FORTRAN_DBUTILS_H

#include <ct_lvtmdb_componentobject.h>
#include <ct_lvtmdb_objectstore.h>
#include <ct_lvtmdb_packageobject.h>

#include <clang/Tooling/CompilationDatabase.h>

#include <filesystem>
#include <lvtclp_export.h>

namespace Codethink::lvtclp::fortran {

static const char *const NON_LAKOSIAN_GROUP_NAME = "non-lakosian group";

static lvtmdb::ComponentObject *addComponentForFile(lvtmdb::ObjectStore& memDb, std::filesystem::path const& filePath)
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

static void recursiveAddComponentDependency(lvtmdb::ComponentObject *sourceComponent,
                                            lvtmdb::ComponentObject *targetComponent)
{
    using namespace Codethink::lvtmdb;
    lvtmdb::ComponentObject::addDependency(sourceComponent, targetComponent);

    PackageObject *srcParent = nullptr;
    sourceComponent->withROLock([&]() {
        srcParent = sourceComponent->package();
    });

    PackageObject *trgParent = nullptr;
    targetComponent->withROLock([&]() {
        trgParent = targetComponent->package();
    });

    while (srcParent && trgParent && srcParent != trgParent) {
        lvtmdb::PackageObject::addDependency(srcParent, trgParent);

        srcParent->withROLock([&]() {
            srcParent = srcParent->parent();
        });

        trgParent->withROLock([&]() {
            trgParent = trgParent->parent();
        });
    }
}

} // namespace Codethink::lvtclp::fortran

#endif // CODEVIS_CT_LVTCLP_FORTRAN_DBUTILS_H
