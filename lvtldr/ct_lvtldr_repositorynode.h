// ct_lvtldr_repositorynode.h                                        -*-C++-*-

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

#ifndef DIAGRAM_SERVER_CT_LVTLDR_REPOSITORYNODE_H
#define DIAGRAM_SERVER_CT_LVTLDR_REPOSITORYNODE_H

#include <ct_lvtldr_lakosiannode.h>
#include <ct_lvtldr_nodestorage.h>

namespace Codethink::lvtldr {

// ==========================
// class RepositoryNode
// ==========================

class LVTLDR_EXPORT RepositoryNode : public LakosianNode {
  private:
    // DATA
    // TODO: Decide if optional should be removed after refactoring is done.
    std::optional<std::reference_wrapper<DatabaseHandler>> d_dbHandler = std::nullopt;
    RepositoryNodeFields d_fields;

  protected:
    // MODIFIERS
    void loadParent() override;
    void loadChildren() override;
    void loadProviders() override;
    void loadClients() override;

  public:
    explicit RepositoryNode(NodeStorage& store,
                            std::optional<std::reference_wrapper<DatabaseHandler>> dbHandler = std::nullopt,
                            std::optional<RepositoryNodeFields> fields = std::nullopt);
    ~RepositoryNode() noexcept override;

    static inline RepositoryNode *from(LakosianNode *other)
    {
        return dynamic_cast<RepositoryNode *>(other);
    }

    inline RepositoryNodeFields const& getFields()
    {
        return d_fields;
    }

    // ACCESSORS
    [[nodiscard]] lvtshr::DiagramType type() const override;
    [[nodiscard]] bool isPackageGroup() override;
    [[nodiscard]] std::string qualifiedName() const override;
    [[nodiscard]] std::string parentName() override;
    [[nodiscard]] long long id() const override;
    [[nodiscard]] lvtshr::UniqueId uid() const override;
    [[nodiscard]] IsLakosianResult isLakosian() override;
    [[nodiscard]] cpp::result<void, AddChildError> addChild(LakosianNode *child) override;
};

template<>
struct LakosianNodeType<Codethink::lvtldr::RepositoryNode> {
    static auto constexpr diagramType = lvtshr::DiagramType::RepositoryType;
};

} // namespace Codethink::lvtldr

#endif // DIAGRAM_SERVER_CT_LVTLDR_REPOSITORYNODE_H
