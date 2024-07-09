// ct_lvttst_tmpdir.cpp                                                -*-C++-*-

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

#include <cassert>
#include <ct_lvttst_tmpdir.h>

TmpDir::TmpDir(const std::string& dirname): tmp_dir(d_tmpDirQt.path().toStdString() + "/" + dirname)
{
    std::filesystem::remove_all(tmp_dir);
    std::filesystem::create_directories(tmp_dir);
}

TmpDir::~TmpDir()
{
    std::filesystem::remove_all(tmp_dir);
}

std::filesystem::path TmpDir::path() const
{
    return tmp_dir;
}

std::filesystem::path TmpDir::createDir(const std::string& dirname) const
{
    auto p = path() / dirname;
    auto r = std::filesystem::create_directories(p);
    (void) r;
    assert(r);
    return p;
}

std::filesystem::path TmpDir::createTextFile(const std::string& name, const std::string& contents) const
{
    auto filePath = path() / std::filesystem::path(name);
    if (!std::filesystem::exists(filePath.parent_path())) {
        (void) createDir(filePath.parent_path().string());
    }
    if (std::filesystem::exists(filePath)) {
        std::filesystem::remove(filePath);
    }
    auto scriptStream = std::ofstream(filePath.string());
    scriptStream << contents;
    scriptStream.close();
    assert(std::ifstream{filePath.string()}.good());
    return filePath;
}
