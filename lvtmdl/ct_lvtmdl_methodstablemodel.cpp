// ct_lvtmdl_fieldstablemodel.cpp                                         -*-C++-*-

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

#include <ct_lvtmdl_methodstablemodel.h>

namespace Codethink::lvtmdl {

struct MethodsTableModel::Private { };

MethodsTableModel::MethodsTableModel(): BaseTableModel(1), d(std::make_unique<MethodsTableModel::Private>())
{
    setHeader({"Signature"});
}

MethodsTableModel::~MethodsTableModel() = default;

void MethodsTableModel::refreshData()
{
    clear();
    if (fullyQualifiedName().empty()) {
        return;
    }

    if (type() != lvtshr::DiagramType::ClassType) {
        return;
    }

    // TODO: Populate from lvtldr
    //    Transaction transaction(*session());
    //
    //    ptr<lvtcdb::UserDefinedType> userDefinedTypePtr =
    //        lvtcdb::CdbUtil::getByQualifiedName<lvtcdb::UserDefinedType>(fullyQualifiedName(), session());
    //
    //    if (!userDefinedTypePtr) {
    //        qDebug() << "No Class Declaration for" << fullyQualifiedName();
    //        return;
    //    }
    //
    //    lvtcdb::CdbUtil::MethodDeclarationList methods = userDefinedTypePtr->methods();
    //    for (auto& method : methods) {
    //        addRow(method->methodSignature());
    //    }
}

} // end namespace Codethink::lvtmdl
