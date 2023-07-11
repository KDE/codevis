// ct_lvtmdl_modelhelpers.h                                          -*-C++-*-

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

#ifndef DEFINE_CT_LVTMDL_MODELHELPERS
#define DEFINE_CT_LVTMDL_MODELHELPERS

#include <lvtmdl_export.h>

#include <ct_lvtshr_graphenums.h>

#include <QDebug>
#include <QStandardItem>
#include <QString>

#include <memory>
#include <optional>

namespace Codethink::lvtldr {
class LakosianNode;
}

namespace Codethink::lvtmdl {

struct LVTMDL_EXPORT ModelRoles {
    enum Enum {
        e_Id = Qt::UserRole + 1, // represents the database id of the element
        e_State, // represents the lazy-load state, empty or fetched.
        e_IsBranch, // represents if it can carry or not child items
        e_QualifiedName, // represents the fully qualified name of the item (with namespaces)
        e_NodeType, // represents what type the node has, see
        e_RecursiveLakosian, // Represents if all the children of this node are lakosian
        e_ChildItemsLoaded, // If true, all childs are loaded. If false, must lazy load children
    };
};

struct LVTMDL_EXPORT NodeType {
    // Represents the type of a node in the tree model
    enum Enum {
        e_Class, // Represents a UserDefinedType
        e_Namespace, // Represents a NamespaceDeclaration
        e_Package, // Represents a SourcePackage
        e_Repository, // Represents a SourceRepository
        e_Component, // Represents a SourceComponent
        e_Invalid // Only used as a return value for bad conversion
    };

    static lvtshr::DiagramType toDiagramType(NodeType::Enum type);
    static NodeType::Enum fromDiagramType(lvtshr::DiagramType type);
};

struct LVTMDL_EXPORT ModelUtil {
    using ShouldPopulateChildren_f = std::function<bool(Codethink::lvtldr::LakosianNode const&)>;

    static QStandardItem *createTreeItemFromLakosianNode(
        lvtldr::LakosianNode& node,
        std::optional<ShouldPopulateChildren_f> const& shouldPopulateChildren = std::nullopt);
    static void
    populateTreeItemChildren(lvtldr::LakosianNode& node,
                             QStandardItem& item,
                             std::optional<ShouldPopulateChildren_f> const& shouldPopulateChildren = std::nullopt);
}; // namespace ModelUtil

} // end namespace Codethink::lvtmdl

#endif
