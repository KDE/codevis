// ct_lvtclp_dumpdatabase.m.cpp                                        -*-C++-*-

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

// Prints a database to stdout in a sorted, human readable format.
// This is useful for calculating diffs of databases (sqlite's .DUMP command
// isn't useful because it shows the database IDs).
//
// Obviously, this outputs a huge amount of text.
#include "ct_lvtmdb_methodobject.h"
#include <ct_lvtmdb_componentobject.h>
#include <ct_lvtmdb_fieldobject.h>
#include <ct_lvtmdb_fileobject.h>
#include <ct_lvtmdb_namespaceobject.h>
#include <ct_lvtmdb_objectstore.h>
#include <ct_lvtmdb_packageobject.h>
#include <ct_lvtmdb_soci_reader.h>
#include <ct_lvtmdb_typeobject.h>

#include <ct_lvtprj_projectfile.h>

#include <ct_lvtshr_stringhelpers.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <vector>

#include <QDebug>

using namespace Codethink;
using namespace Codethink::lvtmdb;

static void printHelp(const char *execName)
{
    qDebug() << "Dump database to stdout";
    qDebug() << "USAGE: " << execName << " <PATH>";
    qDebug() << "ARGUMENTS:";
    qDebug() << "PATH\t\tPath to the project file";
}

template<typename T>
static void sort(std::vector<T *>& vec)
{
    std::sort(vec.begin(), vec.end(), [](const auto& a, const auto& b) {
        auto a_lock = a->readOnlyLock();
        (void) a_lock;
        auto b_lock = b->readOnlyLock();
        (void) b_lock;

        return a->qualifiedName() < b->qualifiedName();
    });
}

static void printFile(FileObject *file)
{
    auto lock = file->readOnlyLock();
    (void) lock;
    qDebug() << "  " << file->qualifiedName();

    const auto& parent = file->package();
    if (parent) {
        auto p_lock = parent->readOnlyLock();
        (void) p_lock;
        qDebug() << "    PARENT: " << parent->qualifiedName();
    }

    std::vector<NamespaceObject *> nmspcs = file->namespaces();
    sort(nmspcs);
    if (!nmspcs.empty()) {
        qDebug() << "    NAMESPACES:";
    }
    for (const auto& nmspc : nmspcs) {
        auto n_lock = nmspc->readOnlyLock();
        (void) n_lock;
        qDebug() << "      " << nmspc->qualifiedName();
    }

    std::vector<TypeObject *> classes = file->types();
    sort(classes);
    if (!classes.empty()) {
        qDebug() << "    CLASSES:";
    }
    for (const auto& klass : classes) {
        auto n_lock = klass->readOnlyLock();
        (void) n_lock;
        qDebug() << "      " << klass->qualifiedName();
    }

    std::vector<FileObject *> includes = file->forwardIncludes();
    sort(includes);
    if (!includes.empty()) {
        qDebug() << "    INCLUDES:";
    }

    for (const auto& include : includes) {
        auto n_lock = include->readOnlyLock();
        (void) n_lock;
        qDebug() << "      " << include->qualifiedName();
    }

    qDebug();
}

static void printFiles(ObjectStore& session)
{
    qDebug() << "SOURCE FILES:";
    for (const auto& [_, file] : session.files()) {
        (void) _;
        printFile(file.get());
    }
}

static void printPackage(PackageObject *package)
{
    auto lock = package->readOnlyLock();
    (void) lock;
    qDebug() << "  " << package->qualifiedName();

    PackageObject *parent = package->parent();
    if (parent) {
        qDebug() << "    PARENT: " << parent->qualifiedName();
    }

    std::vector<TypeObject *> classes = package->types();
    sort(classes);
    if (!classes.empty()) {
        qDebug() << "    CLASSES:";
    }

    for (const auto& klass : classes) {
        auto k_lock = klass->readOnlyLock();
        (void) k_lock;
        qDebug() << "      " << klass->qualifiedName();
    }

    std::vector<PackageObject *> dependencies = package->forwardDependencies();

    sort(dependencies);
    if (!dependencies.empty()) {
        qDebug() << "    DEPENDENCIES:";
    }
    for (const auto& pkgDep : dependencies) {
        auto p_lock = pkgDep->readOnlyLock();
        (void) p_lock;
        qDebug() << "      " << pkgDep->qualifiedName();
    }

    std::vector<ComponentObject *> components = package->components();
    sort(components);
    if (!components.empty()) {
        qDebug() << "    COMPONENTS:";
    }
    for (ComponentObject *component : components) {
        auto c_lock = component->readOnlyLock();
        (void) c_lock;
        qDebug() << "      " << component->qualifiedName();
    }

    qDebug();
}

static void printPackages(ObjectStore& session)
{
    qDebug() << "PACKAGES:";
    for (const auto& [_, package] : session.packages()) {
        (void) _;
        printPackage(package.get());
    }
}

static void printClass(TypeObject *klass)
{
    auto lock = klass->readOnlyLock();
    (void) lock;

    qDebug() << "  " << klass->qualifiedName();

    std::vector<FileObject *> files = klass->files();
    if (!files.empty()) {
        qDebug() << "    FILES:";
    }
    for (const auto& file : files) {
        auto lockFile = file->readOnlyLock();
        (void) lockFile;
        qDebug() << "      " << file->qualifiedName();
    }

    const auto *nmspc = klass->parentNamespace();
    if (nmspc) {
        qDebug() << "    NAMESPACE: " << nmspc->qualifiedName();
    }

    const auto *pkg = klass->package();
    if (pkg) {
        qDebug() << "    PACKAGE: " << pkg->qualifiedName();
    }

    const auto *parentClass = klass->parent();
    if (parentClass) {
        qDebug() << "    PARENT CLASS: " << parentClass->qualifiedName();
    }

    // Is-A relationships
    std::vector<TypeObject *> isAs = klass->superclasses();

    if (!isAs.empty()) {
        qDebug() << "    IS-A RELATIONSHPS:";
    }
    for (TypeObject *superClass : isAs) {
        auto c_lock = superClass->readOnlyLock();
        (void) c_lock;
        qDebug() << "      " << superClass->qualifiedName();
    }

    std::vector<TypeObject *> usesInInters = klass->usesInTheInterface();
    sort(usesInInters);
    if (!usesInInters.empty()) {
        qDebug() << "    USES-IN-THE-INTERFACE RELATIONSHIPS:";
    }
    for (const auto& dep : usesInInters) {
        qDebug() << "      " << dep->qualifiedName();
    }

    std::vector<TypeObject *> usesInImpls = klass->usesInTheImplementation();
    sort(usesInImpls);
    if (!usesInImpls.empty()) {
        qDebug() << "    USES-IN-THE-IMPLEMENTATION RELATIONSHIPS:";
    }
    for (const auto& dep : usesInImpls) {
        qDebug() << "      " << dep->qualifiedName();
    }

    std::vector<MethodObject *> methods = klass->methods();
    sort(methods);
    if (!methods.empty()) {
        qDebug() << "    METHODS:";
    }
    for (const auto& method : methods) {
        auto m_lock = method->readOnlyLock();
        (void) m_lock;
        qDebug() << "      " << method->qualifiedName();
    }

    std::vector<FieldObject *> fields = klass->fields();
    sort(fields);
    if (!fields.empty()) {
        qDebug() << "    FIELDS:";
    }
    for (const auto& field : fields) {
        auto f_lock = field->readOnlyLock();
        (void) f_lock;
        qDebug() << "      " << field->qualifiedName();
    }

    qDebug();
}

static void printClasses(ObjectStore& session)
{
    qDebug() << "CLASSES:";
    for (const auto& [_, klass] : session.types()) {
        (void) _;
        printClass(klass.get());
    }
}

static void printDatabase(ObjectStore& session)
{
    auto lock = session.readOnlyLock();
    (void) lock;
    printFiles(session);
    printPackages(session);
    printClasses(session);
}

int main(int argc, const char **argv)
{
    const char *const prog = argv[0];
    if (argc != 2) {
        printHelp(prog);
        return EXIT_FAILURE;
    }

    const char *const arg = argv[1];

    if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
        printHelp(prog);
        return EXIT_SUCCESS;
    }

    if (!std::filesystem::is_regular_file(arg)) {
        qDebug() << arg << " is not a file";
        return EXIT_FAILURE;
    }

    lvtprj::ProjectFile projectFile;
    auto projectOpened = projectFile.open(arg);
    if (projectOpened.has_error()) {
        qDebug() << "Error opening project file" << projectOpened.error().errorMessage;
        return EXIT_FAILURE;
    }

    lvtmdb::SociReader reader;
    ObjectStore store;
    auto res = store.readFromDatabase(reader, projectFile.databasePath().string());
    if (res.has_error()) {
        qDebug() << "Error opening database: " << res.error().what << "\n";
        return EXIT_FAILURE;
    }

    printDatabase(store);

    return EXIT_SUCCESS;
}
