// ct_lvtclp_logicalpostprocessutil.cpp                               -*-C++-*-

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

#include <ct_lvtclp_logicalpostprocessutil.h>

#include <ct_lvtshr_fuzzyutil.h>

#include <ct_lvtmdb_componentobject.h>
#include <ct_lvtmdb_fileobject.h>
#include <ct_lvtmdb_namespaceobject.h>
#include <ct_lvtmdb_objectstore.h>
#include <ct_lvtmdb_packageobject.h>
#include <ct_lvtmdb_typeobject.h>

#include <algorithm>
#include <cctype>
#include <iostream>
#include <limits>
#include <string>

#include <QDebug>

namespace {

using namespace Codethink::lvtmdb;
using Codethink::lvtshr::FuzzyUtil;

// We already have the lock for the file, do not lock again.
void setUdtFile(TypeObject *udt, FileObject *file, bool debugOutput)
{
    ComponentObject *comp = nullptr;
    PackageObject *pkg = nullptr;
    file->withROLock([&] {
        pkg = file->package();
        comp = file->component();
    });

    udt->withRWLock([&] {
        udt->setUniqueFile(file);
        udt->setUniqueComponent(comp);
        udt->setPackage(pkg);
    });

    if (debugOutput) {
        auto udtQName = udt->withROLock([&]() {
            return QString::fromStdString(udt->qualifiedName());
        });
        auto fileQName = file->withROLock([&]() {
            return QString::fromStdString(file->qualifiedName());
        });
        qDebug() << "Set file for " << udtQName << " to " << fileQName;
    }
}

bool fixUdt(TypeObject *udt, bool debugOutput)
{
    int numFiles = 0;
    std::string loweredClassName;
    NamespaceObject *nmspc = nullptr;

    udt->withROLock([&] {
        numFiles = static_cast<int>(udt->files().size());
        loweredClassName = udt->name();
        nmspc = udt->parentNamespace();
    });

    if (numFiles == 0) {
        if (debugOutput) {
            auto udtQName = udt->withROLock([&]() {
                return QString::fromStdString(udt->qualifiedName());
            });
            qDebug() << "WARN: UDT " << udtQName << " has no source files";
        }
        // this is just a warning not a database killing error so return success
        return true;
    }
    if (numFiles == 1) {
        // class doesn't need fixing
        return true;
    }

    std::transform(loweredClassName.begin(), loweredClassName.end(), loweredClassName.begin(), ::tolower);

    FileObject *bestFile = nullptr;
    if (nmspc) {
        std::string expectedFilename;
        nmspc->withROLock([&] {
            expectedFilename = nmspc->name() + '_' + loweredClassName + ".";
        });

        bool found = false;
        udt->withRWLock([&] {
            for (FileObject *file : udt->files()) {
                std::string thisFilename;
                file->withROLock([&] {
                    thisFilename = file->name();
                });

                if (thisFilename.find(expectedFilename) != std::string::npos) {
                    // we found the right one! This should be the only file for that
                    // type
                    bestFile = file;
                    found = true;
                }
            }
        });
        if (found) {
            setUdtFile(udt, bestFile, debugOutput);
            return true;
        }
    }

    // second try: which of the files has a name most similar to the class
    // name?
    size_t lowest_distance = std::numeric_limits<size_t>::max();
    udt->withROLock([&] {
        for (FileObject *file : udt->files()) {
            file->withROLock([&] {
                size_t dist = FuzzyUtil::levensteinDistance(loweredClassName, file->name());
                if (dist < lowest_distance) {
                    lowest_distance = dist;
                    bestFile = file;
                }
            });
        }
    });

    setUdtFile(udt, bestFile, debugOutput);

    return true;
}

} // namespace

namespace Codethink::lvtclp {

bool LogicalPostProcessUtil::postprocess(lvtmdb::ObjectStore& store, bool debugOutput)
{
    bool success = true;

    for (const auto& [_, udt] : store.types()) {
        (void) _;
        success &= fixUdt(udt.get(), debugOutput);
    }

    return success;
}

} // namespace Codethink::lvtclp
