// ct_lvtclp_fileutil.cpp                                              -*-C++-*-

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

#include <ct_lvtclp_fileutil.h>

#include <QtGlobal>
#include <algorithm>

namespace Codethink::lvtclp {

bool FileUtil::pathStartsWith(const std::filesystem::path& prefix, const std::filesystem::path& path)
{
    // Avoid using the path object for mismatch since trailling '/' may not be considered
    auto prefixStr = prefix.string();
    auto pathStr = path.string();
    const auto [it1, _] = std::mismatch(prefixStr.begin(), prefixStr.end(), pathStr.begin(), pathStr.end());
    // the whole of prefix matches path (which may be longer)
    return it1 == prefixStr.end();
}

std::filesystem::path FileUtil::nonPrefixPart(const std::filesystem::path& prefix, const std::filesystem::path& path)
// Returns the suffix of path which isn't in the prefix
// e.g. path: /foo/bar/baz, prefix = /foo -> bar/baz
{
    const auto [_, it2] = std::mismatch(prefix.begin(), prefix.end(), path.begin(), path.end());

    std::filesystem::path ret;
    // return only the part of path after the mismatch: it2 to path.end().
    // unfortunately we can't construct a path directly from it2, path.end()
    // instead loop through, building up the path
    std::for_each(it2, path.end(), [&ret](const auto& elem) {
        ret /= elem;
    });

#ifdef Q_OS_WINDOWS
    ret = ret.generic_string();
#endif

    return ret;
}

std::filesystem::path FileUtil::commonParent(const std::vector<std::filesystem::path>& paths)
// All paths should be canonical
{
    if (paths.empty()) {
        return "";
    }

    auto startIt = paths.begin();
    const std::filesystem::path& first = *startIt;
    ++startIt;

    std::filesystem::path oldPrefix;
    std::filesystem::path prefix;

    // for each element in the first path, see if all the paths still start with
    // that prefix, returning when they don't all match
    // e.g.
    // Paths: /home/user/proj/pkgone/pkgone_file.cpp
    //        /home/user/proj/pkgtwo/pkgtwo_file.cpp
    //
    // Loop:
    //    - "/" : both match
    //    - "/home" : both match
    //    - "/home/user" : both match
    //    - "/home/user/proj" : both match
    //    - "/home/user/proj/pkgone" : second doesn't match
    //
    // This also takes in consideration if the last patch that matches is
    // inside a patch called "groups". This is needed for projects that
    // contains a single package, like:
    //
    // Paths: /home/user/proj/pkggrp/pkgone/pkgone_file.cpp
    //        /home/user/proj/pkggrp/pkgone/pkgtwo_file.cpp
    // the correct group here is pkggrp and not pkgone.

    for (const auto& component : first) {
        prefix /= component;

        auto pathStartsWithPrefix = [&prefix](const std::filesystem::path& path) -> bool {
            return pathStartsWith(prefix, path);
        };
        // start from the second path because the first definately matches
        if (!std::all_of(startIt, paths.end(), pathStartsWithPrefix)) {
            // now, try to look if the previous patch has *only* one folder.
            // and check that this folder is three letters long. this is the
            // sign of a project with a single package group.

            if (oldPrefix.filename().string().length() == 3) {
                return oldPrefix.parent_path();
            }
            return oldPrefix;
        }

        oldPrefix = prefix;
    }

    return oldPrefix;
}

} // end namespace Codethink::lvtclp
