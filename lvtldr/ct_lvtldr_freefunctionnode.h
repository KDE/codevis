// ct_lvtldr_freefunctionnode.h                                              -*-C++-*-

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

#ifndef DIAGRAM_SERVER_CT_LVTLDR_FREEFUNCTIONNODE_H
#define DIAGRAM_SERVER_CT_LVTLDR_FREEFUNCTIONNODE_H

#include <ct_lvtldr_databasehandler.h>
#include <ct_lvtldr_freefunctionnodefields.h>
#include <ct_lvtldr_lakosiannode.h>

namespace Codethink::lvtldr {

// ==========================
// class FreeFunctionNode
// ==========================

class LVTLDR_EXPORT FreeFunctionNode : public LakosianNode {
  private:
    // DATA
    std::optional<std::reference_wrapper<DatabaseHandler>> d_dbHandler = std::nullopt;
    FreeFunctionNodeFields d_fields;
    std::vector<std::string> d_qualifiedNameParts;

  protected:
    explicit FreeFunctionNode(NodeStorage& store);

  public:
    explicit FreeFunctionNode(NodeStorage& store,
                              std::optional<std::reference_wrapper<DatabaseHandler>> dbHandler = std::nullopt,
                              std::optional<FreeFunctionNodeFields> d_fields = std::nullopt);

    ~FreeFunctionNode() noexcept override;

    static inline FreeFunctionNode *from(LakosianNode *other)
    {
        return dynamic_cast<FreeFunctionNode *>(other);
    }

    inline FreeFunctionNodeFields const& getFields()
    {
        return d_fields;
    }

    // MODIFIERS
    void loadParent() override;

    void loadChildren() override
    {
        d->childrenLoaded = true;
        // There are no expected children for free functions.
    }

    void loadProviders() override;
    void loadClients() override;

    // ACCESSORS
    [[nodiscard]] lvtshr::DiagramType type() const override;
    [[nodiscard]] std::string qualifiedName() const override;
    [[nodiscard]] std::string parentName() override;
    [[nodiscard]] long long id() const override;
    [[nodiscard]] lvtshr::UniqueId uid() const override;
    [[nodiscard]] IsLakosianResult isLakosian() override;
    [[nodiscard]] cpp::result<void, AddChildError> addChild(LakosianNode *) override
    {
        // We don't currently support adding childs to free functions (what does that even mean?)
        return {};
    }
};

template<>
struct LakosianNodeType<Codethink::lvtldr::FreeFunctionNode> {
    static auto constexpr diagramType = lvtshr::DiagramType::FreeFunctionType;
};

} // namespace Codethink::lvtldr

#endif // DIAGRAM_SERVER_CT_LVTLDR_FREEFUNCTIONNODE_H
