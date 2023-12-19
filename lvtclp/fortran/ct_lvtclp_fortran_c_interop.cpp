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
#include <ct_lvtmdb_fileobject.h>
#include <ct_lvtmdb_functionobject.h>
#include <ct_lvtmdb_packageobject.h>
#include <fortran/ct_lvtclp_fortran_c_interop.h>

#include <unordered_map>

using namespace Codethink::lvtmdb;

void Codethink::lvtclp::fortran::solveFortranToCInteropDeps(ObjectStore& sharedMemDb)
{
    // Heuristically find bindings from/to C/Fortran code
    auto& all_functions = sharedMemDb.functions();
    auto bindDependencies = std::vector<std::pair<FunctionObject *, FunctionObject *>>{};
    for (auto const& [_, ownedCFuncObject] : all_functions) {
        // C functions have a "_" prefix (e.g.: "myFunc_" in C is the function "myFunc" in Fortran)
        auto *cFunc = ownedCFuncObject.get();
        auto cFuncName = std::string{};
        cFunc->withROLock([&]() {
            cFuncName = cFunc->name();
        });
        if (!cFuncName.ends_with('_')) {
            continue;
        }
        auto fortranName = cFuncName.substr(0, cFuncName.size() - 1);

        // Check for possible Fortran binding
        // TODO: Only take in consideration *Fortran functions* in the loop below, because there may be a C function
        //       that matches the rules below, and the output would be wrong. But currently we don't have any way
        //       of identifying Fortran functions.
        auto it = std::find_if(std::cbegin(all_functions), std::cend(all_functions), [&fortranName](auto const& f) {
            auto lock = f.second->readOnlyLock();
            (void) lock;
            return f.second->name() == fortranName;
        });
        if (it == std::end(all_functions)) {
            continue; // Couldn't find.
        }

        auto *fortranFunc = it->second.get();
        bindDependencies.emplace_back(std::make_pair(cFunc, fortranFunc));
    }

    sharedMemDb.withRWLock([&]() {
        // There is no back-mapping from functions to the components in the database, so we create a local one.
        auto functionObjToComponentObj = std::unordered_map<FunctionObject *, ComponentObject *>{};
        for (auto& [_, component] : sharedMemDb.components()) {
            auto componentLock = component->readOnlyLock();
            for (auto *file : component->files()) {
                auto fileLock = file->readOnlyLock();
                for (auto *function : file->globalFunctions()) {
                    functionObjToComponentObj[function] = component.get();
                }
            }
        }

        for (auto& [from, to] : bindDependencies) {
            FunctionObject::addDependency(from, to);

            // Propagate dependency to parents
            auto *fromComponent = functionObjToComponentObj[from];
            auto *toComponent = functionObjToComponentObj[to];
            if (fromComponent && toComponent && fromComponent != toComponent) {
                ComponentObject::addDependency(fromComponent, toComponent);

                auto fromComponentLock = fromComponent->rwLock();
                auto toComponentLock = toComponent->rwLock();
                auto fromPackage = fromComponent->package();
                auto toPackage = toComponent->package();
                if (fromPackage && toPackage && fromPackage != toPackage) {
                    PackageObject::addDependency(fromPackage, toPackage);
                }
            }
        }
    });
}
