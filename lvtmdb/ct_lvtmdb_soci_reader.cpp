// ct_lvtmdb_soci_reader.h                                      -*-C++-*-

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

#include <ct_lvtmdb_soci_helper.h>
#include <ct_lvtmdb_soci_reader.h>
#include <ct_lvtshr_graphenums.h>

#include <filesystem>
#include <optional>

#include <soci/soci-backend.h>
#include <soci/soci.h>
#include <soci/sqlite3/soci-sqlite3.h>

#include <ct_lvtmdb_componentobject.h>
#include <ct_lvtmdb_errorobject.h>
#include <ct_lvtmdb_fieldobject.h>
#include <ct_lvtmdb_fileobject.h>
#include <ct_lvtmdb_functionobject.h>
#include <ct_lvtmdb_methodobject.h>
#include <ct_lvtmdb_namespaceobject.h>
#include <ct_lvtmdb_packageobject.h>
#include <ct_lvtmdb_repositoryobject.h>
#include <ct_lvtmdb_typeobject.h>
#include <ct_lvtmdb_variableobject.h>

using namespace Codethink::lvtmdb;

namespace {

struct Maps {
    std::map<std::string, RepositoryObject *> repositoryMap;
    std::map<std::string, PackageObject *> packageMap;
    std::map<int, ComponentObject *> componentMap;
    std::map<int, FileObject *> filesMap;
    std::map<std::string, NamespaceObject *> namespaceMap;
    std::map<int, VariableObject *> variableMap;
    std::map<int, FunctionObject *> functionMap;
    std::map<int, TypeObject *> userDefinedTypeMap;
    std::map<int, FieldObject *> fieldMap;
    std::map<int, MethodObject *> methodMap;
};

void loadRepositories(ObjectStore& store, soci::session& db, Maps& map)
{
    soci::rowset<soci::row> rowset = (db.prepare << "select name, disk_path from source_repository");
    for (const auto& row : rowset) {
        const auto name = row.get<std::string>(0);
        const auto path = row.get<std::string>(1);
        auto *repo = store.getOrAddRepository(name, path);
        map.repositoryMap[name] = repo;
    }
}

struct PackageQueryParams {
    int id = 0;

    soci::indicator parent_ind = soci::indicator::i_null;
    std::string parent_qualified_name;

    soci::indicator repository_ind = soci::indicator::i_null;
    std::string repository_name;

    std::string name;
    std::string qualified_name;
    std::string path;
};

// This loads the packages in order so we don't need to worry about having
// not loaded the parent yet.
void loadPackages(ObjectStore& store, soci::session& db, Maps& maps)
{
    // soci prepared statements didn't work for null vs value, neither with std::optional.'
    std::string query =
        R"(select
  sp.id,
  pp.qualified_name as parent_name,
  sp.name,
  sp.qualified_name,
  sp.disk_path,
  re.name
from
  source_package sp
left join
	source_package pp on sp.parent_id = pp.id
left join
    source_repository re on sp.source_repository_id = re.id
order by sp.parent_id ASC )";

    PackageQueryParams params;
    soci::statement st = (db.prepare << query,
                          soci::into(params.id),
                          soci::into(params.parent_qualified_name, params.parent_ind),
                          soci::into(params.name),
                          soci::into(params.qualified_name),
                          soci::into(params.path),
                          soci::into(params.repository_name, params.repository_ind));

    st.execute();
    std::vector<int> current_parents;
    while (st.fetch()) {
        PackageObject *parent = nullptr;
        if (params.parent_ind == soci::indicator::i_ok) {
            parent = maps.packageMap[params.parent_qualified_name];
        }

        RepositoryObject *repository = nullptr;
        if (params.repository_ind == soci::indicator::i_ok) {
            repository = maps.repositoryMap[params.repository_name];
        }

        PackageObject *pkg = store.getOrAddPackage(params.qualified_name, params.name, params.path, parent, repository);
        current_parents.push_back(params.id);
        maps.packageMap[params.qualified_name] = pkg;

        if (parent) {
            parent->withRWLock([&]() {
                parent->addChild(pkg);
            });
        }
    }
}

struct ComponentQueryParams {
    int id = 0;
    std::string packageQualifiedName;
    std::string qualifiedName;
    std::string name;
};

void loadComponents(ObjectStore& store, soci::session& db, Maps& maps)
{
    // soci prepared statements didn't work for null vs value, neither with std::optional.'
    std::string query = R"(select
	sc.id,
	sc.qualified_name,
	sc.name,
	sp.qualified_name as package_name
from
	source_component sc
left join
	source_package sp on sc.package_id = sp.id)";

    ComponentQueryParams params;
    soci::statement st = (db.prepare << query,
                          soci::into(params.id),
                          soci::into(params.qualifiedName),
                          soci::into(params.name),
                          soci::into(params.packageQualifiedName));

    st.execute();
    while (st.fetch()) {
        PackageObject *package = maps.packageMap[params.packageQualifiedName];
        auto *comp = store.getOrAddComponent(params.qualifiedName, params.name, package);
        maps.componentMap[params.id] = comp;

        auto lock = package->rwLock();
        (void) lock;
        package->addComponent(comp);
    }
}

struct FileQueryParams {
    int id = 0;
    std::string package_qualified_name;
    int component_id = 0;
    std::string name;
    std::string qualifiedName;
    int is_header = false; // bool.
    std::string hash;
};

void loadFiles(ObjectStore& store, soci::session& db, Maps& maps)
{
    std::string query = R"(select
	sf.id,
	sp.qualified_name as package_name,
	sf.component_id,
	sf.name,
	sf.qualified_name,
	sf.is_header,
	sf.hash
from
    source_file sf
left join
	source_package sp on sf.package_id = sp.id)";

    FileQueryParams params;
    soci::statement st = (db.prepare << query,
                          soci::into(params.id),
                          soci::into(params.package_qualified_name),
                          soci::into(params.component_id),
                          soci::into(params.name),
                          soci::into(params.qualifiedName),
                          soci::into(params.is_header),
                          soci::into(params.hash));

    st.execute();
    while (st.fetch()) {
        PackageObject *package = maps.packageMap[params.package_qualified_name];
        ComponentObject *comp = maps.componentMap[params.component_id];
        auto *fileObj =
            store.getOrAddFile(params.qualifiedName, params.name, params.is_header, params.hash, package, comp);

        auto lock = comp->rwLock();
        (void) lock;
        comp->addFile(fileObj);
        maps.filesMap[params.id] = fileObj;
    }
}

/* this is a 1-1 mapping of the Sql schema. it clearly shows that we should
 * change this table to have the file_id instead of the file_name.
 */
struct ErrorQueryParams {
    int id = 0;
    int error_kind = 0;
    std::string fully_qualified_name;
    std::string error_message;
    std::string file_name;
};

void loadErrors(ObjectStore& store, soci::session& db, Maps& maps)
{
    std::string query = "select id, error_kind, fully_qualified_name, error_message, file_name from error_messages";
    ErrorQueryParams params;
    soci::statement st = (db.prepare << query,
                          soci::into(params.id),
                          soci::into(params.error_kind),
                          soci::into(params.fully_qualified_name),
                          soci::into(params.error_message),
                          soci::into(params.file_name));

    st.execute();
    while (st.fetch()) {
        // I see no need to store the errors on a map, just load it directly.
        auto kind = static_cast<MdbUtil::ErrorKind>(params.error_kind);
        store.getOrAddError(kind, params.fully_qualified_name, params.error_message, params.file_name);
    }
}

struct NamespaceQueryParams {
    int id = 0;
    soci::indicator parent_qualified_name_ind = soci::indicator::i_null;
    std::string parent_qualified_name;
    std::string name;
    std::string qualified_name;
};

void loadNamespaces(ObjectStore& store, soci::session& db, Maps& maps)
{
    std::string query =
        R"(select
        ns.id,
        ns.name,
        ns.qualified_name,
        pns.qualified_name
    from namespace_declaration ns
    left join namespace_declaration pns on ns.parent_id = pns.id
    order by ns.parent_id ASC)";

    NamespaceQueryParams params;
    soci::statement st = (db.prepare << query,
                          soci::into(params.id),
                          soci::into(params.name),
                          soci::into(params.qualified_name),
                          soci::into(params.parent_qualified_name, params.parent_qualified_name_ind));

    st.execute();
    std::vector<int> current_parents;
    while (st.fetch()) {
        NamespaceObject *parent = nullptr;
        if (params.parent_qualified_name_ind == soci::indicator::i_ok) {
            parent = maps.namespaceMap[params.parent_qualified_name];
        }

        auto *ns = store.getOrAddNamespace(params.qualified_name, params.name, parent);
        current_parents.push_back(params.id);
        maps.namespaceMap[params.qualified_name] = ns;
    }
}

struct VariableQueryParams {
    int id = 0;
    soci::indicator ns_qual_name_ind = soci::indicator::i_null;
    std::string ns_qual_name;
    std::string name;
    std::string qualified_name;
    std::string signature;
    int is_global = 0; // bool.
};

void loadVariables(ObjectStore& store, soci::session& db, Maps& maps)
{
    std::string query =
        R"(select
        va.id,
        va.name,
        va.qualified_name,
        va.signature,
        va.is_global,
        ns.qualified_name
    from
        variable_declaration va
    left join
        namespace_declaration ns on ns.id = va.namespace_id
    )";

    VariableQueryParams params;
    soci::statement st = (db.prepare << query,
                          soci::into(params.id),
                          soci::into(params.name),
                          soci::into(params.qualified_name),
                          soci::into(params.signature),
                          soci::into(params.is_global),
                          soci::into(params.ns_qual_name, params.ns_qual_name_ind));

    st.execute();

    while (st.fetch()) {
        NamespaceObject *ns = nullptr;
        if (params.ns_qual_name_ind != soci::indicator::i_null) {
            ns = maps.namespaceMap[params.ns_qual_name];
        }

        auto *variableObj =
            store.getOrAddVariable(params.qualified_name, params.name, params.signature, params.is_global, ns);
        maps.variableMap[params.id] = variableObj;
    }
}

struct FunctionQueryParams {
    int id = 0;
    soci::indicator ns_qual_name_ind = soci::indicator::i_null;
    std::string ns_qual_name;
    std::string name;
    std::string qualified_name;
    std::string signature;
    std::string rtype;
    std::string template_params;
};

void loadFunctions(ObjectStore& store, soci::session& db, Maps& maps)
{
    std::string query =
        R"(select
            fn.id,
            fn.name,
            fn.qualified_name,
            fn.signature,
            fn.return_type,
            fn.template_parameters,
            ns.qualified_name
        from
            function_declaration fn
        left join
            namespace_declaration ns on ns.id = fn.namespace_id
)";

    FunctionQueryParams params;
    soci::statement st = (db.prepare << query,
                          soci::into(params.id),
                          soci::into(params.name),
                          soci::into(params.qualified_name),
                          soci::into(params.signature),
                          soci::into(params.rtype),
                          soci::into(params.template_params),
                          soci::into(params.ns_qual_name, params.ns_qual_name_ind));

    st.execute();

    while (st.fetch()) {
        NamespaceObject *ns = nullptr;
        if (params.ns_qual_name_ind != soci::indicator::i_null) {
            ns = maps.namespaceMap[params.ns_qual_name];
        }

        auto *functionObj = store.getOrAddFunction(params.qualified_name,
                                                   params.name,
                                                   params.signature,
                                                   params.rtype,
                                                   params.template_params,
                                                   ns);
        maps.functionMap[params.id] = functionObj;
    }
}

struct UserDefinedTypeQueryParams {
    int id = 0;
    soci::indicator parent_ns_qual_name_ind = soci::indicator::i_null;
    std::string parent_ns_qual_name;

    soci::indicator class_namespace_ind = soci::indicator::i_null;
    int class_namespace_id = 0;

    soci::indicator parent_package_ind = soci::indicator::i_null;
    std::string package_qualified_name;
    std::string name;
    std::string qualified_name;
    int kind = 0;
    int access = 0;
};

void loadUserDefinedTypes(ObjectStore& store, soci::session& db, Maps& maps)
{
    // soci prepared statements didn't work for null vs value, neither with std::optional.'
    std::string query =
        R"(select
	cd.id,
	cd.class_namespace_id,
	sp.qualified_name as package_name,
	cd.name,
	cd.qualified_name,
	cd.kind,
    cd.access,
    ns.qualified_name
from
	class_declaration cd
left join
	source_package sp on sp.id = cd.parent_package_id
left join
    namespace_declaration ns on ns.id = cd.parent_namespace_id
order by class_namespace_id ASC)";

    UserDefinedTypeQueryParams params;
    soci::statement st = (db.prepare << query,
                          soci::into(params.id),
                          soci::into(params.class_namespace_id, params.class_namespace_ind),
                          soci::into(params.package_qualified_name, params.parent_package_ind),
                          soci::into(params.name),
                          soci::into(params.qualified_name),
                          soci::into(params.kind),
                          soci::into(params.access),
                          soci::into(params.parent_ns_qual_name, params.parent_ns_qual_name_ind));

    st.execute();
    std::vector<int> current_parents;
    while (st.fetch()) {
        NamespaceObject *parent_ns = nullptr;
        if (params.parent_ns_qual_name_ind == soci::indicator::i_ok) {
            parent_ns = maps.namespaceMap[params.parent_ns_qual_name];
        }

        TypeObject *parent_class = nullptr;
        if (params.class_namespace_ind == soci::indicator::i_ok) {
            parent_class = maps.userDefinedTypeMap[params.class_namespace_id];
        }

        PackageObject *parent_pkg = nullptr;
        if (params.parent_package_ind == soci::indicator::i_ok) {
            parent_pkg = maps.packageMap[params.package_qualified_name];
        }

        auto kind = static_cast<Codethink::lvtshr::UDTKind>(params.kind);
        // TODO: Move CDBUtil::AccessSpecifier to lvtmdb.
        auto accessSpecifier = static_cast<Codethink::lvtshr::AccessSpecifier>(params.access);

        auto *udt = store.getOrAddType(params.qualified_name,
                                       params.name,
                                       kind,
                                       accessSpecifier,
                                       parent_ns,
                                       parent_pkg,
                                       parent_class);
        maps.userDefinedTypeMap[params.id] = udt;
        current_parents.push_back(params.id);

        if (parent_pkg) {
            auto lock = parent_pkg->rwLock();
            (void) lock;
            parent_pkg->addType(udt);
        }
        if (parent_ns) {
            auto lock = parent_ns->rwLock();
            (void) lock;
            parent_ns->addType(udt);
        }
        if (parent_class) {
            auto lock = parent_class->rwLock();
            (void) lock;
            parent_class->addChild(udt);
        }
    }
}

struct FieldQueryParams {
    int id = 0;
    int class_id = 0;
    std::string name;
    std::string qualified_name;
    std::string signature;
    int access = 0;
    int is_static = false; // bool.
};

void loadFields(ObjectStore& store, soci::session& db, Maps& maps)
{
    std::string query =
        "select id, class_id, name, qualified_name, signature, access, is_static from "
        "field_declaration";

    FieldQueryParams params;
    soci::statement st = (db.prepare << query,
                          soci::into(params.id),
                          soci::into(params.class_id),
                          soci::into(params.name),
                          soci::into(params.qualified_name),
                          soci::into(params.signature),
                          soci::into(params.access),
                          soci::into(params.is_static));

    st.execute();

    while (st.fetch()) {
        TypeObject *parent = maps.userDefinedTypeMap[params.class_id];
        auto accessSpecifier = static_cast<Codethink::lvtshr::AccessSpecifier>(params.access);
        auto *field = store.getOrAddField(params.qualified_name,
                                          params.name,
                                          params.signature,
                                          accessSpecifier,
                                          params.is_static,
                                          parent);
        maps.fieldMap[params.id] = field;
    }
}

struct MethodQueryParams {
    int id = 0;
    soci::indicator class_ind = soci::indicator::i_null;
    int class_id = 0;
    std::string name;
    std::string qualified_name;
    std::string signature;
    std::string rtype;
    std::string template_parameters;
    int access = 0;
    int is_virtual = false;
    int is_pure = false;
    int is_static = false;
    soci::indicator const_ind = soci::indicator::i_null;
    int is_const = false;
};

void loadMethods(ObjectStore& store, soci::session& db, Maps& maps)
{
    std::string query =
        "select id, class_id, name, qualified_name, signature, return_type "
        " template_parameters, access, is_virtual, is_pure, is_static, is_const from "
        "method_declaration";

    MethodQueryParams params;
    soci::statement st = (db.prepare << query,
                          soci::into(params.id),
                          soci::into(params.class_id),
                          soci::into(params.name),
                          soci::into(params.qualified_name),
                          soci::into(params.signature),
                          soci::into(params.rtype),
                          soci::into(params.template_parameters),
                          soci::into(params.access),
                          soci::into(params.is_virtual),
                          soci::into(params.is_pure),
                          soci::into(params.is_static),

                          // This looks like it's a bug on soci. `is_const`
                          // is defined as "NOT NULL" but soci is fetching
                          // a null value.
                          soci::into(params.is_const, params.const_ind));

    st.execute();

    while (st.fetch()) {
        TypeObject *parent = maps.userDefinedTypeMap[params.class_id];
        auto accessSpecifier = static_cast<Codethink::lvtshr::AccessSpecifier>(params.access);

        // This looks like it's a bug on soci. `is_const`
        // is defined as "NOT NULL" but soci is fetching
        // a null value.
        if (params.const_ind == soci::indicator::i_null) {
            params.is_const = false;
        }

        auto *method = store.getOrAddMethod(params.qualified_name,
                                            params.name,
                                            params.signature,
                                            params.rtype,
                                            params.template_parameters,
                                            accessSpecifier,
                                            params.is_virtual,
                                            params.is_pure,
                                            params.is_static,
                                            params.is_const,
                                            parent);
        maps.methodMap[params.id] = method;
    }
}

void loadPackageRelations(ObjectStore& store, soci::session& db, Maps& maps)
{
    const std::string query = R"(
select
    target.qualified_name
from
    dependencies dep
left join
    source_package target on target_id = target.id
where dep.source_id = (
	select id
	from source_package
	where qualified_name = :s
) )";

    // Those are all the *database* connections but I guess there's more.
    for (const auto& [qualified_name, _] : maps.packageMap) {
        (void) _;

        PackageObject *source = maps.packageMap[qualified_name];
        std::string target_qual_name;
        soci::statement st = (db.prepare << query, soci::into(target_qual_name), soci::use(qualified_name));
        st.execute();
        while (st.fetch()) {
            PackageObject *target = maps.packageMap[target_qual_name];
            PackageObject::addDependency(target, source);
        }
    }
}

void loadComponentRelations(ObjectStore& store, soci::session& db, const Maps& maps)
{
    int target_id = 0;
    int source_id = 0;
    soci::statement st = (db.prepare << "select source_id, target_id from component_relation",
                          soci::into(source_id),
                          soci::into(target_id));

    st.execute();
    while (st.fetch()) {
        ComponentObject *target = maps.componentMap.at(target_id);
        ComponentObject *source = maps.componentMap.at(source_id);
        ComponentObject::addDependency(source, target);
    }
}

void loadFileRelations(ObjectStore& store, soci::session& db, const Maps& maps)
{
    const std::string query_1 = R"(select target_id from includes where source_id = :s)";
    const std::string query_2 =
        R"(select
            ns.qualified_name
        from namespace_source_file nsf
        left join
            namespace_declaration ns on ns.id = nsf.namespace_id
        where nsf.source_file_id = :s)";

    for (const auto& [source_id, source] : maps.filesMap) {
        int target_id = 0;
        soci::statement st = (db.prepare << query_1, soci::into(target_id), soci::use(source_id));
        st.execute();
        while (st.fetch()) {
            FileObject *target = maps.filesMap.at(target_id);
            FileObject::addIncludeRelation(source, target);
        }

        std::string ns_qual_name;
        soci::statement st2 = (db.prepare << query_2, soci::into(ns_qual_name), soci::use(source_id));
        st2.execute();
        while (st2.fetch()) {
            NamespaceObject *target = maps.namespaceMap.at(ns_qual_name);
            {
                auto lock = source->rwLock();
                source->addNamespace(target);
            }
            {
                auto lock = target->rwLock();
                target->addFile(source);
            }
        }
    }
}

void loadUserDefinedTypeRelations(ObjectStore& store, soci::session& db, const Maps& maps)
{
    {
        int source_id = 0;
        int target_id = 0;
        soci::statement st = (db.prepare << "select source_id, target_id from uses_in_the_interface",
                              soci::into(source_id),
                              soci::into(target_id));
        st.execute();
        while (st.fetch()) {
            TypeObject *source = maps.userDefinedTypeMap.at(source_id);
            TypeObject *target = maps.userDefinedTypeMap.at(target_id);
            TypeObject::addUsesInTheInterface(source, target);
        }
    }

    {
        int source_id = 0;
        int target_id = 0;
        soci::statement st = (db.prepare << "select source_id, target_id from uses_in_the_implementation",
                              soci::into(source_id),
                              soci::into(target_id));
        st.execute();
        while (st.fetch()) {
            TypeObject *source = maps.userDefinedTypeMap.at(source_id);
            TypeObject *target = maps.userDefinedTypeMap.at(target_id);
            TypeObject::addUsesInTheImplementation(source, target);
        }
    }

    {
        int source_id = 0;
        int target_id = 0;
        soci::statement st = (db.prepare << "select source_id, target_id from class_hierarchy",
                              soci::into(source_id),
                              soci::into(target_id));
        st.execute();
        while (st.fetch()) {
            TypeObject *source = maps.userDefinedTypeMap.at(source_id);
            TypeObject *target = maps.userDefinedTypeMap.at(target_id);
            TypeObject::addIsARelationship(source, target);
        }
    }

    {
        int source_id = 0;
        int target_id = 0;
        soci::statement st = (db.prepare << "select udt_id, component_id from udt_component",
                              soci::into(source_id),
                              soci::into(target_id));
        st.execute();
        while (st.fetch()) {
            TypeObject *source = maps.userDefinedTypeMap.at(source_id);
            ComponentObject *target = maps.componentMap.at(target_id);
            {
                auto lock = target->rwLock();
                (void) lock;
                target->addType(source);
            }
            {
                auto lock = source->rwLock();
                (void) lock;
                source->addComponent(target);
            }
        }
    }

    {
        int source_id = 0;
        int target_id = 0;
        soci::statement st = (db.prepare << "select class_id, source_file_id from class_source_file",
                              soci::into(source_id),
                              soci::into(target_id));
        st.execute();
        while (st.fetch()) {
            TypeObject *source = maps.userDefinedTypeMap.at(source_id);
            FileObject *target = maps.filesMap.at(target_id);
            {
                auto lock = target->rwLock();
                (void) lock;
                target->addType(source);
            }
            {
                auto lock = source->rwLock();
                (void) lock;
                source->addFile(target);
            }
        }
    }
}

void loadFieldRelations(ObjectStore& store, soci::session& db, const Maps& maps)
{
    int udt_id = 0;
    int field_id = 0;

    soci::statement st =
        (db.prepare << "select type_class_id, field_id from field_type", soci::into(udt_id), soci::into(field_id));

    while (st.fetch()) {
        FieldObject *source = maps.fieldMap.at(field_id);
        TypeObject *target = maps.userDefinedTypeMap.at(udt_id);
        {
            auto lock = target->rwLock();
            (void) lock;
            target->addField(source);
        }
        {
            auto lock = source->rwLock();
            (void) lock;
            source->addType(target);
        }
    }
}

void loadMethodRelations(ObjectStore& store, soci::session& db, const Maps& maps)
{
    int udt_id = 0;
    int method_id = 0;

    soci::statement st = (db.prepare << "select type_class_id, method_id from method_argument_class",
                          soci::into(udt_id),
                          soci::into(method_id));

    st.execute();
    while (st.fetch()) {
        MethodObject *source = maps.methodMap.at(method_id);
        TypeObject *target = maps.userDefinedTypeMap.at(udt_id);
        auto lock = target->rwLock();
        (void) lock;
        target->addMethod(source);
    }
}

} // end namespace

cpp::result<void, ObjectStore::ReadFromDatabaseError> SociReader::readInto(ObjectStore& store, const std::string& path)
{
    auto lock = store.rwLock();
    (void) lock;

    if (path != ":memory:") {
        if (!std::filesystem::exists(path)) {
            return cpp::fail(ObjectStore::ReadFromDatabaseError{"File doesn't exist on disk"});
        }
    }

    soci::session db;
    db.open(*soci::factory_sqlite3(), path);

    // Currently there's no version check on lvtmdb.'
    //  const int version_key = static_cast<int>(SociHelper::Key::Version);
    const int db_state_key = static_cast<int>(SociHelper::Key::DatabaseState);

    int query_res;
    db << "select value from db_option where key = :k", soci::into(query_res), soci::use(db_state_key);

    // Load the Repositories
    Maps maps;

    loadRepositories(store, db, maps);
    loadPackages(store, db, maps);
    loadComponents(store, db, maps);
    loadFiles(store, db, maps);
    loadErrors(store, db, maps);
    loadNamespaces(store, db, maps);
    loadVariables(store, db, maps);
    loadFunctions(store, db, maps);
    loadUserDefinedTypes(store, db, maps);
    loadFields(store, db, maps);
    loadMethods(store, db, maps);

    // Here we have all the entities, we now need to add the connections between them.
    loadPackageRelations(store, db, maps);
    loadComponentRelations(store, db, maps);
    loadFileRelations(store, db, maps);
    loadUserDefinedTypeRelations(store, db, maps);
    loadFieldRelations(store, db, maps);
    loadMethodRelations(store, db, maps);
    return {};
}
