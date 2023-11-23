// ct_lvtmdb_objectstore.h                                            -*-C++-*-

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

#ifndef INCLUDED_CT_LVTMDB_OBJECTSTORE
#define INCLUDED_CT_LVTMDB_OBJECTSTORE

//@PURPOSE: Store lvtmdb::DbObjects with fast lookup
//
//@CLASSES:
//  lvtldr::ObjectStore: Append-only store of DbObjects with fast lookup

#include <lvtmdb_export.h>

#include <ct_lvtmdb_lockable.h>
#include <ct_lvtmdb_util.h>

#include <ct_lvtshr_graphenums.h>

#include <QList>

#include <result/result.hpp>

#include <filesystem>
#include <memory>
#include <set>
#include <unordered_map>

namespace Codethink::lvtmdb {

// FORWARD DECLARATIONS
class ComponentObject;
class ErrorObject;
class FieldObject;
class FileObject;
class FunctionObject;
class MethodObject;
class NamespaceObject;
class PackageObject;
class RepositoryObject;
class TypeObject;
class VariableObject;

// ==========================
// class ObjectStore
// ==========================

class LVTMDB_EXPORT ObjectStore : public Lockable {
    // Append-only store of DbObjects with fast lookup
    // The caller must acquire a ROLock or RWLock before calling a const method
    // The caller must acquire a RWLock before calling a non-const method
    // These locks do not need to be held for the duration of object access

  private:
    // TYPES
    struct Private;

    // DATA
    std::unique_ptr<Private> d;

  public:
    // CREATORS
    ObjectStore();

    ~ObjectStore() noexcept override;

    ObjectStore(const ObjectStore& other) = delete;

    enum class State : int {
        // It is important this is an int so it can be stored in DbOption
        Error,
        ManuallyStopped,
        NoneReady,
        PhysicalReady, // Physical parse finished correctly
        PhysicalError, // Parse error, but it should still be able to visualize physical information.
        AllReady, // Physical and logical finished correctly.
        LogicalError // Physical finished correctly, and Logical finished with error.
    };

    void setState(State state);
    // indicates if this ObjectStorage contains partial data, error, is empty, or finished correctly.

    State state() const;

    // ACCESSORS
    [[nodiscard]] std::unordered_map<std::string, std::unique_ptr<ComponentObject>>& components() const;
    [[nodiscard]] std::unordered_map<std::string, std::unique_ptr<ErrorObject>>& errors() const;
    [[nodiscard]] std::unordered_map<std::string, std::unique_ptr<FieldObject>>& fields() const;
    [[nodiscard]] std::unordered_map<std::string, std::unique_ptr<FileObject>>& files() const;
    [[nodiscard]] std::unordered_map<std::string, std::unique_ptr<FunctionObject>>& functions() const;
    [[nodiscard]] std::unordered_map<std::string, std::unique_ptr<MethodObject>>& methods() const;
    [[nodiscard]] std::unordered_map<std::string, std::unique_ptr<NamespaceObject>>& namespaces() const;
    [[nodiscard]] std::unordered_map<std::string, std::unique_ptr<RepositoryObject>>& repositories() const;
    [[nodiscard]] std::unordered_map<std::string, std::unique_ptr<PackageObject>>& packages() const;
    [[nodiscard]] std::unordered_map<std::string, std::unique_ptr<TypeObject>>& types() const;
    [[nodiscard]] std::unordered_map<std::string, std::unique_ptr<VariableObject>>& variables() const;

    // interface to look up by qualified name:
    // Return a matching object or nullptr if nothing was found
    [[nodiscard]] FileObject *getFile(const std::string& qualifiedName) const;
    [[nodiscard]] ComponentObject *getComponent(const std::string& qualifiedName) const;
    [[nodiscard]] RepositoryObject *getRepository(const std::string& qualifiedName) const;
    [[nodiscard]] PackageObject *getPackage(const std::string& qualifiedName) const;
    [[nodiscard]] ErrorObject *
    getError(const std::string& qualifiedName, const std::string& errorMessage, const std::string& fileName) const;
    [[nodiscard]] NamespaceObject *getNamespace(const std::string& qualifiedName) const;
    [[nodiscard]] VariableObject *getVariable(const std::string& qualifiedName) const;
    [[nodiscard]] FunctionObject *getFunction(const std::string& qualifiedName,
                                              const std::string& signature,
                                              const std::string& templateParameters,
                                              const std::string& returnType) const;
    [[nodiscard]] TypeObject *getType(const std::string& qualifiedName) const;
    [[nodiscard]] FieldObject *getField(const std::string& qualifiedName) const;
    [[nodiscard]] MethodObject *getMethod(const std::string& qualifiedName,
                                          const std::string& signature,
                                          const std::string& templateParameters,
                                          const std::string& returnType) const;

    [[nodiscard]] std::vector<PackageObject *> getAllPackages() const;
    [[nodiscard]] std::vector<FileObject *> getAllFiles() const;

    // *do not* call those methods - this is extremely easy to break.
    // there's a correct order to be called, and currently, it all starts
    // with `removeFile` - removeFile will then descend and delete everything
    // from inside of all the other files.
    // return the list of deleted files, since this can propagate to more files.
    QList<std::string> removeFile(FileObject *file, std::set<intptr_t>& removed);
    void removeNamespace(NamespaceObject *nmspc);
    void removeType(TypeObject *type);
    void removeMethod(MethodObject *method);
    void removeField(FieldObject *field);
    void removeFunction(FunctionObject *func);
    void removeVariable(VariableObject *var);
    void removeComponent(ComponentObject *comp);
    void removePackage(PackageObject *pkg, std::set<intptr_t>& removed);

    // MODIFIERS
    // This should be used by a DataWriter - such as SociWriter,
    // to save the contents of this ObjectStore to disk.
    template<typename DatabaseAccessWriter>
    void writeToDatabase(DatabaseAccessWriter& writer)
    {
        writer.writeFrom(*this);
    }

    struct ReadFromDatabaseError {
        std::string what;
    };

    template<typename DatabaseAccessReader>
    cpp::result<void, ReadFromDatabaseError> readFromDatabase(DatabaseAccessReader& reader, std::string db)
    // DatabaseAccessReader is a class that implements readInto(ObjectStore& obj, std::string db);
    // db is a path in disk *or* ":memory:", so we can't use std::filesystem::path'
    {
        return reader.readInto(*this, db);
    }
    // This will call clear(). Be extremely careful there aren't any
    // DbObject pointers left dangling!

    // interface to lookup or construct new DbObjects
    // Returns a pointer to the new object
    FileObject *getOrAddFile(const std::string& qualifiedName,
                             std::string name,
                             bool isHeader,
                             std::string hash,
                             PackageObject *package,
                             ComponentObject *component);

    ComponentObject *getOrAddComponent(const std::string& qualifiedName, std::string name, PackageObject *package);
    RepositoryObject *getOrAddRepository(const std::string& name, const std::string& diskPath);

    PackageObject *getOrAddPackage(const std::string& qualifiedName,
                                   std::string name,
                                   std::string diskPath,
                                   PackageObject *parent,
                                   RepositoryObject *repository);

    ErrorObject *getOrAddError(lvtmdb::MdbUtil::ErrorKind errorKind,
                               std::string qualifiedName,
                               std::string errorMessage,
                               std::string fileName);

    NamespaceObject *getOrAddNamespace(const std::string& qualifiedName, std::string name, NamespaceObject *parent);

    VariableObject *getOrAddVariable(const std::string& qualifiedName,
                                     std::string name,
                                     std::string signature,
                                     bool isGlobal,
                                     NamespaceObject *parent);

    FunctionObject *getOrAddFunction(std::string qualifiedName,
                                     std::string name,
                                     std::string signature,
                                     std::string returnType,
                                     std::string templateParameters,
                                     NamespaceObject *parent);

    TypeObject *getOrAddType(const std::string& qualifiedName,
                             std::string name,
                             lvtshr::UDTKind kind,
                             lvtshr::AccessSpecifier access,
                             NamespaceObject *nmspc,
                             PackageObject *pkg,
                             TypeObject *parent);

    FieldObject *getOrAddField(const std::string& qualifiedName,
                               std::string name,
                               std::string signature,
                               lvtshr::AccessSpecifier access,
                               bool isStatic,
                               TypeObject *parent);

    MethodObject *getOrAddMethod(std::string qualifiedName,
                                 std::string name,
                                 std::string signature,
                                 std::string returnType,
                                 std::string templateParameters,
                                 lvtshr::AccessSpecifier access,
                                 bool isVirtual,
                                 bool isPure,
                                 bool isStatic,
                                 bool isConst,
                                 TypeObject *parent);

    void clear();
};

} // namespace Codethink::lvtmdb

#endif // INCLUDED_CT_LVTMDB_OBJECTSTORE
