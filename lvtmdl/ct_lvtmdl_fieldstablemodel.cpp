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

#include <ct_lvtmdl_fieldstablemodel.h>

#include <ct_lvtshr_stringhelpers.h>

namespace Codethink::lvtmdl {

struct FieldsTableModel::Private { };

FieldsTableModel::FieldsTableModel(): BaseTableModel(1), d(std::make_unique<FieldsTableModel::Private>())
{
    setHeader({"Signature"});
}

FieldsTableModel::~FieldsTableModel() = default;

void FieldsTableModel::refreshData()
{
    clear();
    if (fullyQualifiedName().empty()) {
        return;
    }

    if (type() != lvtshr::DiagramType::ClassType) {
        return;
    }

    // TODO: Retrieve fields table information from lvtldr
    //    Transaction transaction(*session());
    //
    //    ptr<lvtcdb::UserDefinedType> userDefinedTypePtr =
    //        lvtcdb::CdbUtil::getByQualifiedName<lvtcdb::UserDefinedType>(fullyQualifiedName(), session());
    //
    //    if (!userDefinedTypePtr) {
    //        qDebug() << "No Class Declaration for" << QString::fromStdString(fullyQualifiedName());
    //        return;
    //    }
    //
    //    lvtcdb::CdbUtil::FieldDeclarationList fields = userDefinedTypePtr->variables();
    //
    //    for (const auto& field : fields) {
    //        addRow(field->fieldSignature());
    //    }
}

} // end namespace Codethink::lvtmdl
