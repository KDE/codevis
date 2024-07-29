// ct_lvtclp_physicaldepscanner.h                                    -*-C++-*-

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

#ifndef INCLUDED_CT_LVTCLP_PHYSICALDEPSCANNER
#define INCLUDED_CT_LVTCLP_PHYSICALDEPSCANNER

//@PURPOSE: Quickly scan a codebase to find all physical dependencies
//
//@CLASSES: lvtclp::DepScanActionFactory Implements FrontendActionFactory
//
//@SEE_ALSO: clang::tooling::FrontendActionFactory

#include <lvtclp_export.h>

#include <clang/Tooling/Tooling.h>
#include <ct_lvtclp_headercallbacks.h>
#include <ct_lvtclp_threadstringmap.h>

#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <QString>

namespace Codethink::lvtmdb {
class ObjectStore;
}

namespace Codethink::lvtclp {

// =============================
// class DepScanActionFactory
// =============================

class LVTCLP_EXPORT DepScanActionFactory : public clang::tooling::FrontendActionFactory {
    // Quickly scan a codebase to find all physical dependencies

  private:
    // TYPES
    struct Private;

    // DATA
    std::unique_ptr<Private> d;

  public:
    // CREATORS
    DepScanActionFactory(
        lvtmdb::ObjectStore& memDb,
        const std::filesystem::path& prefix,
        const std::filesystem::path& buildFolder,
        const std::vector<std::filesystem::path>& nonLakosians,
        const std::vector<std::pair<std::string, std::string>>& thirdPartyDirs,
        std::function<void(const std::string&)> filenameCallback, // callback that sends the current filename to the UI
        std::vector<llvm::GlobPattern> ignoreGlobs,
        ThreadStringMap& pathToCanonical,
        bool enableLakosianRules,
        std::optional<HeaderCallbacks::HeaderLocationCallback_f> headerLocationCallback = std::nullopt);

    ~DepScanActionFactory() noexcept override;

    // MANIPULATORS
    std::unique_ptr<clang::FrontendAction> create() override;
    // Construct a new clang::FrontendAction
};

} // namespace Codethink::lvtclp

#endif // INCLUDED_CT_LVTCLP_PHYSICALDEPSCANNER
