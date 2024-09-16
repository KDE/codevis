// ct_lvtldr_databasehandler.h                                       -*-C++-*-

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

#ifndef DIAGRAM_SERVER_CT_LVTLDR_DATABASEHANDLER_H
#define DIAGRAM_SERVER_CT_LVTLDR_DATABASEHANDLER_H

#include <ct_lvtldr_componentnodefields.h>
#include <ct_lvtldr_freefunctionnodefields.h>
#include <ct_lvtldr_packagenodefields.h>
#include <ct_lvtldr_repositorynodefields.h>
#include <ct_lvtldr_typenodefields.h>
#include <ct_lvtshr_graphenums.h>
#include <ct_lvtshr_uniqueid.h>

#include <result/result.hpp>
#include <vector>

namespace Codethink::lvtldr {

struct ErrorSqlQuery {
    std::string what;
};

struct RawDbQueryResult {
    std::vector<std::string> columns;
    std::vector<std::vector<std::string>> data;
};

class DatabaseHandler {
    using RecordNumberType = lvtshr::UniqueId::RecordNumberType;

  public:
    virtual ~DatabaseHandler() = 0;

    virtual void close() = 0;
    virtual cpp::result<RawDbQueryResult, ErrorSqlQuery> rawDbQuery(const std::string& query) = 0;

    virtual std::vector<lvtshr::UniqueId> getTopLevelEntityIds() = 0;

    virtual RepositoryNodeFields getRepositoryFieldsByQualifiedName(std::string const& qualifiedName) = 0;
    virtual RepositoryNodeFields getRepositoryFieldsById(RecordNumberType id) = 0;
    virtual void updateFields(RepositoryNodeFields const& dao) = 0;

    virtual void addFields(TypeNodeFields& dao) = 0;
    virtual TypeNodeFields getUdtFieldsByQualifiedName(std::string const& qualifiedName) = 0;
    virtual TypeNodeFields getUdtFieldsById(RecordNumberType id) = 0;
    virtual void updateFields(TypeNodeFields const& dao) = 0;
    virtual void removeUdtFieldsById(RecordNumberType id) = 0;

    virtual void addFields(ComponentNodeFields& dao) = 0;
    virtual ComponentNodeFields getComponentFieldsByQualifiedName(std::string const& qualifiedName) = 0;
    virtual ComponentNodeFields getComponentFieldsById(RecordNumberType id) = 0;
    virtual std::vector<ComponentNodeFields> getComponentFieldsByIds(const std::vector<RecordNumberType>& ids) = 0;
    virtual void updateFields(ComponentNodeFields const& dao) = 0;
    virtual void removeComponentFieldsById(RecordNumberType id) = 0;

    virtual PackageNodeFields getPackageFieldsByQualifiedName(std::string const& qualifiedName) = 0;
    virtual std::vector<PackageNodeFields> getPackageFieldsByIds(const std::vector<RecordNumberType>& ids) = 0;
    virtual std::vector<RecordNumberType> getPackageChildById(RecordNumberType ids) = 0;
    virtual std::vector<RecordNumberType> getPackageComponentsById(RecordNumberType ids) = 0;

    virtual PackageNodeFields getPackageFieldsById(RecordNumberType id) = 0;
    virtual void updateFields(PackageNodeFields const& dao) = 0;
    virtual void addFields(PackageNodeFields& dao) = 0;
    virtual void removePackageFieldsById(RecordNumberType id) = 0;

    virtual FreeFunctionNodeFields getFreeFunctionFieldsByQualifiedName(std::string const& qualifiedName) = 0;
    virtual FreeFunctionNodeFields getFreeFunctionFieldsById(RecordNumberType id) = 0;
    virtual void updateFields(FreeFunctionNodeFields const& dao) = 0;

    virtual void addConcreteDependency(RecordNumberType idFrom, RecordNumberType idTo) = 0;
    virtual void removeConcreteDependency(RecordNumberType idFrom, RecordNumberType idTo) = 0;

    virtual void addComponentDependency(RecordNumberType idFrom, RecordNumberType idTo) = 0;
    virtual void removeComponentDependency(RecordNumberType idFrom, RecordNumberType idTo) = 0;

    virtual void addClassHierarchy(RecordNumberType idFrom, RecordNumberType idTo) = 0;
    virtual void removeClassHierarchy(RecordNumberType idFrom, RecordNumberType idTo) = 0;

    virtual void addImplementationRelationship(RecordNumberType idFrom, RecordNumberType idTo) = 0;
    virtual void removeImplementationRelationship(RecordNumberType idFrom, RecordNumberType idTo) = 0;

    virtual void addInterfaceRelationship(RecordNumberType idFrom, RecordNumberType idTo) = 0;
    virtual void removeInterfaceRelationship(RecordNumberType idFrom, RecordNumberType idTo) = 0;

    virtual std::string getNotesFromId(lvtshr::UniqueId uid) = 0;
    virtual bool hasNotes(lvtshr::UniqueId uid) = 0;
    virtual void addNotes(lvtshr::UniqueId uid, std::string const& notes) = 0;
    virtual void setNotes(lvtshr::UniqueId uid, std::string const& notes) = 0;
};

inline DatabaseHandler::~DatabaseHandler() = default;

} // namespace Codethink::lvtldr

#endif // DIAGRAM_SERVER_CT_LVTLDR_DATABASEHANDLER_H
