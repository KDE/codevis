// ct_lvtmdb_soci_writer.h                                      -*-C++-*-

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

#ifndef CT_LVTMDB_SOCI_WRITER_H
#define CT_LVTMDB_SOCI_WRITER_H

#include <filesystem>
#include <lvtmdb_export.h>
#include <result/result.hpp>
#include <soci/soci.h>

namespace Codethink::lvtmdb {
class ObjectStore;

/* Usage:
 * LvtMDb::ObjectStore obj; // populated with information.
 * SociWriter writer;
 * writer.createOrOpen("/some/path.db");
 * obj.writeToDatabase(writer);
 */
class LVTMDB_EXPORT SociWriter {
  public:
    friend class ObjectStore;

    SociWriter();

    bool updateDbSchema(const std::string& db, const std::string& schemaPath);
    bool createOrOpen(const std::string& path, const std::string& schemaPath = "codebase_db.sql");
    /* this needs to be std::string because the
     * path can also be ":memory:" - we transform
     * it to an actual std::filesystem::path when
     * it's not.
     */

  private:
    void writeFrom(const ObjectStore& store);
    // Acessor to the object store.

    std::filesystem::path d_path;
    soci::session d_db;
};
} // namespace Codethink::lvtmdb

#endif
