// ct_lvtldr_sociutils.h                                             -*-C++-*-

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

#ifndef DIAGRAM_SERVER_CT_LVTLDR_SOCIUTILS_H
#define DIAGRAM_SERVER_CT_LVTLDR_SOCIUTILS_H

#include "soci/soci-backend.h"
#include <ct_lvtldr_componentnodefields.h>
#include <ct_lvtldr_databasehandler.h>
#include <ct_lvtshr_uniqueid.h>

#include <result/result.hpp>
#include <soci/soci.h>
#include <soci/sqlite3/soci-sqlite3.h>

#include <QObject>
#include <QString>

namespace {
template<typename T>
std::string optionalToDb(std::optional<T> data)
{
    if (!data) {
        return "NULL";
    }
    return std::to_string(*data);
}
} // namespace

namespace Codethink::lvtldr {

class SociDatabaseHandler : public DatabaseHandler {
    using RecordNumberType = Codethink::lvtshr::UniqueId::RecordNumberType;

  public:
    explicit SociDatabaseHandler(std::string const& path)
    {
        d_db.open(*soci::factory_sqlite3(), path);
    }

    void close() override
    {
        d_db.close();
    }

    cpp::result<RawDbQueryResult, ErrorSqlQuery> rawDbQuery(const std::string& query) override
    {
        RawDbQueryResult res;
        try {
            soci::rowset<soci::row> rs = (d_db.prepare << query);

            int loadColumnNames = true;
            for (const auto& row : rs) {
                std::vector<std::string> thisRow;
                for (int i = 0, size = row.size(); i < size; i += 1) {
                    const soci::column_properties& props = row.get_properties(i);

                    if (loadColumnNames) {
                        res.columns.push_back(props.get_name());
                    }
                    switch (props.get_data_type()) {
                    case soci::dt_string:
                        thisRow.push_back(row.get<std::string>(i));
                        break;
                    case soci::dt_double:
                        thisRow.push_back(std::to_string(row.get<double>(i)));
                        break;
                    case soci::dt_integer:
                        thisRow.push_back(std::to_string(row.get<int>(i)));
                        break;
                    case soci::dt_long_long:
                        thisRow.push_back(std::to_string(row.get<long long>(i)));
                        break;
                    case soci::dt_unsigned_long_long:
                        thisRow.push_back(std::to_string(row.get<unsigned long long>(i)));
                        break;
                    case soci::dt_xml:
                        thisRow.push_back(row.get<std::string>(i));
                        break;
                    case soci::dt_blob:
                        thisRow.push_back(QObject::tr("binary-data").toStdString());
                        break;
                    case soci::dt_date:
                        std::tm when = row.get<std::tm>(i);
                        thisRow.push_back(asctime(&when));
                        break;
                    }
                }
                loadColumnNames = false;
                res.data.push_back(thisRow);
            }
        } catch (std::exception const& e) {
            return cpp::fail(e.what());
        }
        return res;
    }

    std::vector<lvtshr::UniqueId> getTopLevelEntityIds() override
    {
        auto out = std::vector<lvtshr::UniqueId>{};
        {
            soci::rowset<RecordNumberType> rs = (d_db.prepare << "select id from source_repository where name != ''");
            for (auto&& i : rs) {
                out.emplace_back(lvtshr::UniqueId{lvtshr::DiagramType::RepositoryType, i});
            }
        }
        {
            soci::rowset<RecordNumberType> rs =
                (d_db.prepare
                 << "select id from source_package where parent_id is NULL and source_repository_id is NULL");
            for (auto&& i : rs) {
                out.emplace_back(lvtshr::UniqueId{lvtshr::DiagramType::PackageType, i});
            }
        }
        return out;
    }

    RepositoryNodeFields getRepositoryFieldsByQualifiedName(std::string const& qualifiedName) override
    {
        return getRepositoryFields("qualified_name", qualifiedName);
    }

    RepositoryNodeFields getRepositoryFieldsById(RecordNumberType id) override
    {
        return getRepositoryFields("id", id);
    }

    void updateFields(RepositoryNodeFields const& dao) override
    {
    }

    TypeNodeFields getUdtFieldsByQualifiedName(std::string const& qualifiedName) override
    {
        return getUdtFields("qualified_name", qualifiedName);
    }

    TypeNodeFields getUdtFieldsById(RecordNumberType id) override
    {
        return getUdtFields("id", id);
    }

    void addFields(TypeNodeFields& dao) override
    {
        soci::transaction tr(d_db);
        d_db << "insert into class_declaration (version, qualified_name, name, kind, access, class_namespace_id, "
                "parent_package_id) "
                "values "
             << "(" << dao.version << ", "
             << "'" << dao.qualifiedName << "', "
             << "'" << dao.name << "', "
             << "" << static_cast<int>(dao.kind) << ", "
             << "" << dao.access << ", "
             << "" << optionalToDb(dao.classNamespaceId) << ", "
             << "" << optionalToDb(dao.parentPackageId) << ""
             << ")";

        d_db.get_last_insert_id("class_declaration", dao.id);

        for (auto&& id : dao.componentIds) {
            d_db << "insert into udt_component (component_id, udt_id) values "
                 << "(" << id << ", " << dao.id << ")";
        }
        tr.commit();
    }

    void updateFields(TypeNodeFields const& dao) override
    {
        soci::transaction tr(d_db);
        d_db << "update class_declaration set "
             << "version = " << dao.version << ", "
             << "name = '" << dao.name << "', "
             << "qualified_name = '" << dao.qualifiedName << "', "
             << "kind = " << static_cast<int>(dao.kind) << ", "
             << "access = " << dao.access << ", "
             << "class_namespace_id = " << optionalToDb(dao.classNamespaceId) << " "
             << "where id = :k",
            soci::use(dao.id);
        tr.commit();
    }

    void removeUdtFieldsById(RecordNumberType id) override
    {
        soci::transaction tr(d_db);
        d_db << "delete from class_declaration where id = " << id;
        tr.commit();
    }

    ComponentNodeFields getComponentFieldsByQualifiedName(std::string const& qualifiedName) override
    {
        return getComponentFields("qualified_name", qualifiedName);
    }

    ComponentNodeFields getComponentFieldsById(RecordNumberType id) override
    {
        return getComponentFields("id", id);
    }

    void addFields(ComponentNodeFields& dao) override
    {
        soci::transaction tr(d_db);
        d_db << "insert into source_component (version, qualified_name, name, package_id) "
                "values "
             << "(" << dao.version << ", "
             << "'" << dao.qualifiedName << "', "
             << "'" << dao.name << "', "
             << "" << optionalToDb(dao.packageId) << ""
             << ")";
        d_db.get_last_insert_id("source_component", dao.id);
        tr.commit();
    }

    void updateFields(ComponentNodeFields const& dao) override
    {
        soci::transaction tr(d_db);
        d_db << "update source_component set "
             << "version = " << dao.version << ", "
             << "qualified_name = '" << dao.qualifiedName << "', "
             << "name = '" << dao.name << "', "
             << "package_id = " << optionalToDb(dao.packageId) << " "
             << "where id = :k",
            soci::use(dao.id);
        tr.commit();
    }

    void removeComponentFieldsById(RecordNumberType id) override
    {
        soci::transaction tr(d_db);
        d_db << "delete from source_component where id = " << id;
        tr.commit();
    }

    PackageNodeFields getPackageFieldsByQualifiedName(std::string const& qualifiedName) override
    {
        return getPackageFields("qualified_name", qualifiedName);
    }

    PackageNodeFields getPackageFieldsById(RecordNumberType id) override
    {
        return getPackageFields("id", id);
    }

    void updateFields(FreeFunctionNodeFields const& dao) override
    {
        // Intentionally not implemented
    }

    FreeFunctionNodeFields getFreeFunctionFieldsByQualifiedName(std::string const& qualifiedName) override
    {
        return getFreeFunctionFields("qualified_name", qualifiedName);
    }

    FreeFunctionNodeFields getFreeFunctionFieldsById(RecordNumberType id) override
    {
        return getFreeFunctionFields("id", id);
    }

    void addFields(PackageNodeFields& dao) override
    {
        soci::transaction tr(d_db);
        d_db
            << "insert into source_package (version, qualified_name, name, disk_path, parent_id, source_repository_id) "
               "values "
            << "(" << dao.version << ", "
            << "'" << dao.qualifiedName << "', "
            << "'" << dao.name << "', "
            << "'" << dao.diskPath << "', "
            << "" << optionalToDb(dao.parentId) << ", "
            << "" << optionalToDb(dao.sourceRepositoryId) << " "
            << ")";
        d_db.get_last_insert_id("source_package", dao.id);
        tr.commit();
    }

    void updateFields(PackageNodeFields const& dao) override
    {
        soci::transaction tr(d_db);
        d_db << "update source_package set "
             << "version = " << dao.version << ", "
             << "parent_id = " << optionalToDb(dao.parentId) << ", "
             << "source_repository_id = " << optionalToDb(dao.sourceRepositoryId) << ", "
             << "name = '" << dao.name << "', "
             << "qualified_name = '" << dao.qualifiedName << "', "
             << "disk_path = '" << dao.diskPath << "' "
             << "where id = :k",
            soci::use(dao.id);
        tr.commit();
    }

    void removePackageFieldsById(RecordNumberType id) override
    {
        soci::transaction tr(d_db);
        d_db << "delete from source_package where id = " << id;
        d_db << "delete from cad_notes where entity_id = " << id
             << " and entity_type = " << static_cast<int>(lvtshr::DiagramType::PackageType);
        tr.commit();
    }

    void addConcreteDependency(RecordNumberType idFrom, RecordNumberType idTo) override
    {
        soci::transaction tr(d_db);
        d_db << "insert into dependencies (source_id, target_id) values (" << idFrom << ", " << idTo << ")";
        tr.commit();
    }

    void removeConcreteDependency(RecordNumberType idFrom, RecordNumberType idTo) override
    {
        soci::transaction tr(d_db);
        d_db << "delete from dependencies where source_id = " << idFrom << " and target_id = " << idTo;
        tr.commit();
    }

    void addComponentDependency(RecordNumberType idFrom, RecordNumberType idTo) override
    {
        soci::transaction tr(d_db);
        d_db << "insert into component_relation (source_id, target_id) values (" << idFrom << ", " << idTo << ")";
        tr.commit();
    }

    void removeComponentDependency(RecordNumberType idFrom, RecordNumberType idTo) override
    {
        soci::transaction tr(d_db);
        d_db << "delete from component_relation where source_id = " << idFrom << " and target_id = " << idTo;
        tr.commit();
    }

    void addClassHierarchy(RecordNumberType idFrom, RecordNumberType idTo) override
    {
        soci::transaction tr(d_db);
        d_db << "insert into class_hierarchy (source_id, target_id) values (" << idFrom << ", " << idTo << ")";
        tr.commit();
    }

    void removeClassHierarchy(RecordNumberType idFrom, RecordNumberType idTo) override
    {
        soci::transaction tr(d_db);
        d_db << "delete from class_hierarchy where source_id = " << idFrom << " and target_id = " << idTo;
        tr.commit();
    }

    void addImplementationRelationship(RecordNumberType idFrom, RecordNumberType idTo) override
    {
        soci::transaction tr(d_db);
        d_db << "insert into uses_in_the_implementation (source_id, target_id) values (" << idFrom << ", " << idTo
             << ")";
        tr.commit();
    }

    void removeImplementationRelationship(RecordNumberType idFrom, RecordNumberType idTo) override
    {
        soci::transaction tr(d_db);
        d_db << "delete from uses_in_the_implementation where source_id = " << idFrom << " and target_id = " << idTo;
        tr.commit();
    }

    void addInterfaceRelationship(RecordNumberType idFrom, RecordNumberType idTo) override
    {
        soci::transaction tr(d_db);
        d_db << "insert into uses_in_the_interface (source_id, target_id) values (" << idFrom << ", " << idTo << ")";
        tr.commit();
    }

    void removeInterfaceRelationship(RecordNumberType idFrom, RecordNumberType idTo) override
    {
        soci::transaction tr(d_db);
        d_db << "delete from uses_in_the_interface where source_id = " << idFrom << " and target_id = " << idTo;
        tr.commit();
    }

    std::string getNotesFromId(lvtshr::UniqueId uid) override
    {
        try {
            std::string notes;
            soci::transaction tr(d_db);
            d_db << "select notes from cad_notes where entity_id = " << uid.recordNumber()
                 << " and entity_type = " << static_cast<int>(uid.diagramType()),
                soci::into(notes);
            tr.commit();
            return notes;
        } catch (...) {
            return "";
        }
    }

    bool hasNotes(lvtshr::UniqueId uid) override
    {
        try {
            int n;
            soci::transaction tr(d_db);
            d_db << "select count(*) from cad_notes where entity_id = " << uid.recordNumber()
                 << " and entity_type = " << static_cast<int>(uid.diagramType()),
                soci::into(n);
            tr.commit();
            return n > 0;
        } catch (...) {
            return false;
        }
    }

    void addNotes(lvtshr::UniqueId uid, std::string const& notes) override
    {
        std::string our_notes = QString::fromStdString(notes).replace("'", "''").toStdString();
        soci::transaction tr(d_db);
        d_db << "insert into cad_notes (version, entity_id, entity_type, notes) values (" << 0 << ", "
             << uid.recordNumber() << ", " << static_cast<int>(uid.diagramType()) << ", "
             << "'" << our_notes << "')";
        tr.commit();
    }

    void setNotes(lvtshr::UniqueId uid, std::string const& notes) override
    {
        std::string our_notes = QString::fromStdString(notes).replace("'", "''").toStdString();
        soci::transaction tr(d_db);
        d_db << "update cad_notes set notes = '" << our_notes << "' where "
             << "entity_id = " << uid.recordNumber() << " and "
             << "entity_type = " << static_cast<int>(uid.diagramType());
        tr.commit();
    }

    soci::session& getSession()
    {
        return d_db;
    }

  private:
    template<typename T>
    RepositoryNodeFields getRepositoryFields(std::string const& uniqueKeyColumnName, T const& keyValue)
    {
        decltype(getRepositoryFields(uniqueKeyColumnName, keyValue)) dao;
        d_db << "select * from source_repository where " + uniqueKeyColumnName + " = :k", soci::into(dao.id),
            soci::into(dao.version), soci::into(dao.name), soci::into(dao.qualifiedName), soci::into(dao.diskPath),
            soci::use(keyValue);

        {
            soci::rowset<RecordNumberType> rs =
                (d_db.prepare << "select id from source_package where source_repository_id = :k and parent_id is NULL",
                 soci::use(dao.id));
            for (auto&& i : rs) {
                dao.childPackagesIds.emplace_back(i);
            }
        }

        return dao;
    }

    template<typename T>
    TypeNodeFields getUdtFields(std::string const& uniqueKeyColumnName, T const& keyValue)
    {
        decltype(getUdtFields(uniqueKeyColumnName, keyValue)) dao;
        soci::indicator parentNamespaceIdIndicator = soci::indicator::i_null;
        typename std::remove_reference<decltype(dao.parentNamespaceId.value())>::type maybeParentNamespaceId = 0;
        soci::indicator classNamespaceIdIndicator = soci::indicator::i_null;
        typename std::remove_reference<decltype(dao.classNamespaceId.value())>::type maybeClassNamespaceId = 0;
        soci::indicator parentPackageIdIndicator = soci::indicator::i_null;
        typename std::remove_reference<decltype(dao.parentPackageId.value())>::type maybeParentPackageId = 0;

        int kindAsInt = -1;

        d_db << "select * from class_declaration where " + uniqueKeyColumnName + " = :k", soci::into(dao.id),
            soci::into(dao.version), soci::into(maybeParentNamespaceId, parentNamespaceIdIndicator),
            soci::into(maybeClassNamespaceId, classNamespaceIdIndicator),
            soci::into(maybeParentPackageId, parentPackageIdIndicator), soci::into(dao.name),
            soci::into(dao.qualifiedName), soci::into(kindAsInt), soci::into(dao.access), soci::use(keyValue);

        dao.kind = static_cast<lvtshr::UDTKind>(kindAsInt);

        if (parentNamespaceIdIndicator == soci::indicator::i_ok) {
            dao.parentNamespaceId = maybeParentNamespaceId;
        } else {
            dao.parentNamespaceId = std::nullopt;
        }

        if (classNamespaceIdIndicator == soci::indicator::i_ok) {
            dao.classNamespaceId = maybeClassNamespaceId;
        } else {
            dao.classNamespaceId = std::nullopt;
        }

        if (parentPackageIdIndicator == soci::indicator::i_ok) {
            dao.parentPackageId = maybeParentPackageId;
        } else {
            dao.parentPackageId = std::nullopt;
        }

        {
            soci::rowset<RecordNumberType> rs =
                (d_db.prepare << "select component_id from udt_component where udt_id = :k", soci::use(dao.id));
            for (auto&& i : rs) {
                dao.componentIds.emplace_back(i);
            }
        }
        {
            soci::rowset<RecordNumberType> rs =
                (d_db.prepare << "select id from class_declaration where class_namespace_id = :k", soci::use(dao.id));
            for (auto&& i : rs) {
                dao.nestedTypeIds.emplace_back(i);
            }
        }
        {
            soci::rowset<RecordNumberType> rs =
                (d_db.prepare << "select source_id from class_hierarchy where target_id = :k", soci::use(dao.id));
            for (auto&& i : rs) {
                dao.isAIds.emplace_back(i);
            }
        }
        {
            soci::rowset<RecordNumberType> rs =
                (d_db.prepare << "select target_id from uses_in_the_interface where source_id = :k", soci::use(dao.id));
            for (auto&& i : rs) {
                dao.usesInInterfaceIds.emplace_back(i);
            }
        }
        {
            soci::rowset<RecordNumberType> rs =
                (d_db.prepare << "select target_id from uses_in_the_implementation where source_id = :k",
                 soci::use(dao.id));
            for (auto&& i : rs) {
                dao.usesInImplementationIds.emplace_back(i);
            }
        }

        {
            soci::rowset<RecordNumberType> rs =
                (d_db.prepare << "select target_id from class_hierarchy where source_id = :k", soci::use(dao.id));
            for (auto&& i : rs) {
                dao.isBaseOfIds.emplace_back(i);
            }
        }
        {
            soci::rowset<RecordNumberType> rs =
                (d_db.prepare << "select source_id from uses_in_the_interface where target_id = :k", soci::use(dao.id));
            for (auto&& i : rs) {
                dao.usedByInterfaceIds.emplace_back(i);
            }
        }
        {
            soci::rowset<RecordNumberType> rs =
                (d_db.prepare << "select source_id from uses_in_the_implementation where target_id = :k",
                 soci::use(dao.id));
            for (auto&& i : rs) {
                dao.usedByImplementationIds.emplace_back(i);
            }
        }

        {
            soci::rowset<soci::row> rs =
                (d_db.prepare << "select id, name from field_declaration where class_id = :k", soci::use(dao.id));
            for (auto&& row : rs) {
                dao.fieldIds.emplace_back(row.get<int>(0));
                dao.fieldNames.emplace_back(row.get<std::string>(1));
            }
        }

        {
            soci::rowset<soci::row> rs =
                (d_db.prepare << "select id, name from method_declaration where class_id = :k", soci::use(dao.id));
            for (auto&& row : rs) {
                dao.methodIds.emplace_back(row.get<int>(0));
                dao.methodNames.emplace_back(row.get<std::string>(1));
            }
        }

        return dao;
    }

    template<typename T>
    ComponentNodeFields getComponentFields(std::string const& uniqueKeyColumnName, T const& keyValue)
    {
        decltype(getComponentFields(uniqueKeyColumnName, keyValue)) dao;
        soci::indicator parentIdIndicator = soci::indicator::i_null;
        typename std::remove_reference<decltype(dao.packageId.value())>::type maybeParentId = 0;
        d_db << "select * from source_component where " + uniqueKeyColumnName + " = :k", soci::into(dao.id),
            soci::into(dao.version), soci::into(dao.qualifiedName), soci::into(dao.name),
            soci::into(maybeParentId, parentIdIndicator), soci::use(keyValue);

        if (parentIdIndicator == soci::indicator::i_ok) {
            dao.packageId = maybeParentId;
        } else {
            dao.packageId = std::nullopt;
        }

        {
            soci::rowset<RecordNumberType> rs =
                (d_db.prepare << "select target_id from component_relation where source_id = :k", soci::use(dao.id));
            for (auto&& i : rs) {
                dao.providerIds.emplace_back(i);
            }
        }
        {
            soci::rowset<RecordNumberType> rs =
                (d_db.prepare << "select source_id from component_relation where target_id = :k", soci::use(dao.id));
            for (auto&& i : rs) {
                dao.clientIds.emplace_back(i);
            }
        }
        {
            soci::rowset<RecordNumberType> rs =
                (d_db.prepare << "select udt_id from udt_component where component_id = :k", soci::use(dao.id));
            for (auto&& i : rs) {
                dao.childUdtIds.emplace_back(i);
            }
        }
        try {
            soci::rowset<RecordNumberType> files_rs =
                (d_db.prepare << "select id from source_file where component_id = :k", soci::use(dao.id));
            for (auto const& source_id : files_rs) {
                soci::rowset<RecordNumberType> rs =
                    (d_db.prepare << "select function_id from global_function_source_file where source_file_id = :k",
                     soci::use(source_id));
                for (auto const& i : rs) {
                    dao.childGlobalFunctionIds.emplace_back(i);
                }
            }
        } catch (soci::sqlite3_soci_error const&) {
            /* Ignore missing data on old databases */
        }

        return dao;
    }

    template<typename T>
    PackageNodeFields getPackageFields(std::string const& uniqueKeyColumnName, T const& keyValue)
    {
        decltype(getPackageFields(uniqueKeyColumnName, keyValue)) dao;
        soci::indicator parentIdIndicator = soci::indicator::i_null;
        typename std::remove_reference<decltype(dao.parentId.value())>::type maybeParentId = 0;
        soci::indicator sourceRepositoryIdIndicator = soci::indicator::i_null;
        typename std::remove_reference<decltype(dao.sourceRepositoryId.value())>::type maybeSourceRepositoryId = 0;
        d_db << "select * from source_package where " + uniqueKeyColumnName + " = :k", soci::into(dao.id),
            soci::into(dao.version), soci::into(maybeParentId, parentIdIndicator),
            soci::into(maybeSourceRepositoryId, sourceRepositoryIdIndicator), soci::into(dao.name),
            soci::into(dao.qualifiedName), soci::use(keyValue);

        if (parentIdIndicator == soci::indicator::i_ok) {
            dao.parentId = maybeParentId;
        } else {
            dao.parentId = std::nullopt;
        }

        if (sourceRepositoryIdIndicator == soci::indicator::i_ok) {
            dao.sourceRepositoryId = maybeSourceRepositoryId;
        } else {
            dao.sourceRepositoryId = std::nullopt;
        }

        {
            soci::rowset<RecordNumberType> rs =
                (d_db.prepare << "select id from source_package where parent_id = :k", soci::use(dao.id));
            for (auto&& i : rs) {
                dao.childPackagesIds.emplace_back(i);
            }
        }
        {
            soci::rowset<RecordNumberType> rs =
                (d_db.prepare << "select id from source_component where package_id = :k", soci::use(dao.id));
            for (auto&& i : rs) {
                dao.childComponentsIds.emplace_back(i);
            }
        }
        {
            soci::rowset<RecordNumberType> rs =
                (d_db.prepare << "select target_id from dependencies where source_id = :k", soci::use(dao.id));
            for (auto&& i : rs) {
                dao.providerIds.emplace_back(i);
            }
        }
        {
            soci::rowset<RecordNumberType> rs =
                (d_db.prepare << "select source_id from dependencies where target_id = :k", soci::use(dao.id));
            for (auto&& i : rs) {
                dao.clientIds.emplace_back(i);
            }
        }

        return dao;
    }

    template<typename T>
    FreeFunctionNodeFields getFreeFunctionFields(std::string const& uniqueKeyColumnName, T const& keyValue)
    {
        decltype(getFreeFunctionFields(uniqueKeyColumnName, keyValue)) dao;
        d_db << "select id, version, name, qualified_name from function_declaration where " + uniqueKeyColumnName
                + " = :k limit 1",
            soci::into(dao.id), soci::into(dao.version), soci::into(dao.name), soci::into(dao.qualifiedName),
            soci::use(keyValue);

        {
            auto source_id = -1;
            d_db << "select source_file_id from global_function_source_file where function_id = :k limit 1",
                soci::into(source_id), soci::use(dao.id);

            auto component_id = -1;
            d_db << "select component_id from source_file where id = :k limit 1", soci::into(component_id),
                soci::use(source_id);
            dao.componentId = component_id;
        }

        {
            soci::rowset<RecordNumberType> rs =
                (d_db.prepare << "select callee_id from function_calls where caller_id = :k", soci::use(dao.id));
            for (auto&& i : rs) {
                dao.calleeIds.emplace_back(i);
            }
        }

        {
            soci::rowset<RecordNumberType> rs =
                (d_db.prepare << "select caller_id from function_calls where callee_id = :k", soci::use(dao.id));
            for (auto&& i : rs) {
                dao.callerIds.emplace_back(i);
            }
        }

        return dao;
    }

    soci::session d_db;
};

} // namespace Codethink::lvtldr

#endif // DIAGRAM_SERVER_CT_LVTLDR_SOCIUTILS_H
