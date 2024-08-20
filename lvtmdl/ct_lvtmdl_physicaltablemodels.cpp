// ct_lvtmdl_physicaltablemodels.cpp                                   -*-C++-*-

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

#include <ct_lvtmdl_physicaltablemodels.h>

#include <QDebug>
#include <functional>

namespace Codethink::lvtmdl {

// ================================
// class PhysicalDepsBaseTableModel
// ================================

PhysicalDepsBaseTableModel::PhysicalDepsBaseTableModel(): BaseTableModel(1)
{
    setHeader({"Name"});
}

void PhysicalDepsBaseTableModel::refreshData()
{
    clear();

    if (fullyQualifiedName().empty()) {
        return;
    }

    std::vector<std::string> rows;
    switch (type()) {
    case lvtshr::DiagramType::ClassType:
        return;
    case lvtshr::DiagramType::ComponentType:
        rows = componentRows();
        break;
    case lvtshr::DiagramType::PackageType:
        rows = packageRows();
        break;
    case lvtshr::DiagramType::RepositoryType:
        break;
    case lvtshr::DiagramType::FreeFunctionType:
        return;
    case lvtshr::DiagramType::NoneType:
        qDebug() << "Database Inconsistent: There's no diagram type for"
                 << QString::fromStdString(fullyQualifiedName());
        break;
    }

    for (const std::string& row : rows) {
        addRow(row);
    }
}

// ===============================
// class PhysicalProvidersTableModel
// ===============================

std::vector<std::string> PhysicalProvidersTableModel::componentRows()
{
    assert(type() == lvtshr::DiagramType::ComponentType);
    if (fullyQualifiedName().empty()) {
        return {};
    }

    // TODO: Populate from lvtldr
    //    using DepsType = std::vector<CompPtr>;
    //    return getRows<lvtcdb::SourceComponent, DepsType>(fullyQualifiedName(), session(), compProviders);
    return {};
}

std::vector<std::string> PhysicalProvidersTableModel::packageRows()
{
    assert(type() == lvtshr::DiagramType::PackageType);
    if (fullyQualifiedName().empty()) {
        return {};
    }

    // TODO: Populate from lvtldr
    //    using DepsType = std::vector<ptr<lvtcdb::SourcePackage>>;
    //    return getRows<lvtcdb::SourcePackage, DepsType>(fullyQualifiedName(), session(), pkgProviders);
    return {};
}

// ===============================
// class PhysicalClientsTableModel
// ===============================

std::vector<std::string> PhysicalClientsTableModel::componentRows()
{
    assert(type() == lvtshr::DiagramType::ComponentType);
    if (fullyQualifiedName().empty()) {
        return {};
    }

    // TODO: Populate from lvtldr
    //    using DepsType = std::vector<CompPtr>;
    //    return getRows<lvtcdb::SourceComponent, DepsType>(fullyQualifiedName(), session(), compClients);
    return {};
}

std::vector<std::string> PhysicalClientsTableModel::packageRows()
{
    assert(type() == lvtshr::DiagramType::PackageType);
    if (fullyQualifiedName().empty()) {
        return {};
    }

    // TODO: Populate from lvtldr
    //    using DepsType = std::vector<ptr<lvtcdb::SourcePackage>>;
    //    return getRows<lvtcdb::SourcePackage, DepsType>(fullyQualifiedName(), session(), pkgClients);
    return {};
}

} // namespace Codethink::lvtmdl

#include "moc_ct_lvtmdl_physicaltablemodels.cpp"
