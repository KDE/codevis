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
#include <fortran/ct_lvtclp_fortran_dbutils.h>

#include <unordered_map>
#include <unordered_set>

using namespace Codethink::lvtmdb;

struct LocalSharedData {
    // Any data shared between the internal steps may be cached here.
    std::unordered_map<FunctionObject *, ComponentObject *> functionObjToComponentObj;
};

void addFunctionDependencyAndPropagate(FunctionObject *from, FunctionObject *to, LocalSharedData& localSharedData);
void heuristicallyFindCallsFromFortranToC(ObjectStore& sharedMemDb, LocalSharedData& localSharedData);
void heuristicallyFindCallsFromCToFortran(ObjectStore& sharedMemDb, LocalSharedData& localSharedData);

void Codethink::lvtclp::fortran::solveFortranToCInteropDeps(ObjectStore& sharedMemDb)
{
    // Heuristics for Fortran/C interoperability resolutions should be processed here.
    // It is assumed that Fortran codebase and C codebase are fully in the sharedMemDb object.

    auto localSharedData = LocalSharedData{};
    // There is no back-mapping from functions to the components in the database, so we create a local one.
    auto& functionObjToComponentObj = localSharedData.functionObjToComponentObj;
    for (auto& [_, component] : sharedMemDb.components()) {
        auto componentLock = component->readOnlyLock();
        for (auto *file : component->files()) {
            auto fileLock = file->readOnlyLock();
            for (auto *function : file->globalFunctions()) {
                functionObjToComponentObj[function] = component.get();
            }
        }
    }

    heuristicallyFindCallsFromFortranToC(sharedMemDb, localSharedData);
    heuristicallyFindCallsFromCToFortran(sharedMemDb, localSharedData);
}

void heuristicallyFindCallsFromFortranToC(ObjectStore& sharedMemDb, LocalSharedData& localSharedData)
{
    // Fortran code can call functions without having them declared anywhere.
    // This means the Fortran parser may have found a function that is called, but doesn't know where
    // the function comes from. Such functions may have been defined in C, with a "_" suffix.
    // The code below search for those functions in the C code and tries to replace the Fortran calls with the C calls.
    std::cout << "heuristicallyFindCallsFromFortranToC...\n";

    using namespace Codethink::lvtclp::fortran;
    auto& all_functions = sharedMemDb.functions();
    auto& functionObjToComponentObj = localSharedData.functionObjToComponentObj;

    auto functionQualifiedNameToFunctionObject = std::unordered_map<std::string, FunctionObject *>{};
    auto functionsWithoutComponent = std::unordered_set<FunctionObject *>{};
    for (auto const& [_, function] : all_functions) {
        auto functionLock = function->readOnlyLock();
        functionQualifiedNameToFunctionObject[function->qualifiedName()] = function.get();
        if (!functionObjToComponentObj.contains(function.get())) {
            functionsWithoutComponent.insert(function.get());
        }
    }

    std::cout << "Found " << functionsWithoutComponent.size() << " orphan functions.\n";
    for (auto const& orphanFunction : functionsWithoutComponent) {
        auto orphanFunctionLock = orphanFunction->readOnlyLock();
        auto guessedCFunctionName = orphanFunction->qualifiedName() + "_";
        std::cout << "Searching for C function " << guessedCFunctionName << "...\n";
        if (!functionQualifiedNameToFunctionObject.contains(guessedCFunctionName)) {
            // The function source remains unknown - Ignore.
            continue;
        }

        std::cout << "Found! Updating callers... ";
        auto matchedCFunctionObject = functionQualifiedNameToFunctionObject[guessedCFunctionName];
        for (auto const& callerObject : orphanFunction->callers()) {
            std::cout << "x ";
            addFunctionDependencyAndPropagate(callerObject, matchedCFunctionObject, localSharedData);
        }
        std::cout << "\n";

        // TODO: Decide if the orphan function should be removed.
        //  Could it be the case that it'll appear again if we merge another code databases?
    }
}

void heuristicallyFindCallsFromCToFortran(ObjectStore& sharedMemDb, LocalSharedData& localSharedData)
{
    // The code below will "force a call" from myFunc_ (C declaration) to myFunc (Fortran implementation).
    std::cout << "heuristicallyFindCallsFromCToFortran...\n";
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
        for (auto& [from, to] : bindDependencies) {
            addFunctionDependencyAndPropagate(from, to, localSharedData);
        }
    });
}

void addFunctionDependencyAndPropagate(FunctionObject *from, FunctionObject *to, LocalSharedData& localSharedData)
{
    using namespace Codethink::lvtclp::fortran;

    auto& functionObjToComponentObj = localSharedData.functionObjToComponentObj;
    FunctionObject::addDependency(from, to);

    // Propagate dependency to parents
    auto *fromComponent = functionObjToComponentObj[from];
    auto *toComponent = functionObjToComponentObj[to];
    if (fromComponent && toComponent && fromComponent != toComponent) {
        recursiveAddComponentDependency(fromComponent, toComponent);
    }
}
