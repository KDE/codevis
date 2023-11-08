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

#include <flang/Frontend/CompilerInstance.h>
#include <fortran/ct_lvtclp_physicalscanner.h>

#include <iostream>

namespace Codethink::lvtclp::fortran {

using namespace Fortran::parser;

void PhysicalParseAction::executeAction()
{
    std::string currentInputPath{getCurrentFileOrBufferName()};
    auto& allSources = getInstance().getAllCookedSources().allSources();
    auto showSourceFileFrom = [&allSources](Provenance const& p) {
        auto srcFile = allSources.GetSourceFile(p);
        if (srcFile == nullptr) {
            return; // Doesn't have associated source file
        }
        // TODO: Save to database
        std::cout << "+ Source file: " << srcFile->path() << "\n";
    };

    auto interval = *allSources.GetFirstFileProvenance();
    auto currProvenance = interval.start();
    while (allSources.IsValid(currProvenance)) {
        showSourceFileFrom(interval.start());
        interval = interval.NextAfter();
        currProvenance = interval.start();
    }
}

} // namespace Codethink::lvtclp::fortran
