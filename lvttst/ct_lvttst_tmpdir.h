// ct_lvttst_tmpdir.h                              -*-C++-*-

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

#ifndef DIAGRAM_SERVER_CT_LVTTST_TMPDIR_H
#define DIAGRAM_SERVER_CT_LVTTST_TMPDIR_H

#include <QTemporaryDir>
#include <filesystem>
#include <fstream>

class TmpDir {
  public:
    explicit TmpDir(const std::string& dirname);
    ~TmpDir();

    [[nodiscard]] std::filesystem::path path() const;
    [[nodiscard]] std::filesystem::path createDir(const std::string& dirname) const;
    [[nodiscard]] std::filesystem::path createTextFile(const std::string& name, const std::string& contents) const;

  private:
    QTemporaryDir d_tmpDirQt;
    std::filesystem::path tmp_dir;
};

#endif // DIAGRAM_SERVER_CT_LVTTST_TMPDIR_H
