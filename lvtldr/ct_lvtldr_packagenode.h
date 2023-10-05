// ct_lvtldr_packagenode.h                                           -*-C++-*-

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

#ifndef DIAGRAM_SERVER_CT_LVTLDR_PACKAGENODE_H
#define DIAGRAM_SERVER_CT_LVTLDR_PACKAGENODE_H

#include <ct_lvtldr_databasehandler.h>
#include <ct_lvtldr_lakosiannode.h>
#include <ct_lvtldr_packagenodefields.h>

namespace Codethink::lvtldr {

// ==========================
// class PackageNode
// ==========================

class LVTLDR_EXPORT PackageNode : public LakosianNode {
    // Implementation of LakosianNode for packages and package groups

  private:
    // DATA
    // TODO: Decide if optional should be removed after refactoring is done.
    std::optional<std::reference_wrapper<DatabaseHandler>> d_dbHandler = std::nullopt;
    PackageNodeFields d_fields;
    std::vector<std::string> d_qualifiedNameParts;

  protected:
    // MODIFIERS
    void loadParent() override;
    void loadChildren() override;
    void loadProviders() override;
    void loadClients() override;

  public:
    explicit PackageNode(NodeStorage& store,
                         std::optional<std::reference_wrapper<DatabaseHandler>> dbHandler = std::nullopt,
                         std::optional<PackageNodeFields> fields = std::nullopt);
    static inline PackageNode *from(LakosianNode *other)
    {
        return dynamic_cast<PackageNode *>(other);
    }

    inline PackageNodeFields const& getFields()
    {
        return d_fields;
    }

    ~PackageNode() noexcept override;

    cpp::result<void, AddChildError> addChild(LakosianNode *child) override;
    void removeChild(LakosianNode *child);
    void setName(std::string const& newName) override;

    void invalidateChildren() override
    {
        LakosianNode::invalidateChildren();
        d->innerPackages.clear();
    }

    // ACCESSORS
    void removeChildPackage(PackageNode *child);
    void addConcreteDependency(PackageNode *other);
    void removeConcreteDependency(PackageNode *other);
    [[nodiscard]] bool hasConcreteDependency(LakosianNode *other);
    [[nodiscard]] lvtshr::DiagramType type() const override;
    [[nodiscard]] bool isPackageGroup() override;
    [[nodiscard]] std::string qualifiedName() const override;
    [[nodiscard]] std::string parentName() override;
    [[nodiscard]] long long id() const override;
    [[nodiscard]] lvtshr::UniqueId uid() const override;
    [[nodiscard]] IsLakosianResult isLakosian() override;
    [[nodiscard]] std::string canonicalName() const;
    [[nodiscard]] bool isStandalone() const;
    [[nodiscard]] std::string dirPath() const;
    [[nodiscard]] bool hasRepository() const;
};

template<>
struct LakosianNodeType<Codethink::lvtldr::PackageNode> {
    static auto constexpr diagramType = lvtshr::DiagramType::PackageType;
};

} // namespace Codethink::lvtldr

#endif
