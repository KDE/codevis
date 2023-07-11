// ct_lvtldr_alloweddependencyloader.cpp                              -*-C++-*-

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

#include <ct_lvtldr_alloweddependencyloader.h>
#include <ct_lvtldr_lakosiannode.h>
#include <ct_lvtldr_nodestorage.h>

#include <QFile>
#include <QString>
#include <QTextStream>

namespace Codethink::lvtldr {

cpp::result<void, ErrorLoadAllowedDependencies>
processLine(NodeStorage& ns, LakosianNode *parentEntity, LakosianNode *sourceEntity, QString line)
{
    line = line.simplified();
    auto targetQualifiedName = line.toStdString();
    if (parentEntity != nullptr) {
        targetQualifiedName = parentEntity->qualifiedName() + "/" + targetQualifiedName;
    }

    auto *targetEntity = [&]() {
        if (parentEntity == nullptr) {
            // Package group dependency. Target can be either a package group or a standalone package
            // Try package group
            auto *pkgGrpEntity = ns.findByQualifiedName("groups/" + targetQualifiedName);
            if (pkgGrpEntity) {
                return pkgGrpEntity;
            }

            // Assume standalone group
            return ns.findByQualifiedName("standalones/" + targetQualifiedName);
        }

        // Package dependency
        return ns.findByQualifiedName(targetQualifiedName);
    }();

    if (!targetEntity) {
        return {};
    }
    auto result = ns.addPhysicalDependency(sourceEntity, targetEntity, PhysicalDependencyType::AllowedDependency);
    if (result.has_error()) {
        using Kind = ErrorLoadAllowedDependencies::Kind;

        switch (result.error().kind) {
        case ErrorAddPhysicalDependency::Kind::InvalidType:
            return cpp::fail(ErrorLoadAllowedDependencies{Kind::UnexpectedErrorAddingPhysicalDependency});
        case ErrorAddPhysicalDependency::Kind::SelfRelation:
            // User made a self-allowed dependency on de dep file. Just ignore it.
            break;
        case ErrorAddPhysicalDependency::Kind::HierarchyLevelMismatch:
            return cpp::fail(ErrorLoadAllowedDependencies{Kind::UnexpectedErrorAddingPhysicalDependency});
        case ErrorAddPhysicalDependency::Kind::MissingParentDependency:
            return cpp::fail(ErrorLoadAllowedDependencies{Kind::UnexpectedErrorAddingPhysicalDependency});
        case ErrorAddPhysicalDependency::Kind::DependencyAlreadyExists:
            // User added the same dependency more than once. Just ignore it.
            break;
        }
    }
    return {};
}

cpp::result<void, ErrorLoadAllowedDependencies>
readAllowedDependenciesFromFile(NodeStorage& ns, std::string const& filename, LakosianNode *sourceEntity)
{
    using Kind = ErrorLoadAllowedDependencies::Kind;

    QFile inputFile(QString::fromStdString(filename));
    if (!inputFile.exists()) {
        // File doesn't exist - skip load.
        return {};
    }
    if (!inputFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return cpp::fail(ErrorLoadAllowedDependencies{Kind::AllowedDependencyFileCouldNotBeOpen});
    }

    QTextStream in(&inputFile);
    while (!in.atEnd()) {
        auto result = processLine(ns, sourceEntity->parent(), sourceEntity, in.readLine());
        if (result.has_error()) {
            return result;
        }
    }
    inputFile.close();
    return {};
}
} // namespace Codethink::lvtldr

cpp::result<void, Codethink::lvtldr::ErrorLoadAllowedDependencies>
Codethink::lvtldr::loadAllowedDependenciesFromDepFile(NodeStorage& ns, std::filesystem::path const& sourcePath)
{
    auto topLevelPackages = ns.getTopLevelPackages();
    for (auto *pkg : topLevelPackages) {
        auto _readAllowedDependenciesFromFileHelper = [&](auto&& depFile, auto&& pkg) {
            return readAllowedDependenciesFromFile(ns, depFile.string(), pkg);
        };
        if (pkg->isPackageGroup()) {
            auto prefix = sourcePath / "groups" / pkg->name();
            {
                auto result = _readAllowedDependenciesFromFileHelper((prefix / "group" / (pkg->name() + ".dep")), pkg);
                if (result.has_error()) {
                    return result;
                }
            }
            {
                auto result =
                    _readAllowedDependenciesFromFileHelper((prefix / "group" / (pkg->name() + ".t.dep")), pkg);
                if (result.has_error()) {
                    return result;
                }
            }

            for (auto *sourcePkg : pkg->children()) {
                auto name = sourcePkg->name();
                {
                    auto result = _readAllowedDependenciesFromFileHelper((prefix / name / "package" / (name + ".dep")),
                                                                         sourcePkg);
                    if (result.has_error()) {
                        return result;
                    }
                }
                {
                    auto result =
                        _readAllowedDependenciesFromFileHelper((prefix / name / "package" / (name + ".t.dep")),
                                                               sourcePkg);
                    if (result.has_error()) {
                        return result;
                    }
                }
            }
        } else {
            {
                auto result = _readAllowedDependenciesFromFileHelper(
                    (sourcePath / "standalones" / pkg->name() / "package" / (pkg->name() + ".dep")),
                    pkg);
                if (result.has_error()) {
                    return result;
                }
            }
            {
                auto result = _readAllowedDependenciesFromFileHelper(
                    (sourcePath / "standalones" / pkg->name() / "package" / (pkg->name() + ".t.dep")),
                    pkg);
                if (result.has_error()) {
                    return result;
                }
            }
        }
    }

    return {};
}
