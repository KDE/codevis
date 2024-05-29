// ct_lvtldr_nodestorage.h                                         -*-C++-*-

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

#ifndef INCLUDED_CT_LVTLDR_NODESTORAGE
#define INCLUDED_CT_LVTLDR_NODESTORAGE

//@PURPOSE: In-Memory API for serialization / deserialization.
//
//@CLASSES:
//  lvtldr::NodeStorage

#include <ct_lvtldr_sociutils.h>
#include <ct_lvtshr_uniqueid.h>
#include <lvtldr_export.h>

#include <QObject>

#include <any>
#include <memory>
#include <soci/soci.h>
#include <vector>

#include <result/result.hpp>

namespace Codethink::lvtldr {
class LakosianNode;
class TypeNode;

// ==========================
// class NodeStorage
// ==========================

struct ErrorRemoveEntity {
    enum class Kind { CannotRemoveWithProviders, CannotRemoveWithClients, CannotRemoveWithChildren };

    Kind kind;
};

struct ErrorAddPhysicalDependency {
    enum class Kind {
        InvalidType,
        SelfRelation,
        HierarchyLevelMismatch,
        MissingParentDependency,
        DependencyAlreadyExists
    };

    Kind kind;
};
struct ErrorRemovePhysicalDependency {
    enum class Kind { InexistentRelation };

    Kind kind;
};
struct ErrorAddComponent {
    enum class Kind { MissingParent, QualifiedNameAlreadyRegistered, CannotAddComponentToPkgGroup };

    Kind kind;
};
struct ErrorAddPackage {
    enum class Kind { QualifiedNameAlreadyRegistered, CannotAddPackageToStandalonePackage, CantAddChildren };
    Kind kind;
    std::string what;
};

struct ErrorAddUDT {
    enum class Kind { BadParentType };

    Kind kind;
};
struct ErrorAddLogicalRelation {
    enum class Kind {
        SelfRelation,
        InvalidLakosRelationType,
        AlreadyHaveDependency,
        ComponentDependencyRequired,
        ParentDependencyRequired,
        InvalidRelation
    };

    Kind kind;
};
struct ErrorRemoveLogicalRelation {
    enum class Kind { InexistentRelation, InvalidLakosRelationType };

    Kind kind;
};
struct ErrorReparentEntity {
    enum class Kind { InvalidEntity, InvalidParent };

    Kind kind;
};

class LVTLDR_EXPORT NodeStorage : public QObject {
    Q_OBJECT
    // Append-only store of LakosianNodes with fast lookup

  private:
    using DatabaseHandlerType = SociDatabaseHandler;

    // TYPES
    struct Private;

    // DATA
    std::unique_ptr<Private> d;

  public:
    // CREATORS
    NodeStorage();
    ~NodeStorage() noexcept;
    NodeStorage(const NodeStorage&) = delete;
    NodeStorage(NodeStorage&&) noexcept;

    void setDatabaseSourcePath(std::string const& path);
    void closeDatabase();

    // MODIFIERS
    cpp::result<LakosianNode *, ErrorAddPackage> addPackage(const std::string& name,
                                                            const std::string& qualifiedName,
                                                            LakosianNode *parent = nullptr,
                                                            std::any userdata = std::any());

    cpp::result<void, ErrorRemoveEntity> removePackage(LakosianNode *node);
    // removes a package.

    cpp::result<LakosianNode *, ErrorAddComponent>
    addComponent(const std::string& name, const std::string& qualifiedName, LakosianNode *parentPackage);
    // adds a component on a Package.

    cpp::result<void, ErrorRemoveEntity> removeComponent(LakosianNode *node);
    // removes a package.

    cpp::result<LakosianNode *, ErrorAddUDT> addLogicalEntity(const std::string& name,
                                                              const std::string& qualifiedName,
                                                              LakosianNode *parent,
                                                              lvtshr::UDTKind kind);
    // adds a logical entity on a given parent.
    // the given parent can be a component, or a logical entity.

    cpp::result<void, ErrorRemoveEntity> removeLogicalEntity(LakosianNode *node);

    [[nodiscard]] cpp::result<void, ErrorAddPhysicalDependency>
    addPhysicalDependency(LakosianNode *source, LakosianNode *target, bool emitSignals = true);

    [[nodiscard]] cpp::result<void, ErrorRemovePhysicalDependency>
    removePhysicalDependency(LakosianNode *source, LakosianNode *target, bool emitSignals = true);

    [[nodiscard]] cpp::result<void, ErrorAddLogicalRelation>
    addLogicalRelation(TypeNode *source, TypeNode *target, lvtshr::LakosRelationType type);
    [[nodiscard]] cpp::result<void, ErrorRemoveLogicalRelation>
    removeLogicalRelation(TypeNode *source, TypeNode *target, lvtshr::LakosRelationType type);

    [[nodiscard]] cpp::result<void, ErrorReparentEntity> reparentEntity(LakosianNode *entity, LakosianNode *newParent);

    LakosianNode *findById(const lvtshr::UniqueId& uid);
    LakosianNode *findByQualifiedName(const std::string& qualifiedName);
    LakosianNode *findByQualifiedName(lvtshr::DiagramType type, const std::string& qualifiedName);
    [[nodiscard]] std::vector<LakosianNode *> getTopLevelPackages();

    void clear();

    std::invoke_result_t<decltype(&DatabaseHandlerType::getSession), DatabaseHandlerType> getSession();

    // Signals
    Q_SIGNAL void storageCleared();
    Q_SIGNAL void storageChanged();
    Q_SIGNAL void nodeAdded(LakosianNode *, std::any);
    Q_SIGNAL void nodeRemoved(LakosianNode *);
    Q_SIGNAL void nodeNameChanged(LakosianNode *);
    Q_SIGNAL void physicalDependencyAdded(LakosianNode *, LakosianNode *);
    Q_SIGNAL void physicalDependencyRemoved(LakosianNode *, LakosianNode *);
    Q_SIGNAL void logicalRelationAdded(LakosianNode *, LakosianNode *, lvtshr::LakosRelationType type);
    Q_SIGNAL void logicalRelationRemoved(LakosianNode *, LakosianNode *, lvtshr::LakosRelationType type);
    Q_SIGNAL void entityReparent(LakosianNode *, LakosianNode *, LakosianNode *);

    cpp::result<RawDbQueryResult, ErrorSqlQuery> rawDbQuery(const std::string& query);

  private:
    template<typename LDR_TYPE>
    LakosianNode *fetchFromDBByQualifiedName(const std::string& qualifiedName);

    template<typename LDR_TYPE>
    LakosianNode *fetchFromDBById(const lvtshr::UniqueId& uid);

    template<typename LDR_TYPE>
    void updateAndNotifyNodeRename(LakosianNode *node);

    void preloadHighLevelComponents();
};

} // namespace Codethink::lvtldr
#endif
