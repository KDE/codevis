// ct_lvtldr_typenode.h                                              -*-C++-*-

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

#ifndef DIAGRAM_SERVER_CT_LVTLDR_TYPENODE_H
#define DIAGRAM_SERVER_CT_LVTLDR_TYPENODE_H

#include <ct_lvtldr_databasehandler.h>
#include <ct_lvtldr_lakosiannode.h>
#include <ct_lvtldr_typenodefields.h>

namespace Codethink::lvtldr {

// ==========================
// class TypeNode
// ==========================

class LVTLDR_EXPORT TypeNode : public LakosianNode {
    // Implementation of LakosianNode for UDTs

  private:
    // DATA
    // TODO: Decide if optional should be removed after refactoring is done.
    std::optional<std::reference_wrapper<DatabaseHandler>> d_dbHandler = std::nullopt;
    TypeNodeFields d_fields;
    std::vector<std::string> d_qualifiedNameParts;

  protected:
    explicit TypeNode(NodeStorage& store);

    // MODIFIERS
    void loadParent() override;
    void loadChildren() override;
    void loadProviders() override;
    void loadClients() override;

  public:
    explicit TypeNode(NodeStorage& store,
                      std::optional<std::reference_wrapper<DatabaseHandler>> dbHandler = std::nullopt,
                      std::optional<TypeNodeFields> d_fields = std::nullopt);

    ~TypeNode() noexcept override;

    static inline TypeNode *from(LakosianNode *other)
    {
        return dynamic_cast<TypeNode *>(other);
    }

    inline TypeNodeFields const& getFields()
    {
        return d_fields;
    }

    // ACCESSORS
    void setParentPackageId(Codethink::lvtshr::UniqueId::RecordNumberType id);

    [[nodiscard]] lvtshr::DiagramType type() const override;
    [[nodiscard]] lvtshr::UDTKind kind() const;
    cpp::result<void, AddChildError> addChild(LakosianNode *child) override;
    bool isA(TypeNode *other);
    bool usesInTheImplementation(TypeNode *other);
    bool usesInTheInterface(TypeNode *other);

    [[nodiscard]] std::string qualifiedName() const override;
    [[nodiscard]] std::string parentName() override;
    [[nodiscard]] long long id() const override;
    [[nodiscard]] lvtshr::UniqueId uid() const override;

    [[nodiscard]] IsLakosianResult isLakosian() override;

    [[nodiscard]] bool hasClassNamespace() const;
    [[nodiscard]] lvtshr::UniqueId::RecordNumberType classNamespaceId() const;
};

template<>
struct LakosianNodeType<Codethink::lvtldr::TypeNode> {
    static auto constexpr diagramType = lvtshr::DiagramType::ClassType;
};

} // namespace Codethink::lvtldr

#endif // DIAGRAM_SERVER_CT_LVTLDR_TYPENODE_H
