// ct_lvtldr_componentnode.h                                         -*-C++-*-

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

#ifndef DIAGRAM_SERVER_CT_LVTLDR_COMPONENTNODE_H
#define DIAGRAM_SERVER_CT_LVTLDR_COMPONENTNODE_H

#include <ct_lvtldr_databasehandler.h>
#include <ct_lvtldr_lakosiannode.h>
#include <ct_lvtldr_packagenode.h>
#include <ct_lvtldr_typenode.h>

namespace Codethink::lvtldr {

// ==========================
// class ComponentNode
// ==========================

class LVTLDR_EXPORT ComponentNode : public LakosianNode {
    // Implementation of LakosianNode for components

  private:
    // DATA
    // TODO: Decide if optional should be removed after refactoring is done.
    std::optional<std::reference_wrapper<DatabaseHandler>> d_dbHandler = std::nullopt;
    ComponentNodeFields d_fields;
    std::vector<std::string> d_qualifiedNameParts;

  protected:
    explicit ComponentNode(NodeStorage& store);

    // MODIFIERS
    void loadParent() override;
    void loadChildrenIds() override;
    void loadChildren() override;
    void loadProviders() override;
    void loadClients() override;

  public:
    explicit ComponentNode(NodeStorage& store,
                           std::optional<std::reference_wrapper<DatabaseHandler>> dbHandler = std::nullopt,
                           std::optional<ComponentNodeFields> fields = std::nullopt);

    ~ComponentNode() noexcept override;

    static inline ComponentNode *from(LakosianNode *other)
    {
        return dynamic_cast<ComponentNode *>(other);
    }

    inline ComponentNodeFields const& getFields()
    {
        return d_fields;
    }

    // ACCESSORS
    void setParentPackageId(Codethink::lvtshr::UniqueId::RecordNumberType id);
    void addConcreteDependency(ComponentNode *other);
    void removeConcreteDependency(ComponentNode *other);
    void removeChild(LakosianNode *child);

    [[nodiscard]] lvtshr::DiagramType type() const override;
    cpp::result<void, AddChildError> addChild(LakosianNode *child) override;

    [[nodiscard]] std::string qualifiedName() const override;
    [[nodiscard]] std::string parentName() override;
    [[nodiscard]] long long id() const override;
    [[nodiscard]] lvtshr::UniqueId uid() const override;

    [[nodiscard]] IsLakosianResult isLakosian() override;
};

template<>
struct LakosianNodeType<Codethink::lvtldr::ComponentNode> {
    static auto constexpr diagramType = lvtshr::DiagramType::ComponentType;
};

} // namespace Codethink::lvtldr

#endif // DIAGRAM_SERVER_CT_LVTLDR_COMPONENTNODE_H
