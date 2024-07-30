// ct_lvtclp_testutil.cpp                                              -*-C++-*-

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

#include "ct_lvtclp_cpp_tool_constants.h"
#include <ct_lvtclp_testutil.h>

#include <ct_lvtclp_logicaldepscanner.h>
#include <ct_lvtclp_logicalpostprocessutil.h>

#include <ct_lvtmdb_componentobject.h>
#include <ct_lvtmdb_fieldobject.h>
#include <ct_lvtmdb_fileobject.h>
#include <ct_lvtmdb_methodobject.h>
#include <ct_lvtmdb_namespaceobject.h>
#include <ct_lvtmdb_objectstore.h>
#include <ct_lvtmdb_packageobject.h>
#include <ct_lvtmdb_typeobject.h>

#include <clang/Tooling/Tooling.h>

#include <QDebug>

#include <algorithm>
#include <cassert>
#include <catch2-local-includes.h>
#include <fstream>
#include <initializer_list>
#include <vector>

namespace {

template<class C>
bool qnameVectorMatches(const std::string& id,
                        const std::vector<C *>& ptrs,
                        const std::vector<std::string>& expectedUnsorted)
{
    if (ptrs.size() != expectedUnsorted.size()) {
        qDebug() << id.c_str() << "Sizes didn't match: Expected size" << expectedUnsorted.size() << ", obtained"
                 << ptrs.size();
        return false;
    }

    std::vector<std::string> qnames;
    qnames.reserve(ptrs.size());

    std::transform(ptrs.begin(), ptrs.end(), std::back_inserter(qnames), [](const auto& ptr) {
        auto lock = ptr->readOnlyLock();
        return ptr->qualifiedName();
    });

    // sort so we don't flag things being in different orders
    std::sort(qnames.begin(), qnames.end());
    std::vector<std::string> expected(expectedUnsorted.begin(), expectedUnsorted.end());
    std::sort(expected.begin(), expected.end());

    bool res = std::equal(expected.begin(), expected.end(), qnames.begin());
    if (!res) {
        qDebug() << id.c_str() << "Vector matches? " << res;

        qDebug() << "Expect:";
        for (const auto& src_include : std::as_const(expected)) {
            qDebug() << "\t" << src_include.c_str();
        }
        qDebug() << "Obtained:";
        for (const auto& src_include : std::as_const(qnames)) {
            qDebug() << "\t" << src_include.c_str();
        }
    }
    return res;
}

template<class C>
bool ptrMatches(C *ptr, const std::string& expected)
{
    if (ptr) {
        bool matches = true;
        ptr->withROLock([&] {
            if (ptr->qualifiedName() != expected) {
                qDebug() << "ptrMatches FAILED. Expected '" << expected.c_str() << "', obtained '"
                         << ptr->qualifiedName().c_str();
                matches = false;
            }
        });
        return matches;
    }

    // ptr is null
    return expected.empty();
}

} // namespace

namespace Codethink::lvtclp {

bool Test_Util::runOnCode(lvtmdb::ObjectStore& mdb, const std::string& source, const std::string& fileName = "file.cpp")
{
    auto callback = [](const std::string&) {};
    auto messageCallback = [](const std::string&, long) {};

    const CppToolConstants constants{
        .prefix = std::filesystem::weakly_canonical(std::filesystem::current_path()).generic_string(),
        .buildPath = {},
        .databasePath = {},
        .nonLakosianDirs = {},
        .thirdPartyDirs = {},
        .ignoreGlobs = {},
        .userProvidedExtraCompileCommandsArgs = {},
        .numThreads = 1,
        .enableLakosianRules = true,
        .printToConsole = false};

    LogicalDepActionFactory actionFactory(mdb, constants, callback, messageCallback);

    auto frontendAction = actionFactory.create();

    const std::vector<std::string> args{
        "-std=c++17", // allow nested namespaces
    };

    bool res = clang::tooling::runToolOnCodeWithArgs(std::move(frontendAction), source, args, fileName);
    if (!res) {
        return res;
    }

    return LogicalPostProcessUtil::postprocess(mdb, false);
}

bool Test_Util::isAExists(const std::string& derivedClassQualifiedName,
                          const std::string& baseClassQualifiedName,
                          lvtmdb::ObjectStore& session)
{
    lvtmdb::TypeObject *base = nullptr;
    lvtmdb::TypeObject *derived = nullptr;

    session.withROLock([&] {
        base = session.getType(baseClassQualifiedName);
        derived = session.getType(derivedClassQualifiedName);
    });

    if (!base) {
        return false;
    }
    if (!derived) {
        return false;
    }

    // check forward relation
    std::vector<lvtmdb::TypeObject *> derivedClasses;
    base->withROLock([&] {
        derivedClasses = base->subclasses();
    });

    auto it = std::find_if(derivedClasses.begin(), derivedClasses.end(), [&derived](lvtmdb::TypeObject *const hier) {
        return hier == derived;
    });

    if (it == derivedClasses.end()) {
        return false;
    }

    // check reverse relation
    std::vector<lvtmdb::TypeObject *> baseClasses;
    derived->withROLock([&] {
        baseClasses = derived->superclasses();
    });

    it = std::find_if(baseClasses.begin(), baseClasses.end(), [&base](const lvtmdb::TypeObject *hier) {
        return hier == base;
    });

    return it != baseClasses.end();
}

bool Test_Util::usesInTheImplementationExists(const std::string& sourceQualifiedName,
                                              const std::string& targetQualifiedName,
                                              lvtmdb::ObjectStore& session)
{
    lvtmdb::TypeObject *source = nullptr;
    lvtmdb::TypeObject *target = nullptr;

    session.withROLock([&] {
        source = session.getType(sourceQualifiedName);
        target = session.getType(targetQualifiedName);
    });

    if (!source) {
        return false;
    }
    if (!target) {
        return false;
    }

    // check forward relation
    std::vector<lvtmdb::TypeObject *> rels;
    source->withROLock([&] {
        rels = source->usesInTheImplementation();
    });

    auto it = std::find_if(rels.begin(), rels.end(), [&target](const auto& rel) {
        return rel == target;
    });

    if (it == rels.end()) {
        return false;
    }

    // check reverse relation
    std::vector<lvtmdb::TypeObject *> revRels;
    target->withROLock([&] {
        revRels = target->revUsesInTheImplementation();
    });

    it = std::find_if(revRels.begin(), revRels.end(), [&source](const auto& revRel) {
        return revRel == source;
    });

    return it != revRels.end();
}

bool Test_Util::usesInTheInterfaceExists(const std::string& sourceQualifiedName,
                                         const std::string& targetQualifiedName,
                                         lvtmdb::ObjectStore& session)
{
    lvtmdb::TypeObject *source = nullptr;
    lvtmdb::TypeObject *target = nullptr;

    session.withROLock([&] {
        source = session.getType(sourceQualifiedName);
        target = session.getType(targetQualifiedName);
    });

    if (!source) {
        return false;
    }

    if (!target) {
        return false;
    }

    // check forward relation
    std::vector<lvtmdb::TypeObject *> rels;
    source->withROLock([&] {
        rels = source->usesInTheInterface();
    });

    auto it = std::find_if(rels.begin(), rels.end(), [&target](const auto& rel) {
        return rel == target;
    });

    if (it == rels.end()) {
        return false;
    }

    // check reverse relation
    std::vector<lvtmdb::TypeObject *> revRels;
    target->withROLock([&] {
        revRels = target->revUsesInTheInterface();
    });

    it = std::find_if(revRels.begin(), revRels.end(), [&source](const auto& revRel) {
        return revRel == source;
    });

    return it != revRels.end();
}

bool Test_Util::classDefinedInFile(const std::string& classQualifiedName,
                                   const std::string& fileQualifiedName,
                                   lvtmdb::ObjectStore& session)
{
    lvtmdb::TypeObject *klass = nullptr;
    lvtmdb::FileObject *file = nullptr;

    session.withROLock([&] {
        klass = session.getType(classQualifiedName);
        file = session.getFile(fileQualifiedName);
    });

    assert(klass);
    assert(file);

    bool ret = false;
    klass->withROLock([&] {
        const std::vector<lvtmdb::FileObject *>& files = klass->files();
        const auto it = std::find(files.begin(), files.end(), file);
        ret = it != files.end();
    });

    return ret;
}

bool Test_Util::createFile(const std::filesystem::path& path, const std::string& contents)
{
    if (std::filesystem::exists(path)) {
        if (!std::filesystem::remove(path)) {
            qDebug() << "Failed to remove path" << path.c_str();
            return false;
        }
    }

    std::filesystem::path parent = path.parent_path();
    if (!std::filesystem::exists(parent)) {
        std::filesystem::create_directories(parent);
    }

    std::ofstream output(path);
    if (output.fail()) {
        qDebug() << "Failed to open" << path.c_str();
        return false;
    }

    output << contents;

    return true;
}

StaticCompilationDatabase::StaticCompilationDatabase(std::initializer_list<clang::tooling::CompileCommand> commands,
                                                     std::filesystem::path directory):
    d_compileCommands(commands), d_directory(std::move(directory))
{
}

StaticCompilationDatabase::StaticCompilationDatabase(std::initializer_list<std::pair<std::string, std::string>> paths,
                                                     const std::string& command,
                                                     const std::vector<std::string>& arguments,
                                                     std::filesystem::path directory):
    d_directory(std::move(directory))
{
    d_compileCommands.reserve(paths.size());

    for (const auto& [inFile, outFile] : paths) {
        auto normalisePath = [&](const auto& path) {
            std::filesystem::path p(path);
            std::filesystem::path full = d_directory / p;
            return std::filesystem::weakly_canonical(full).string();
        };

        std::string normInFile = normalisePath(inFile);
        std::string normOutFile = normalisePath(outFile);

        std::vector<std::string> cmdLine;
        cmdLine.reserve(arguments.size() + 4);

        cmdLine.push_back(command);
        std::copy(arguments.begin(), arguments.end(), std::back_inserter(cmdLine));
        cmdLine.emplace_back(normInFile);
        cmdLine.emplace_back("-o");
        cmdLine.emplace_back(normOutFile);

        d_compileCommands.emplace_back(d_directory.string(), normInFile, std::move(cmdLine), normOutFile);
    }
}

std::vector<clang::tooling::CompileCommand>
StaticCompilationDatabase::getCompileCommands(llvm::StringRef FilePath) const
{
    for (auto const& cmd : d_compileCommands) {
        if (cmd.Filename == FilePath) {
            return {cmd};
        }
    }
    return {};
}

std::vector<std::string> StaticCompilationDatabase::getAllFiles() const
{
    std::vector<std::string> out;
    out.reserve(d_compileCommands.size());

    std::transform(d_compileCommands.begin(),
                   d_compileCommands.end(),
                   std::back_inserter(out),
                   [](const clang::tooling::CompileCommand& cmd) {
                       return cmd.Filename;
                   });

    return out;
}

std::vector<clang::tooling::CompileCommand> StaticCompilationDatabase::getAllCompileCommands() const
{
    return d_compileCommands;
}

bool StaticCompilationDatabase::containsFile(const std::string& file) const
{
    const std::filesystem::path filePath = std::filesystem::weakly_canonical(d_directory / file);

    // this is only test code. It can be slow.
    std::vector<std::string> files = getAllFiles();

    // The compile commands only contain references to the .cpp file, so compare
    // filenames without the file extension so we can still find headers. Lakosian
    // rules don't allow headers without sources or sources without headers
    std::filesystem::path component(filePath);
    component.replace_extension();

    const auto it = std::find_if(files.begin(), files.end(), [&component](const std::string& str) {
        std::filesystem::path iterPath(str);
        iterPath.replace_extension();

        return iterPath == component;
    });
    return it != files.end();
}

bool StaticCompilationDatabase::containsPackage(const std::string& str) const
{
    const std::filesystem::path pkgPath = std::filesystem::weakly_canonical(d_directory / str);

    // this is only test code. It can be slow.
    std::vector<std::string> files = getAllFiles();

    const auto it = std::find_if(files.begin(), files.end(), [&pkgPath](const std::string& file) {
        std::filesystem::path path(file);

        // remember paths look like
        // groups/grp/grppkg/grppkg_component.cpp
        std::filesystem::path pkg = path.parent_path();
        std::filesystem::path pkggrp = pkg.parent_path();

        return pkg == pkgPath || pkggrp == pkgPath;
    });
    return it != files.end();
}

bool ModelUtil::checkSourceFiles(lvtmdb::ObjectStore& store,
                                 const std::initializer_list<ModelUtil::SourceFileModel>& files,
                                 bool printToConsole)
{
    auto storeLock = store.readOnlyLock();
    if (printToConsole) {
        qDebug() << "Store Files:";
        for (auto *innerFile : store.getAllFiles()) {
            auto lock = innerFile->readOnlyLock();
            qDebug() << "\t" << innerFile->qualifiedName().c_str();
        }

        qDebug() << "And Expected files:";
        for (auto const& expected : files) {
            INFO("Processing " << expected.path);
            qDebug() << "\t" << expected.path.c_str();
        }
    }

    for (auto const& expected : files) {
        lvtmdb::FileObject *file = store.getFile(expected.path);
        if (file == nullptr) {
            qDebug() << "File pointer is null.";
            return false;
        }

        bool res = true;
        file->withROLock([&] {
            if (file->isHeader() != expected.isHeader) {
                qDebug() << "File header mismatch." << QString::fromStdString(file->qualifiedName());
                res = false;
                return;
            }
            if (!ptrMatches(file->package(), expected.pkg)) {
                qDebug() << "Invalid file package." << QString::fromStdString(file->qualifiedName());
                res = false;
                return;
            }
            if (!ptrMatches(file->component(), expected.component)) {
                qDebug() << "Invalid file Component." << QString::fromStdString(file->qualifiedName());
                res = false;
                return;
            };
            if (!qnameVectorMatches("Namespaces", file->namespaces(), expected.namespaces)) {
                qDebug() << "Namespace Mismatch" << QString::fromStdString(file->qualifiedName());
                res = false;
                return;
            }
            if (!qnameVectorMatches("Files", file->types(), expected.classes)) {
                qDebug() << "Files Mismatch" << QString::fromStdString(file->qualifiedName());
                res = false;
                return;
            }
        });

        if (!res) {
            return res;
        }
        // REQUIRE(qnameVectorMatchesRelations(file->includeSources(), expected.includes));
    }

    return true;
}

bool ModelUtil::checkComponents(lvtmdb::ObjectStore& store,
                                const std::initializer_list<ModelUtil::ComponentModel>& components)
{
    auto storeLock = store.readOnlyLock();
    for (auto const& expected : components) {
        lvtmdb::ComponentObject *comp = store.getComponent(expected.qualifiedName);
        if (comp == nullptr) {
            qDebug() << "Component is null";
            return false;
        }

        bool res = true;
        comp->withROLock([&] {
            if (comp->name() != expected.name) {
                // qDebug() << "Component name mismatch:" << QString::fromStdString(comp->name()) <<
                // QString::fromStdString(expected.name());
                res = false;
                return;
            }
            if (!qnameVectorMatches("Files", comp->files(), expected.files)) {
                qDebug() << "Files mismatch" << QString::fromStdString(comp->qualifiedName());
                res = false;
                return;
            }
        });

        if (!res) {
            return res;
        }
        // REQUIRE(qnameVectorMatchesRelations(comp->relationSources(), expected.deps));
    }
    return true;
}

bool ModelUtil::checkPackages(lvtmdb::ObjectStore& store, const std::initializer_list<ModelUtil::PackageModel>& pkgs)
{
    auto lock = store.readOnlyLock();
    for (auto const& expected : pkgs) {
        INFO("Checking expected package '" << expected.name << "'...");
        lvtmdb::PackageObject *pkg = store.getPackage(expected.name);
        if (pkg == nullptr) {
            qDebug() << "Package is null";
            return false;
        }
        bool res = true;
        pkg->withROLock([&] {
            if (!ptrMatches(pkg->parent(), expected.parent)) {
                qDebug() << "Package parent mismatch" << QString::fromStdString(pkg->qualifiedName());
                res = false;
                return;
            }
            if (!qnameVectorMatches("Types", pkg->types(), expected.udts)) {
                qDebug() << "Types mismatch" << QString::fromStdString(pkg->qualifiedName());
                res = false;
                return;
            }
            if (!qnameVectorMatches("Components", pkg->components(), expected.components)) {
                qDebug() << "Components mismatch" << QString::fromStdString(pkg->qualifiedName());
                res = false;
                return;
            }
        });

        if (!res) {
            return res;
        }
        // REQUIRE(qnameVectorMatchesRelations(pkg->dependencies(), expected.dependencies));
    }
    return true;
}

bool ModelUtil::checkUDTs(lvtmdb::ObjectStore& store, const std::initializer_list<ModelUtil::UDTModel>& udts)
{
    auto lock = store.readOnlyLock();

    for (auto const& expected : udts) {
        lvtmdb::TypeObject *udt = store.getType(expected.qualifiedName);
        if (udt == nullptr) {
            qDebug() << "UDT is null";
            return false;
        }

        bool res = true;
        udt->withROLock([&] {
            if (!ptrMatches(udt->parentNamespace(), expected.nmspc)) {
                qDebug() << "Namespace mismatch" << QString::fromStdString(udt->qualifiedName());
                res = false;
                return;
            }
            if (!ptrMatches(udt->package(), expected.pkg)) {
                qDebug() << "Package mismatch" << QString::fromStdString(udt->qualifiedName());
                res = false;
                return;
            }
            // REQUIRE(qnameVectorMatchesRelations(udt->usesInTheImplementation(), expected.usesInImpl));
            // REQUIRE(qnameVectorMatchesRelations(udt->usesInTheInterface(), expected.usesInInter));
            // REQUIRE(qnameVectorMatchesRevRelations(udt->parents(), expected.isA));
            if (!qnameVectorMatches("Methods", udt->methods(), expected.methods)) {
                qDebug() << "Methods mismatch" << QString::fromStdString(udt->qualifiedName());
                res = false;
                return;
            }
            if (!qnameVectorMatches("Fields", udt->fields(), expected.fields)) {
                qDebug() << "Fields mismatch" << QString::fromStdString(udt->qualifiedName());
                res = false;
                return;
            }
        });

        if (!res) {
            return res;
        }
    }
    return true;
}

} // namespace Codethink::lvtclp
