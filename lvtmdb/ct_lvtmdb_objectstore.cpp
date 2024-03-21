// ct_lvtmdb_objectstore.cpp                                           -*-C++-*-

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

#include <cassert>
#include <unordered_map>

#include <ct_lvtmdb_soci_reader.h>
#include <ct_lvtmdb_soci_writer.h>

namespace {

template<class... ARGS>
typename std::unordered_map<ARGS...>::mapped_type lookup(const std::unordered_map<ARGS...>& cache,
                                                         const typename std::unordered_map<ARGS...>::key_type& key)
// Attempt a lookup of a pointer-like thing in an unordered_map, if it isn't
// found, return nullptr
{
    try {
        return cache.at(key);
    } catch (const std::out_of_range&) {
        return nullptr;
    }
}

template<class OBJECT>
OBJECT *lookupByQName(const std::unordered_map<std::string, std::unique_ptr<OBJECT>>& map, const std::string& key)
// It would be really cool if we could implement this in terms of lookup()
// but we can't copy construct a unique_ptr. Returning a reference doesn't
// work because the nullptr return would have nothing to refer to
{
    try {
        return map.at(key).get();
    } catch (const std::out_of_range&) {
        return nullptr;
    }
}

template<class OBJECT>
std::vector<OBJECT *> getAll(const std::unordered_map<std::string, std::unique_ptr<OBJECT>>& map)
{
    std::vector<OBJECT *> out;
    out.reserve(map.size());

    for (const auto& [qualifiedName, ptr] : map) {
        (void) qualifiedName;
        out.push_back(ptr.get());
    }

    return out;
}

template<class OBJECT>
OBJECT *add(std::unordered_map<std::string, std::unique_ptr<OBJECT>>& map,
            std::string qualifiedName,
            std::unique_ptr<OBJECT>&& val)
{
    auto [it, _] = map.emplace(std::move(qualifiedName), std::move(val));
    return it->second.get();
}

} // namespace

namespace Codethink::lvtmdb {

struct ObjectStore::Private {
    template<class OBJECT>
    struct Maps {
        std::unordered_map<std::string, std::unique_ptr<OBJECT>> store;
    };

    Maps<ComponentObject> components;
    Maps<ErrorObject> errors;
    Maps<FieldObject> fields;
    Maps<FileObject> files;
    Maps<FunctionObject> functions;
    Maps<MethodObject> methods;
    Maps<NamespaceObject> namespaces;
    Maps<RepositoryObject> repositories;
    Maps<PackageObject> packages;
    Maps<TypeObject> types;
    Maps<VariableObject> variables;

    ObjectStore::State state = ObjectStore::State::NoneReady;
};

void ObjectStore::clear()
{
    assertWritable();
    d->state = State::NoneReady;
    d->files.store.clear();
    d->components.store.clear();
    d->packages.store.clear();
    d->errors.store.clear();
    d->namespaces.store.clear();
    d->variables.store.clear();
    d->functions.store.clear();
    d->types.store.clear();
    d->fields.store.clear();
    d->methods.store.clear();
}

ObjectStore::ObjectStore(): d(std::make_unique<Private>())
{
}

ObjectStore::~ObjectStore() noexcept = default;

void ObjectStore::setState(State state)
{
    d->state = state;
}

ObjectStore::State ObjectStore::state() const
{
    return d->state;
}

FileObject *ObjectStore::getFile(const std::string& qualifiedName) const
{
    assertReadable();
    return lookupByQName(d->files.store, qualifiedName);
}

ComponentObject *ObjectStore::getComponent(const std::string& qualifiedName) const
{
    assertReadable();
    return lookupByQName(d->components.store, qualifiedName);
}

RepositoryObject *ObjectStore::getRepository(const std::string& qualifiedName) const
{
    assertReadable();
    return lookupByQName(d->repositories.store, qualifiedName);
}

PackageObject *ObjectStore::getPackage(const std::string& qualifiedName) const
{
    assertReadable();
    return lookupByQName(d->packages.store, qualifiedName);
}

ErrorObject *ObjectStore::getError(const std::string& qualifiedName,
                                   const std::string& errorMessage,
                                   const std::string& fileName) const
{
    assertReadable();
    return lookupByQName(d->errors.store, ErrorObject::getStorageKey(qualifiedName, errorMessage, fileName));
}

NamespaceObject *ObjectStore::getNamespace(const std::string& qualifiedName) const
{
    assertReadable();
    return lookupByQName(d->namespaces.store, qualifiedName);
}

VariableObject *ObjectStore::getVariable(const std::string& qualifiedName) const
{
    assertReadable();
    return lookupByQName(d->variables.store, qualifiedName);
}

FunctionObject *ObjectStore::getFunction(const std::string& qualifiedName,
                                         const std::string& signature,
                                         const std::string& templateParameters,
                                         const std::string& returnType) const
{
    assertReadable();
    return lookupByQName(d->functions.store,
                         FunctionObject::getStorageKey(qualifiedName, signature, templateParameters, returnType));
}

TypeObject *ObjectStore::getType(const std::string& qualifiedName) const
{
    assertReadable();
    return lookupByQName(d->types.store, qualifiedName);
}

FieldObject *ObjectStore::getField(const std::string& qualifiedName) const
{
    assertReadable();
    return lookupByQName(d->fields.store, qualifiedName);
}

MethodObject *ObjectStore::getMethod(const std::string& qualifiedName,
                                     const std::string& signature,
                                     const std::string& templateParameters,
                                     const std::string& returnType) const
{
    assertReadable();
    return lookupByQName(d->methods.store,
                         MethodObject::getStorageKey(qualifiedName, signature, templateParameters, returnType));
}

std::vector<PackageObject *> ObjectStore::getAllPackages() const
{
    assertReadable();
    return getAll(d->packages.store);
}

std::vector<FileObject *> ObjectStore::getAllFiles() const
{
    assertReadable();
    return getAll(d->files.store);
}

FileObject *ObjectStore::getOrAddFile(const std::string& qualifiedName,
                                      std::string name,
                                      bool isHeader,
                                      std::string hash,
                                      PackageObject *package,
                                      ComponentObject *component)
{
    assertWritable();
    if (FileObject *ret = getFile(qualifiedName)) {
        return ret;
    }

    return add(
        d->files.store,
        qualifiedName,
        std::make_unique<FileObject>(qualifiedName, std::move(name), isHeader, std::move(hash), package, component));
}

ComponentObject *
ObjectStore::getOrAddComponent(const std::string& qualifiedName, std::string name, PackageObject *package)
{
    assertWritable();
    if (ComponentObject *ret = getComponent(qualifiedName)) {
        return ret;
    }

    return add(d->components.store,
               qualifiedName,
               std::make_unique<ComponentObject>(qualifiedName, std::move(name), package));
}

RepositoryObject *ObjectStore::getOrAddRepository(const std::string& name, const std::string& diskPath)
{
    assertWritable();
    if (auto *ret = getRepository(name)) {
        return ret;
    }

    return add(d->repositories.store, name, std::make_unique<RepositoryObject>(name, name, diskPath));
}

PackageObject *ObjectStore::getOrAddPackage(const std::string& qualifiedName,
                                            std::string name,
                                            std::string diskPath,
                                            PackageObject *parent,
                                            RepositoryObject *repository)
{
    assertWritable();
    if (PackageObject *ret = getPackage(qualifiedName)) {
        return ret;
    }

    return add(
        d->packages.store,
        qualifiedName,
        std::make_unique<PackageObject>(qualifiedName, std::move(name), std::move(diskPath), parent, repository));
}

ErrorObject *ObjectStore::getOrAddError(lvtmdb::MdbUtil::ErrorKind errorKind,
                                        std::string qualifiedName,
                                        std::string errorMessage,
                                        std::string fileName)
{
    assertWritable();
    std::string key = ErrorObject::getStorageKey(qualifiedName, errorMessage, fileName);

    if (ErrorObject *error = lookupByQName(d->errors.store, key)) {
        return error;
    }

    return add(d->errors.store,
               std::move(key),
               std::make_unique<ErrorObject>(errorKind,
                                             std::move(qualifiedName),
                                             std::move(errorMessage),
                                             std::move(fileName)));
}

NamespaceObject *
ObjectStore::getOrAddNamespace(const std::string& qualifiedName, std::string name, NamespaceObject *parent)
{
    assertWritable();
    if (NamespaceObject *ret = getNamespace(qualifiedName)) {
        return ret;
    }

    return add(d->namespaces.store,
               qualifiedName,
               std::make_unique<NamespaceObject>(qualifiedName, std::move(name), parent));
}

VariableObject *ObjectStore::getOrAddVariable(
    const std::string& qualifiedName, std::string name, std::string signature, bool isGlobal, NamespaceObject *parent)
{
    assertWritable();
    if (VariableObject *ret = getVariable(qualifiedName)) {
        return ret;
    }

    return add(
        d->variables.store,
        qualifiedName,
        std::make_unique<VariableObject>(qualifiedName, std::move(name), std::move(signature), isGlobal, parent));
}

FunctionObject *ObjectStore::getOrAddFunction(std::string qualifiedName,
                                              std::string name,
                                              std::string signature,
                                              std::string returnType,
                                              std::string templateParameters,
                                              NamespaceObject *parent)
{
    assertWritable();
    std::string key = FunctionObject::getStorageKey(qualifiedName, signature, templateParameters, returnType);
    if (FunctionObject *fn = lookupByQName(d->functions.store, key)) {
        return fn;
    }

    return add(d->functions.store,
               std::move(key),
               std::make_unique<FunctionObject>(std::move(qualifiedName),
                                                std::move(name),
                                                std::move(signature),
                                                std::move(returnType),
                                                std::move(templateParameters),
                                                parent));
}

TypeObject *ObjectStore::getOrAddType(const std::string& qualifiedName,
                                      std::string name,
                                      lvtshr::UDTKind kind,
                                      lvtshr::AccessSpecifier access,
                                      NamespaceObject *nmspc,
                                      PackageObject *pkg,
                                      TypeObject *parent)
{
    assertWritable();
    if (TypeObject *ret = getType(qualifiedName)) {
        return ret;
    }

    return add(d->types.store,
               qualifiedName,
               std::make_unique<TypeObject>(qualifiedName, std::move(name), kind, access, nmspc, pkg, parent));
}

FieldObject *ObjectStore::getOrAddField(const std::string& qualifiedName,
                                        std::string name,
                                        std::string signature,
                                        lvtshr::AccessSpecifier access,
                                        bool isStatic,
                                        TypeObject *parent)
{
    assertWritable();
    if (FieldObject *ret = getField(qualifiedName)) {
        return ret;
    }

    return add(
        d->fields.store,
        qualifiedName,
        std::make_unique<FieldObject>(qualifiedName, std::move(name), std::move(signature), access, isStatic, parent));
}

MethodObject *ObjectStore::getOrAddMethod(std::string qualifiedName,
                                          std::string name,
                                          std::string signature,
                                          std::string returnType,
                                          std::string templateParameters,
                                          lvtshr::AccessSpecifier access,
                                          bool isVirtual,
                                          bool isPure,
                                          bool isStatic,
                                          bool isConst,
                                          TypeObject *parent)
{
    assertWritable();
    std::string key = MethodObject::getStorageKey(qualifiedName, signature, templateParameters, returnType);
    if (MethodObject *method = lookupByQName(d->methods.store, key)) {
        return method;
    }

    return add(d->methods.store,
               std::move(key),
               std::make_unique<MethodObject>(std::move(qualifiedName),
                                              std::move(name),
                                              std::move(signature),
                                              std::move(returnType),
                                              std::move(templateParameters),
                                              access,
                                              isVirtual,
                                              isPure,
                                              isStatic,
                                              isConst,
                                              parent));
}

void ObjectStore::removeMethod(MethodObject *method)
{
    if (!method) {
        return;
    }

    std::string key;
    method->withROLock([&] {
        key = method->storageKey();
    });
    d->methods.store.erase(key);
}

void ObjectStore::removeField(FieldObject *field)
{
    if (!field) {
        return;
    }

    std::string key;
    field->withROLock([&] {
        key = field->qualifiedName();
    });
    d->fields.store.erase(key);
}

void ObjectStore::removeType(TypeObject *type)
{
    if (!type) {
        return;
    }

    std::string key;
    type->withROLock([&] {
        NamespaceObject *parentNamespace = type->parentNamespace();
        if (parentNamespace) {
            parentNamespace->withRWLock([&] {
                parentNamespace->removeType(type);
            });
        }

        PackageObject *parentPkg = type->package();
        if (parentPkg) {
            parentPkg->withRWLock([&] {
                parentPkg->removeType(type);
            });
        }

        // Type will be locked when it recurses here.
        for (TypeObject *children : type->children()) {
            removeType(children);
        }

        for (MethodObject *method : type->methods()) {
            removeMethod(method);
        }

        for (FieldObject *field : type->fields()) {
            removeField(field);
        }

        key = type->qualifiedName();
    });

    d->types.store.erase(key);
}

void ObjectStore::removeFunction(FunctionObject *func)
{
    if (!func) {
        return;
    }

    NamespaceObject *parent = nullptr;
    std::string key;

    func->withROLock([&] {
        parent = func->parent();
        key = func->storageKey();
    });

    if (parent) {
        parent->removeFunction(func);
    }

    d->functions.store.erase(key);
}

void ObjectStore::removeVariable(VariableObject *var)
{
    if (!var) {
        return;
    }

    NamespaceObject *parent = nullptr;
    std::string key;

    var->withROLock([&] {
        parent = var->parent();
        key = var->qualifiedName();
    });

    if (parent) {
        parent->removeVariable(var);
    }

    d->variables.store.erase(key);
}

void ObjectStore::removeNamespace(NamespaceObject *nmspc)
{
    if (!nmspc) {
        return;
    }

    NamespaceObject *parent = nullptr;
    std::string key;
    nmspc->withROLock([&] {
        parent = nmspc->parent();
        key = nmspc->qualifiedName();

        for (auto *udt : nmspc->typeChildren()) {
            removeType(udt);
        }

        for (auto *func : nmspc->functions()) {
            removeFunction(func);
        }

        for (auto *var : nmspc->variables()) {
            removeVariable(var);
        }
    });

    // no lock here as the recursion will lock it.
    if (parent) {
        parent->removeChild(nmspc);
    }

    d->namespaces.store.erase(key);
}

void ObjectStore::removeComponent(ComponentObject *comp)
{
    if (!comp) {
        return;
    }

    PackageObject *pkg = nullptr;
    std::string key;
    comp->withROLock([&] {
        pkg = comp->package();
        key = comp->qualifiedName();
    });

    if (pkg) {
        bool empty = false;
        pkg->withRWLock([&] {
            pkg->removeComponent(comp);
            empty = pkg->components().empty();
        });

        // TODO: Aparently removing all components from a
        // package still keeps the package alive, and removing it
        // is considered to be a bug - We need to check if that's
        // indeed the case.'
        //
        // See ct_lvtclp_fileupdatemrg_physical.t.cpp
        //
        // if (empty) {
        //    removePackage(pkg);
        //}
    }

    //    for (auto *relation : comp->forwardDependencies()) {
    //        // TODO:
    //    }

    d->components.store.erase(key);
}

void ObjectStore::removePackage(PackageObject *pkg, std::set<intptr_t>& removed)
{
    if (!pkg) {
        return;
    }

    if (removed.count(reinterpret_cast<intptr_t>(pkg)) != 0) {
        return;
    }

    removed.insert(reinterpret_cast<intptr_t>(pkg));

    std::string key;
    PackageObject *parent = nullptr;
    pkg->withROLock([&] {
        parent = pkg->parent();
        key = pkg->qualifiedName();
    });

    if (parent) {
        bool empty = false;
        parent->withRWLock([&] {
            parent->removeChild(pkg);
            empty = parent->children().empty();
        });

        if (empty) {
            removePackage(parent, removed);
        }
    }

    d->packages.store.erase(key);
}

QList<std::string> ObjectStore::removeFile(FileObject *file, std::set<intptr_t>& removed)
{
    if (!file) {
        return {};
    }
    if (removed.count(reinterpret_cast<intptr_t>(file)) != 0) {
        return {};
    }

    removed.insert(reinterpret_cast<intptr_t>(file));

    std::string key;
    std::vector<NamespaceObject *> namespaces;
    std::vector<TypeObject *> types;
    std::vector<FileObject *> files;
    ComponentObject *component = nullptr;

    file->withROLock([&] {
        key = file->qualifiedName();
        namespaces = file->namespaces();
        types = file->types();
        component = file->component();
        files = file->reverseIncludes();
    });

    QList<std::string> res;

    for (auto *nmspc : namespaces) {
        bool empty = false;
        nmspc->withRWLock([&] {
            nmspc->removeFile(file);
            empty = nmspc->files().empty();
        });

        if (empty) {
            removeNamespace(nmspc);
        }
    }

    for (auto *type : types) {
        bool empty = false;
        type->withRWLock([&] {
            type->removeFile(file);
            empty = type->files().empty();
        });

        if (empty) {
            removeType(type);
        }
    }

    // We need to remove all the files that uses this file as include
    // from the object store, to force a re-parse. this has nothing to do
    // with a file that's removed from disk, but as a way to force a parse.
    for (auto& includeRelationship : files) {
        assert(includeRelationship != file);
        res += removeFile(includeRelationship, removed);
    }

    if (component) {
        bool empty = false;
        std::string comp_key;
        component->withRWLock([&] {
            component->removeFile(file);
            empty = component->files().empty();
            comp_key = component->qualifiedName();
        });

        if (empty) {
            removeComponent(component);
        }
    }

    d->files.store.erase(key);
    res.push_back(key);
    return res;
}

std::unordered_map<std::string, std::unique_ptr<ComponentObject>>& ObjectStore::components() const
{
    return d->components.store;
}

std::unordered_map<std::string, std::unique_ptr<ErrorObject>>& ObjectStore::errors() const
{
    return d->errors.store;
}

std::unordered_map<std::string, std::unique_ptr<FieldObject>>& ObjectStore::fields() const
{
    return d->fields.store;
}

std::unordered_map<std::string, std::unique_ptr<FileObject>>& ObjectStore::files() const
{
    return d->files.store;
}

std::unordered_map<std::string, std::unique_ptr<FunctionObject>>& ObjectStore::functions() const
{
    return d->functions.store;
}

std::unordered_map<std::string, std::unique_ptr<MethodObject>>& ObjectStore::methods() const
{
    return d->methods.store;
}

std::unordered_map<std::string, std::unique_ptr<NamespaceObject>>& ObjectStore::namespaces() const
{
    return d->namespaces.store;
}

std::unordered_map<std::string, std::unique_ptr<RepositoryObject>>& ObjectStore::repositories() const
{
    return d->repositories.store;
}

std::unordered_map<std::string, std::unique_ptr<PackageObject>>& ObjectStore::packages() const
{
    return d->packages.store;
}

std::unordered_map<std::string, std::unique_ptr<TypeObject>>& ObjectStore::types() const
{
    return d->types.store;
}

std::unordered_map<std::string, std::unique_ptr<VariableObject>>& ObjectStore::variables() const
{
    return d->variables.store;
}

} // namespace Codethink::lvtmdb
