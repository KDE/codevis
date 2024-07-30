// ct_lvtclp_logicaldepscanner.h                                      -*-C++-*-

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

#ifndef INCLUDED_CT_LVTCLP_LOGICALDEPSCANNER
#define INCLUDED_CT_LVTCLP_LOGICALDEPSCANNER

//@PURPOSE: A factory for lvtclp::CodebaseDbAction
//
//@CLASSES: lvtclp::LogicalDepActionFactory Implements FrontendActionFactory
//          for scanning for logical dependencies
//
//@SEE_ALSO: clang::tooling::FrontendActionFactory

#include "ct_lvtclp_cpp_tool_constants.h"
#include <lvtclp_export.h>

#include <clang/Tooling/Tooling.h>

#include <functional>
#include <memory>
#include <optional>
#include <string>

#include <QString>

namespace Codethink::lvtmdb {
class ObjectStore;
}

namespace Codethink::lvtclp {

// =============================
// class LogicalDepActionFactory
// =============================

using HandleCppCommentsCallback_f = std::function<void(
    const std::string& filename, const std::string& briefText, unsigned startLine, unsigned endLine)>;

class LVTCLP_EXPORT LogicalDepActionFactory : public clang::tooling::FrontendActionFactory {
    // A factory for CodebaseDbFrontendAction

    // DATA
    lvtmdb::ObjectStore& d_memDb;
    // Database session generator

    std::function<void(const std::string&)> d_filenameCallback;
    // Callback whenever we start to process a new file

    std::optional<std::function<void(const std::string&, long)>> d_messageCallback;
    // Callback to send errors to the UI.

    std::optional<HandleCppCommentsCallback_f> d_handleCppCommentsCallback;

    const CppToolConstants& d_constants;

  public:
    // CREATORS
    LogicalDepActionFactory(lvtmdb::ObjectStore& memDb,
                            const CppToolConstants& constants,
                            std::function<void(const std::string&)> filenameCallback,
                            std::optional<std::function<void(const std::string&, long)>> messageCallback,
                            std::optional<HandleCppCommentsCallback_f> handleCppCommentsCallback = std::nullopt);

    // MANIPULATORS
    std::unique_ptr<clang::FrontendAction> create() override;
    // Construct a new clang::FrontendAction
};

} // end namespace Codethink::lvtclp

#endif
