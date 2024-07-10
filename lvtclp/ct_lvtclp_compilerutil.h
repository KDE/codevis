// ct_lvtclp_compilerutil.h                                           -*-C++-*-

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

#ifndef INCLUDED_CT_LVTCLP_COMPILERUTIL
#define INCLUDED_CT_LVTCLP_COMPILERUTIL

//@PURPOSE: Query information about installed compilers
//
//@CLASSES:
//  lvtclp::CompilerUtil Query information about installed compilers

#include <optional>
#include <string>
#include <vector>

#include <lvtclp_export.h>

namespace Codethink::lvtclp {

// =======================
// class CompilerUtil
// =======================

class LVTCLP_EXPORT CompilerUtil {
  public:
    // CLASS METHODS
    static std::optional<std::string> findBundledHeaders(bool silent);
    // Look to see if there were any clang headers bundled with this binary
    // If the headers are found, return a path to the directory containing
    // these headers. If we don't find any headers *and that is correct*,
    // return {""}. If we don't find any headers *and that is a problem*,
    // return {}
    // arguments:
    // silent: supress output

    static bool weNeedSystemHeaders();
    // Figure out if we need to use findSystemIncludes() to compile with
    // these flags

    static std::vector<std::string> findSystemIncludes();
    // Attempts to locate appropriate include directories to find standard
    // headers etc

    static std::optional<std::string> runCompiler(const std::string& compiler);
};

} // namespace Codethink::lvtclp

#endif // INCLUDED_CT_LVTCLP_COMPILERUTIL
