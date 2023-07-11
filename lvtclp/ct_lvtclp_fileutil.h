// ct_lvtclp_fileutil.h                                                -*-C++-*-

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

#ifndef INCLUDED_CT_LVTCLP_FILEUTIL
#define INCLUDED_CT_LVTCLP_FILEUTIL

// Helper functions for dealing with paths
#include <lvtclp_export.h>

#include <filesystem>
#include <vector>

namespace Codethink::lvtclp {

// ================
// struct FileUtil
// ================

struct LVTCLP_EXPORT FileUtil {
    // This struct groups functions for dealing with files

  public:
    // CLASS METHODS

    static bool pathStartsWith(const std::filesystem::path& prefix, const std::filesystem::path& path);
    // Returns true if path starts with prefix
    // path and prefix should be canonical

    static std::filesystem::path nonPrefixPart(const std::filesystem::path& prefix, const std::filesystem::path& path);
    // Returns the suffix of path which isn't in the prefix
    // e.g. path: /foo/bar/baz, prefix = /foo -> bar/baz
    // path and prefix should be canonical

    static std::filesystem::path commonParent(const std::vector<std::filesystem::path>& paths);
    // Finds the common parent path of all of the supplied paths
    // The paths should be canonical
};

} // end namespace Codethink::lvtclp

#endif // !INCLUDED_CT_LVTCLP_FILEUTIL
