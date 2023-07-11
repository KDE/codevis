// ct_lvtldr_nodestorageutils.h                                      -*-C++-*-

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

#ifndef DIAGRAM_SERVER_CT_LVTLDR_NODESTORAGETESTUTILS_H
#define DIAGRAM_SERVER_CT_LVTLDR_NODESTORAGETESTUTILS_H

#include <ct_lvtldr_nodestorage.h>
#include <ct_lvtmdb_objectstore.h>
#include <ct_lvtmdb_soci_writer.h>

#include <filesystem>
#include <string>

namespace Codethink::lvtldr {

struct NodeStorageTestUtils {
    static lvtldr::NodeStorage createEmptyNodeStorage(std::filesystem::path const& dbPath,
                                                      std::string const& schemaPath = "cad_db.sql")
    {
        {
            lvtmdb::SociWriter writer;
            writer.createOrOpen(dbPath.string(), schemaPath);
        }

        NodeStorage nodeStorage;
        nodeStorage.setDatabaseSourcePath(dbPath.string());
        return nodeStorage;
    }
};

} // namespace Codethink::lvtldr

#endif
