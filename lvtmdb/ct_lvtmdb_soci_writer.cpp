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

#include <ct_lvtmdb_objectstore.h>
#include <ct_lvtmdb_soci_helper.h>
#include <ct_lvtmdb_soci_writer.h>

#include <soci/soci-backend.h>
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

#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>

#include <QCoreApplication> // for applicationDirPath
#include <QElapsedTimer>
#include <QStandardPaths>

#include <QDebug>
#include <QFile>

using namespace Codethink::lvtmdb;

// must be called on the global namespace.
void initialize_resources()
{
    static bool initialized_resources = false;
    if (initialized_resources) {
        return;
    }
    Q_INIT_RESOURCE(databases);
    initialized_resources = true;
}

namespace {

bool make_sure_file_exist(const std::filesystem::path& db_schema_path, const std::string& db_schema_id)
{
    std::filesystem::path resulting_file = db_schema_path / db_schema_id;

#if 0
// Temporarely disable this check, as it causes problems such as not updating the
// database spec when we need.
// We need a new way so that users can say that they want to override the file.
    if (std::filesystem::exists(resulting_file)) {
        return true;
    }
#endif

    QFile thisFile = QString::fromStdString(":/db/" + db_schema_id);
    if (!thisFile.exists()) {
        qDebug() << "FATAL Error, can't access resource file from Qt - use Gammaray to verify if names are correct";
        return false;
    }

    if (!std::filesystem::exists(db_schema_path)) {
        bool res = std::filesystem::create_directories(db_schema_path);
        if (!res) {
            qDebug() << "error, could not create data folder" << QString::fromStdString(db_schema_path.string());
            return false;
        }
    }

    if (std::filesystem::exists(resulting_file)) {
        try {
            std::filesystem::remove(resulting_file);
        } catch (std::exception& e) {
            // This only happens in windows. something keeps those files open and windows
            // can't delete them. but it's safe to assume that at least, we have a database to use.
            // so even if it's an exception, we can safely return true.
            std::cout << "Exception: " << e.what() << "\n";
            return true;
        }
    }

    bool res = thisFile.copy(QString::fromStdString(resulting_file.string())); // to the filesystem.
    if (!res) {
        qDebug() << "error, could not copy file to our internal filesystem" << thisFile.errorString();
        return false;
    }
    return true;
}

bool run_migration(soci::session& db, const std::string& db_schema_id)
{
    initialize_resources();
    QStringList dataLocations;
    if (const char *env_p = std::getenv("DBSPEC_PATH")) {
        dataLocations << QString::fromStdString(env_p);
    }
    dataLocations << QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

    auto db_schema_path = [&dataLocations, &db_schema_id]() -> std::filesystem::path {
        for (const auto& currPath : qAsConst(dataLocations)) {
            auto db_schema_path = std::filesystem::path(currPath.toStdString());
            if (!make_sure_file_exist(db_schema_path, db_schema_id)) {
                continue;
            }
            return db_schema_path / db_schema_id;
        }

        std::cerr << "Could not find db schema in all the searched paths.\n";
        return {};
    }();
    if (db_schema_path.empty()) {
        return false;
    }

    std::ifstream codebase_schema(db_schema_path);
    std::stringstream buffer;
    buffer << codebase_schema.rdbuf();
    QStringList result_schema = QString::fromStdString(buffer.str())
                                    .split(";",
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
                                           Qt::SkipEmptyParts
#else
                                           QString::SkipEmptyParts
#endif
                                    );

    for (const QString& res : result_schema) {
        if (res.simplified().isEmpty()) {
            continue;
        }

        try {
            db << res.toStdString();
        } catch (const std::exception& e) {
            std::cout << e.what() << "\n";
            return false;
        }
    }

    return true;
}

std::optional<std::pair<int, soci::indicator>>
query_id_from_qual_name(soci::session& db, const std::string& table_name, const std::string& qual_name)
{
    assert(!table_name.empty());

    if (qual_name.empty()) {
        return std::nullopt;
    }

    int id = 0;
    soci::indicator ind = soci::indicator::i_null;
    std::string query = "select id from " + table_name + " where qualified_name = :name";

    db << query, soci::into(id, ind), soci::use(qual_name, "name");

    if (!db.got_data()) {
        return std::nullopt;
    }

    return std::make_pair(id, ind);
}

// Quick and dirty print macro - to be removed when we feel that this
// code is stable enough.
template<typename Obj>
void print(int id, Obj *obj)
{
    std::cout << "Writing" << id << " " << obj->name() << " " << obj->qualifiedName() << "\n";
}

template<typename DbObject, typename callable>
std::pair<int, soci::indicator>
get_or_add_thing(DbObject *dbobj, soci::session& db, const std::string& table_name, callable fn)
{
    int id = 0;
    soci::indicator indicator = soci::indicator::i_null;
    if (!dbobj) {
        return {id, indicator};
    }

    auto lock = dbobj->readOnlyLock();
    (void) lock;

    // TODO:
    // This should never happen but we have a problem with repositories
    // that might be empty, currently.
    // so let's this for a while untill we fix that.'
    if (dbobj->name().empty() || dbobj->qualifiedName().empty()) {
        std::cout << "We are looking for something without name and qualified name.\n";
        std::cout << "returning an empty indicator / null item.\n";
        return {id, indicator};
    }

    std::optional<std::pair<int, soci::indicator>> res =
        query_id_from_qual_name(db, table_name, dbobj->qualifiedName());

    if (!res.has_value()) {
        fn(dbobj, db);
        res = query_id_from_qual_name(db, table_name, dbobj->qualifiedName());
    }

    // here it *needs* to have value.
    id = res.value().first;
    indicator = res.value().second;

    return std::make_pair(id, indicator);
}

void exportRepository(RepositoryObject *obj, soci::session& db)
{
    auto lock = obj->readOnlyLock();
    if (obj->qualifiedName().empty()) {
        return;
    }

    if (query_id_from_qual_name(db, "source_repository", obj->qualifiedName()).has_value()) {
        return;
    }

    db << "insert into source_repository(version, name, qualified_name, disk_path) values (0, :name, :qual_name, "
          ":disk_path)",
        soci::use(obj->name()), soci::use(obj->qualifiedName()), soci::use(obj->diskPath());
}

void exportPackage(PackageObject *pkg, soci::session& db)
{
    auto lock = pkg->readOnlyLock();
    (void) lock;

    if (query_id_from_qual_name(db, "source_package", pkg->qualifiedName()).has_value()) {
        return;
    }

    auto [parent_id, parent_ind] = get_or_add_thing(pkg->parent(), db, "source_package", exportPackage);
    auto [repository_id, repository_ind] =
        get_or_add_thing(pkg->repository(), db, "source_repository", exportRepository);

    std::cout << "Inserting the package " << pkg->name() << " " << pkg->qualifiedName() << "on the database\n";
    db << "insert into source_package(version, parent_id, source_repository_id, name, qualified_name, disk_path)"
          " values (0, :parent_id, :repo_id, :name, :qual_name, :disk_path)",
        soci::use(parent_id, parent_ind), soci::use(repository_id, repository_ind), soci::use(pkg->name()),
        soci::use(pkg->qualifiedName()), soci::use(pkg->diskPath());
}

void exportComponent(ComponentObject *comp, soci::session& db)
{
    auto lock = comp->readOnlyLock();
    (void) lock;

    if (query_id_from_qual_name(db, "source_component", comp->qualifiedName()).has_value()) {
        return;
    }

    auto [pkg_id, pkg_ind] = get_or_add_thing(comp->package(), db, "source_package", exportPackage);

    db << "insert into source_component(version, qualified_name, name, package_id)"
          " values (0, :qual_name, :name, :pkg_id)",
        soci::use(comp->qualifiedName()), soci::use(comp->name()), soci::use(pkg_id, pkg_ind);
}

void exportFile(FileObject *file, soci::session& db)
{
    auto lock = file->readOnlyLock();
    (void) lock;

    if (query_id_from_qual_name(db, "source_file", file->qualifiedName()).has_value()) {
        return;
    }

    auto [pkg_id, pkg_ind] = get_or_add_thing(file->package(), db, "source_package", exportPackage);
    auto [component_id, component_ind] = get_or_add_thing(file->component(), db, "source_component", exportComponent);
    const int isHeader = file->isHeader() ? 1 : 0;

    db << "insert into source_file(version, package_id, component_id, name, qualified_name, is_header, hash)"
          "values(0, :pkg_id, :comp_id, :name, :qual_name, :is_header, :hash)",
        soci::use(pkg_id, pkg_ind), soci::use(component_id, component_ind), soci::use(file->name()),
        soci::use(file->qualifiedName()), soci::use(isHeader), soci::use(file->hash());
}

void exportError(ErrorObject *error, soci::session& db)
{
    auto lock = error->readOnlyLock();
    (void) lock;

    // TODO: this is the *only* table that the `qualified_name` field is named `fully_qualified_name`.
    // if we can reword that, we can simplify this code.
    db << "select id from error_messages where fully_qualified_name = :name", soci::use(error->qualifiedName());
    if (db.got_data()) {
        return;
    }

    const int kind = static_cast<int>(error->errorKind());
    db << "insert into error_messages(version, error_kind, fully_qualified_name, error_message, file_name)"
          "values(0, :error_kind, :qual_name, :error_msg, :file)",
        soci::use(kind), soci::use(error->qualifiedName()), soci::use(error->errorMessage()),
        soci::use(error->fileName());
}

void exportNamespace(NamespaceObject *nmspc, soci::session& db)
{
    auto lock = nmspc->readOnlyLock();
    (void) lock;

    if (query_id_from_qual_name(db, "namespace_declaration", nmspc->qualifiedName()).has_value()) {
        return;
    }

    auto [nspc_parent_id, nspc_parent_ind] =
        get_or_add_thing(nmspc->parent(), db, "namespace_declaration", exportNamespace);

    db << "insert into namespace_declaration(version, parent_id, name, qualified_name)"
          "values(0, :parent, :name, :qual_name)",
        soci::use(nspc_parent_id, nspc_parent_ind), soci::use(nmspc->name()), soci::use(nmspc->qualifiedName());
}

void exportVariable(VariableObject *var, soci::session& db)
{
    auto lock = var->readOnlyLock();
    (void) lock;

    if (query_id_from_qual_name(db, "variable_declaration", var->qualifiedName()).has_value()) {
        return;
    }

    auto [nspc_id, nspc_ind] = get_or_add_thing(var->parent(), db, "namespace_declaration", exportNamespace);
    const int isGlobal = var->isGlobal() ? 1 : 0;
    db << "insert into variable_declaration(version, namespace_id, name, qualified_name, signature, is_global)"
          "values(0, :namespace_id, :name, :qual_name, :sig, :global)",
        soci::use(nspc_id, nspc_ind), soci::use(var->name()), soci::use(var->qualifiedName()),
        soci::use(var->signature()), soci::use(isGlobal);
}

void exportFunction(FunctionObject *fn, soci::session& db)
{
    auto lock = fn->readOnlyLock();
    (void) lock;

    if (query_id_from_qual_name(db, "function_declaration", fn->qualifiedName()).has_value()) {
        return;
    }
    auto [nspc_id, nspc_ind] = get_or_add_thing(fn->parent(), db, "namespace_declaration", exportNamespace);

    db << "insert into function_declaration(version, namespace_id, name, "
          "qualified_name, signature, return_type, template_parameters)"
          "values(0, :namespace_id, :name, :qual_name, :signature, :return_type, :template_parameters)",
        soci::use(nspc_id, nspc_ind), soci::use(fn->name()), soci::use(fn->qualifiedName()), soci::use(fn->signature()),
        soci::use(fn->returnType()), soci::use(fn->templateParameters());
}

// Weird. the class_namespace_id is not a namespace at all, but this is
// what we have on the database chema. it actually points as a declaration.
// a better name would be parent_class or parent_udt.
void exportUserDefinedType(TypeObject *type, soci::session& db)
{
    auto lock = type->readOnlyLock();
    (void) lock;

    if (query_id_from_qual_name(db, "class_declaration", type->qualifiedName()).has_value()) {
        return;
    }

    auto [parent_namespace_id, parent_namespace_ind] =
        get_or_add_thing(type->parentNamespace(), db, "namespace_declaration", exportNamespace);
    auto [class_namespace_id, class_namespace_ind] =
        get_or_add_thing(type->parent(), db, "class_declaration", exportUserDefinedType);
    auto [parent_package_id, parent_package_ind] =
        get_or_add_thing(type->package(), db, "source_package", exportPackage);

    const int kind = static_cast<int>(type->kind());
    const int access = static_cast<int>(type->access());

    db << "insert into class_declaration(version, parent_namespace_id, class_namespace_id, parent_package_id, "
          " name, qualified_name, kind, access)"
          "values(0, :parent_namespace_id, :class_namespace_id, :parent_package_id, :name, :qual_name, :kind, :access)",
        soci::use(parent_namespace_id, parent_namespace_ind), soci::use(class_namespace_id, class_namespace_ind),
        soci::use(parent_package_id, parent_package_ind), soci::use(type->name()), soci::use(type->qualifiedName()),
        soci::use(kind), soci::use(access);
}

void exportField(FieldObject *field, soci::session& db)
{
    auto lock = field->readOnlyLock();
    (void) lock;

    if (query_id_from_qual_name(db, "field_declaration", field->qualifiedName()).has_value()) {
        return;
    }

    auto [class_id, class_ind] = get_or_add_thing(field->parent(), db, "class_declaration", exportUserDefinedType);
    const int access = static_cast<int>(field->access());
    const int isStatic = field->isStatic() ? 1 : 0;

    db << "insert into field_declaration(version, class_id, name, qualified_name, signature, access, is_static)"
          "values(0, :class_id, :name, :qual_name, :signature, :access, :is_static)",
        soci::use(class_id, class_ind), soci::use(field->name()), soci::use(field->qualifiedName()),
        soci::use(field->signature()), soci::use(access), soci::use(isStatic);
}

void exportMethod(MethodObject *method, soci::session& db)
{
    auto lock = method->readOnlyLock();
    (void) lock;

    if (query_id_from_qual_name(db, "method_declaration", method->qualifiedName()).has_value()) {
        return;
    }

    auto [class_id, class_ind] = get_or_add_thing(method->parent(), db, "class_declaration", exportUserDefinedType);
    const int access = static_cast<int>(method->access());

    const int isVirtual = method->isVirtual() ? 1 : 0;
    const int isPure = method->isPure() ? 1 : 0;
    const int isStatic = method->isStatic() ? 1 : 0;
    const int isConst = method->isConst() ? 1 : 0;

    db << "insert into method_declaration(version, class_id, name, qualified_name, "
          "signature, return_type, template_parameters, access, "
          "is_virtual, is_pure, is_static, is_const) values (0, "
          ":class_id, :name, :qual_name, :signature, :return_type, :template_params, :access, :is_virtual, :is_pure, "
          ":is_static, :is_const)",
        soci::use(class_id, class_ind), soci::use(method->name()), soci::use(method->qualifiedName()),
        soci::use(method->signature()), soci::use(method->returnType()), soci::use(method->templateParameters()),
        soci::use(access), soci::use(isVirtual), soci::use(isPure), soci::use(isStatic), soci::use(isConst);
}

void exportPkgRelations(PackageObject *pkg, soci::session& db)
{
    auto lock = pkg->readOnlyLock();
    (void) lock;

    // dependencies. Here we assume that all package queries are correct and that there's no need
    // to check if the sql failed.
    int this_id = query_id_from_qual_name(db, "source_package", pkg->qualifiedName()).value().first;

    for (PackageObject *dep : pkg->forwardDependencies()) {
        auto _lock = dep->readOnlyLock();
        (void) _lock;

        int dep_id = query_id_from_qual_name(db, "source_package", dep->qualifiedName()).value().first;

        db << "select id from dependencies where source_id = :s and target_id = :t limit 1", soci::use(this_id),
            soci::use(dep_id);

        if (db.got_data()) {
            continue;
        }

        db << "insert into dependencies(source_id, target_id) values (:s, :t)", soci::use(this_id), soci::use(dep_id);

        std::cout << "Adding a connection between" << this_id << " and " << dep_id << "\n";
    }
}

void exportCompRelations(ComponentObject *comp, soci::session& db)
{
    auto lock = comp->readOnlyLock();
    (void) lock;

    // types handled in exportUserDefinedTypeRelations
    // dependencies
    int this_id = query_id_from_qual_name(db, "source_component", comp->qualifiedName()).value().first;
    for (ComponentObject *dep : comp->forwardDependencies()) {
        auto _lock = dep->readOnlyLock();
        (void) _lock;

        int dep_id = query_id_from_qual_name(db, "source_component", dep->qualifiedName()).value().first;

        db << "select id from component_relation where source_id = :s and target_id = :t limit 1", soci::use(this_id),
            soci::use(dep_id);

        if (db.got_data()) {
            continue;
        }

        db << "insert into component_relation(source_id, target_id) values (:s, :t)", soci::use(this_id),
            soci::use(dep_id);
    }
}

void exportFileRelations(FileObject *file, soci::session& db)
{
    auto lock = file->readOnlyLock();
    (void) lock;

    int this_id = query_id_from_qual_name(db, "source_file", file->qualifiedName()).value().first;

    // includes
    for (FileObject *include : file->forwardIncludes()) {
        auto _lock = include->readOnlyLock();
        (void) _lock;

        int dep_id = query_id_from_qual_name(db, "source_file", include->qualifiedName()).value().first;

        db << "select id from includes where source_id = :s and target_id = :t limit 1", soci::use(this_id),
            soci::use(dep_id);

        if (db.got_data()) {
            continue;
        }

        db << "insert into includes(source_id, target_id) values (:s, :t)", soci::use(this_id), soci::use(dep_id);
    }

    // namespaces
    for (NamespaceObject *nmspc : file->namespaces()) {
        auto _lock = nmspc->readOnlyLock();
        (void) _lock;

        int dep_id = query_id_from_qual_name(db, "namespace_declaration", nmspc->qualifiedName()).value().first;
        db << "select source_file_id from namespace_source_file where source_file_id = :s and namespace_id = :t limit "
              "1",
            soci::use(this_id), soci::use(dep_id);

        if (db.got_data()) {
            continue;
        }

        db << "insert into namespace_source_file(source_file_id, namespace_id) values (:s, :t)", soci::use(this_id),
            soci::use(dep_id);
    }

    // free functions
    for (FunctionObject *fnc : file->globalFunctions()) {
        auto _lock = fnc->readOnlyLock();
        (void) _lock;

        int dep_id = query_id_from_qual_name(db, "function_declaration", fnc->qualifiedName()).value().first;
        db << "select source_file_id from global_function_source_file where source_file_id = :s and function_id = :t "
              "limit "
              "1",
            soci::use(this_id), soci::use(dep_id);

        if (db.got_data()) {
            continue;
        }

        db << "insert into global_function_source_file(source_file_id, function_id) values (:s, :t)",
            soci::use(this_id), soci::use(dep_id);
    }
}

void exportUserDefinedTypeRelations(TypeObject *type, soci::session& db)
{
    auto lock = type->readOnlyLock();
    (void) lock;

    int this_id = query_id_from_qual_name(db, "class_declaration", type->qualifiedName()).value().first;

    // isA
    for (TypeObject *subclass : type->subclasses()) {
        auto _lock = subclass->readOnlyLock();
        (void) _lock;

        int dep_id = query_id_from_qual_name(db, "class_declaration", subclass->qualifiedName()).value().first;
        db << "select id from class_hierarchy where source_id = :s and target_id = :t limit "
              "1",
            soci::use(this_id), soci::use(dep_id);

        if (db.got_data()) {
            continue;
        }

        db << "insert into class_hierarchy(source_id, target_id) values (:s, :t)", soci::use(this_id),
            soci::use(dep_id);
    }
    // usesInTheInterface
    for (TypeObject *dep : type->usesInTheInterface()) {
        auto _lock = dep->readOnlyLock();
        (void) _lock;

        int dep_id = query_id_from_qual_name(db, "class_declaration", dep->qualifiedName()).value().first;
        db << "select id from uses_in_the_interface where source_id = :s and target_id = :t limit "
              "1",
            soci::use(this_id), soci::use(dep_id);

        if (db.got_data()) {
            continue;
        }

        db << "insert into uses_in_the_interface(source_id, target_id) values (:s, :t)", soci::use(this_id),
            soci::use(dep_id);
    }

    // usesInTheImplementation
    for (TypeObject *dep : type->usesInTheImplementation()) {
        auto _lock = dep->readOnlyLock();
        (void) _lock;

        int dep_id = query_id_from_qual_name(db, "class_declaration", dep->qualifiedName()).value().first;
        db << "select id from uses_in_the_implementation where source_id = :s and target_id = :t limit "
              "1",
            soci::use(this_id), soci::use(dep_id);

        if (db.got_data()) {
            continue;
        }

        db << "insert into uses_in_the_implementation(source_id, target_id) values (:s, :t)", soci::use(this_id),
            soci::use(dep_id);
    }

    for (ComponentObject *comp : type->components()) {
        auto _lock = comp->readOnlyLock();
        (void) _lock;

        int dep_id = query_id_from_qual_name(db, "source_component", comp->qualifiedName()).value().first;
        db << "select component_id from udt_component where udt_id = :s and component_id = :t limit "
              "1",
            soci::use(this_id), soci::use(dep_id);

        if (db.got_data()) {
            continue;
        }

        db << "insert into udt_component(udt_id, component_id) values (:s, :t)", soci::use(this_id), soci::use(dep_id);
    }

    for (FileObject *file : type->files()) {
        auto _lock = file->readOnlyLock();
        (void) _lock;

        int dep_id = query_id_from_qual_name(db, "source_file", file->qualifiedName()).value().first;
        db << "select class_id from class_source_file where class_id = :s and source_file_id = :t limit "
              "1",
            soci::use(this_id), soci::use(dep_id);

        if (db.got_data()) {
            continue;
        }

        db << "insert into class_source_file(class_id, source_file_id) values (:s, :t)", soci::use(this_id),
            soci::use(dep_id);
    }
}

void exportFieldRelations(FieldObject *field, soci::session& db)
{
    auto lock = field->readOnlyLock();
    (void) lock;

    int this_id = query_id_from_qual_name(db, "field_declaration", field->qualifiedName()).value().first;

    // variable types
    for (TypeObject *type : field->variableTypes()) {
        auto _lock = type->readOnlyLock();
        (void) _lock;

        int dep_id = query_id_from_qual_name(db, "class_declaration", type->qualifiedName()).value().first;
        db << "select field_id from field_type where field_id = :s and type_class_id = :t limit "
              "1",
            soci::use(this_id), soci::use(dep_id);

        if (db.got_data()) {
            continue;
        }

        db << "insert into field_type(field_id, type_class_id) values (:s, :t)", soci::use(this_id), soci::use(dep_id);
    }
}

void exportMethodRelations(MethodObject *method, soci::session& db)
{
    auto lock = method->readOnlyLock();
    (void) lock;

    int this_id = query_id_from_qual_name(db, "method_declaration", method->qualifiedName()).value().first;

    // variable types
    for (TypeObject *type : method->argumentTypes()) {
        auto _lock = type->readOnlyLock();
        (void) _lock;

        int dep_id = query_id_from_qual_name(db, "class_declaration", type->qualifiedName()).value().first;
        db << "select method_id from method_argument_class where method_id = :s and type_class_id = :t limit "
              "1",
            soci::use(this_id), soci::use(dep_id);

        if (db.got_data()) {
            continue;
        }

        db << "insert into method_argument_class(method_id, type_class_id) values (:s, :t)", soci::use(this_id),
            soci::use(dep_id);
    }
}

void exportFunctionCallgraph(FunctionObject *fn, soci::session& db)
{
    auto lock = fn->readOnlyLock();
    (void) lock;

    int this_id = query_id_from_qual_name(db, "function_declaration", fn->qualifiedName()).value().first;

    // variable types
    for (FunctionObject *callee : fn->callees()) {
        auto _lock = callee->readOnlyLock();
        (void) _lock;

        int callee_id = query_id_from_qual_name(db, "function_declaration", callee->qualifiedName()).value().first;
        db << "select callee_id from function_calls where caller_id = :s and callee_id = :t limit 1",
            soci::use(this_id), soci::use(callee_id);

        if (db.got_data()) {
            continue;
        }

        db << "insert into function_calls(caller_id, callee_id) values (:s, :t)", soci::use(this_id),
            soci::use(callee_id);
    }
}

} // namespace
namespace Codethink::lvtmdb {

SociWriter::SociWriter() = default;

bool SociWriter::updateDbSchema(const std::string& path, const std::string& schemaPath)
{
    if (!std::filesystem::exists(path)) {
        return createOrOpen(path, schemaPath);
    }

    // not a problem if called multiple times.
    initialize_resources();
    d_db.open(*soci::factory_sqlite3(), path);
    bool res = run_migration(d_db, schemaPath);
    return res;
}

bool SociWriter::createOrOpen(const std::string& path, const std::string& schemaPath)
{
    const bool create_db = path == ":memory:" || !std::filesystem::exists(path);

    d_db.open(*soci::factory_sqlite3(), path);
    if (create_db) {
        const bool res = run_migration(d_db, schemaPath);
        if (!res) {
            return false;
        }
        std::cout << "Database created correctly\n";
    } else {
        std::cout << "Using a pre-existing database\n";
    }

    d_path = path;
    return true;
}

void SociWriter::writeFrom(const ObjectStore& store)
{
    QElapsedTimer timer;
    timer.start();
    assert(!d_path.empty());
    assert(d_db.is_connected());

    std::cout << "Starting to write to the database\n";
    soci::transaction tr(d_db);

    // TODO: To optimize this call, we need to create the queries outside of the
    //  loops, as each query takes a time to be created and validated.
    //  best case scenario is to create all queries in this method, and just
    //  pass the statements to the functions.
    //  this function is taking 17 seconds to run on a small source code.
    for (const auto& [_, repo] : store.repositories()) {
        (void) _;
        exportRepository(repo.get(), d_db);
    }

    for (const auto& [_, pkg] : store.packages()) {
        (void) _;
        exportPackage(pkg.get(), d_db);
    }

    for (const auto& [_, comp] : store.components()) {
        (void) _;
        exportComponent(comp.get(), d_db);
    }

    for (const auto& [_, file] : store.files()) {
        (void) _;
        exportFile(file.get(), d_db);
    }

    for (const auto& [_, error] : store.errors()) {
        (void) _;
        exportError(error.get(), d_db);
    }
    for (const auto& [_, nmspc] : store.namespaces()) {
        (void) _;
        exportNamespace(nmspc.get(), d_db);
    }
    for (const auto& [_, var] : store.variables()) {
        (void) _;
        exportVariable(var.get(), d_db);
    }
    for (const auto& [_, fn] : store.functions()) {
        (void) _;
        exportFunction(fn.get(), d_db);
    }
    for (const auto& [_, type] : store.types()) {
        (void) _;
        exportUserDefinedType(type.get(), d_db);
    }
    for (const auto& [_, field] : store.fields()) {
        (void) _;
        exportField(field.get(), d_db);
    }
    for (const auto& [_, method] : store.methods()) {
        (void) _;
        exportMethod(method.get(), d_db);
    }
    for (const auto& [_, pkg] : store.packages()) {
        (void) _;
        exportPkgRelations(pkg.get(), d_db);
    }

    for (const auto& [_, comp] : store.components()) {
        (void) _;
        exportCompRelations(comp.get(), d_db);
    }

    for (const auto& [_, file] : store.files()) {
        (void) _;
        exportFileRelations(file.get(), d_db);
    }

    for (const auto& [_, type] : store.types()) {
        (void) _;
        exportUserDefinedTypeRelations(type.get(), d_db);
    }

    for (const auto& [_, field] : store.fields()) {
        (void) _;
        exportFieldRelations(field.get(), d_db);
    }

    for (const auto& [_, method] : store.methods()) {
        (void) _;
        exportMethodRelations(method.get(), d_db);
    }

    for (const auto& [_, fn] : store.functions()) {
        (void) _;
        exportFunctionCallgraph(fn.get(), d_db);
    }

    int n = 0;
    d_db << "select count(*) from db_option", soci::into(n);
    if (n == 0) {
        const std::vector<std::pair<int, int>> db_options = {
            {static_cast<int>(SociHelper::Key::Version), SociHelper::CURRENT_VERSION},
            {static_cast<int>(SociHelper::Key::DatabaseState), static_cast<int>(store.state())}};

        for (const auto& [key, val] : db_options) {
            d_db << "insert into db_option(key, value) values(:k, :v)", soci::use(key), soci::use(val);
        }
    }

    tr.commit();
    std::cout << "Ending to write to the database: " << timer.elapsed() << "\n";
}

} // namespace Codethink::lvtmdb
