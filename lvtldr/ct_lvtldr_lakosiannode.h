// ct_lvtldr_lakosiannode.h                                           -*-C++-*-

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

#ifndef INCLUDED_CT_LVTLDR_LAKOSIANNODE
#define INCLUDED_CT_LVTLDR_LAKOSIANNODE

//@PURPOSE: Types *internal to lvtldr* used to cache loaded graphs in memory
//
//@CLASSES:
//  lvtldr::NodeStorage: Append-only store of LakosianNodes with fast lookup
//  lvtldr::LakosianEdge: Models a relation between LakosianNodes
//  lvtldr::LakosianNode: Representation of one node in the graph we loaded from
//                        the database
//  lvtldr::TypeNode: Implementation of LakosianNode for UDTs
//  lvtldr::ComponentNode: Implementation of LakosianNode for components
//  lvtldr::PackageNode: Implementation of LakosianNode for packages

#include <lvtldr_export.h>

#include <ct_lvtldr_lakosianedge.h>

#include <ct_lvtldr_databasehandler.h>
#include <ct_lvtshr_graphenums.h>
#include <ct_lvtshr_uniqueid.h>

#include <QObject>

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <result/result.hpp>

namespace Codethink::lvtldr {

// FORWARD DECLARATIONS
class LakosianNode;

// TODO: Remove NodeStorage from this class. there should be no reason that a Node needs to know *how* it's stored.
class NodeStorage;

struct LVTLDR_EXPORT AddChildError {
    std::string what;
};

struct LVTLDR_EXPORT NamingUtils {
    static std::vector<std::string> buildQualifiedNamePrefixParts(const std::string& qname,
                                                                  const std::string& separator);
    static std::string
    buildQualifiedName(const std::vector<std::string>& parts, const std::string& name, const std::string& separator);
};

// ==========================
// class LakosianNode
// ==========================

class LVTLDR_EXPORT LakosianNode : public QObject {
    Q_OBJECT
    // A node in a physical graph. This could be a package group, package,
    // component or type.
    // Should not be used outside of lvtldr.

  protected:
    // TYPES
    struct Private {
        NodeStorage& store;
        std::optional<std::reference_wrapper<DatabaseHandler>> dbHandler = std::nullopt;

        std::string name;

        bool parentLoaded = false;
        bool childrenLoaded = false;
        bool providersLoaded = false;
        bool clientsLoaded = false;
        bool fieldsLoaded = false;

        LakosianNode *parent = nullptr;
        std::vector<LakosianNode *> children;
        std::vector<LakosianNode *> innerPackages;
        std::vector<LakosianEdge> providers;
        std::vector<LakosianEdge> clients;
        std::vector<std::string> fieldNames;

        explicit Private(NodeStorage& store, std::optional<std::reference_wrapper<DatabaseHandler>> dbHandler):
            store(store), dbHandler(dbHandler)
        {
        }
    };

    // DATA
    std::unique_ptr<Private> d; // NOLINT

    // MODIFIERS
    virtual void loadParent() = 0;
    // Load the parent (if any) of this node

    virtual void loadChildren() = 0;
    // Load the children (if any) of this node

    virtual void loadProviders() = 0;
    // Load the forward dependencies (if any) of this node

    virtual void loadClients() = 0;
    // Load the reverse dependencies (if any) of this node

  public:
    enum class IsLakosianResult {
        IsLakosian,
        ComponentHasNoPackage,
        ComponentDoesntStartWithParentName,
        PackageParentIsNotGroup,
        PackagePrefixDiffersFromGroup,
        PackageNameInvalidNumberOfChars,
        PackageGroupNameInvalidNumberOfChars
    };

    // CREATORS
    explicit LakosianNode(NodeStorage& store, std::optional<std::reference_wrapper<DatabaseHandler>> dbHandler);
    // Stores the database session as a reference, so the SessionPtr
    // must outlive this LakosianNode

    virtual ~LakosianNode() noexcept;

    // ACCESSORS
    [[nodiscard]] virtual lvtshr::DiagramType type() const = 0;
    [[nodiscard]] virtual bool isPackageGroup();
    // Unfortunately lvtshr::DiagramType doesn't distinguish between packages
    // and package groups

    // For adding to IGraphLoader
    [[nodiscard]] virtual std::string qualifiedName() const = 0;
    [[nodiscard]] virtual std::string parentName() = 0;
    [[nodiscard]] virtual long long id() const = 0;

    [[nodiscard]] virtual lvtshr::UniqueId uid() const = 0;

    [[nodiscard]] virtual IsLakosianResult isLakosian() = 0;

    virtual cpp::result<void, AddChildError> addChild(LakosianNode *child) = 0;
    // Add a child to this node NodeStorage.
    // Useful when creating a Node manually.

    [[nodiscard]] virtual std::string name() const;
    virtual void setName(std::string const& newName);

    // these aren't const because they are lazy-loaded

    LakosianNode *parent();
    // Returns the parent of this LakosianNode,
    // or nullptr if there is no parent.

    LakosianNode *topLevelParent();
    // Returns the parent of this LakosianNode,
    // or itself, if it's the toplevel entity.

    std::vector<LakosianNode *> parentHierarchy();
    // first element is the topmost parent, last element is this node.

    const std::vector<LakosianNode *>& children();

    const std::vector<LakosianEdge>& providers();
    const std::vector<LakosianEdge>& clients();

    const std::vector<std::string>& fields();

    std::string notes() const;
    void setNotes(const std::string& setNotes);

    void invalidateProviders();
    void invalidateClients();
    virtual void invalidateChildren();
    void invalidateParent();

    bool hasProvider(LakosianNode *other);

    // signals
    Q_SIGNAL void onNameChanged(LakosianNode *);
    Q_SIGNAL void onChildCountChanged(size_t);
    Q_SIGNAL void onNotesChanged(std::string);
};

LVTLDR_EXPORT bool operator==(const LakosianNode& lhs, const LakosianNode& rhs);

template<typename T>
struct LakosianNodeType {
};

} // namespace Codethink::lvtldr

#endif // INCLUDED_CT_LVTLDR_LAKOSIANNODE
