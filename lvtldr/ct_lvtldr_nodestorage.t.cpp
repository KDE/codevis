// ct_lvtldr_nodestorage.t.cpp                              -*-C++-*-

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

#include <catch2/catch.hpp>

#include <ct_lvtldr_lakosiannode.h>
#include <ct_lvtldr_nodestorage.h>
#include <ct_lvtldr_packagenode.h>
#include <ct_lvtldr_typenode.h>

#include <ct_lvtldr_nodestoragetestutils.h>
#include <ct_lvttst_tmpdir.h>

using Codethink::lvtshr::DiagramType;
using Codethink::lvtshr::LakosRelationType;
using Codethink::lvtshr::UDTKind;
using Codethink::lvtshr::UniqueId;

using namespace Codethink::lvtldr;

#include <ct_lvtmdb_objectstore.h>
#include <ct_lvtmdb_packageobject.h>
#include <ct_lvtmdb_soci_writer.h>
#include <ct_lvtprj_projectfile.h>

TEST_CASE("Add entities to nodeStorage")
{
    auto tmpDir = TmpDir{"add_entities_to_ns_test"};
    auto dbPath = tmpDir.path() / "codedb.db";

    auto nodeStorage = NodeStorageTestUtils::createEmptyNodeStorage(dbPath);

    int callcount = 0;
    QObject::connect(&nodeStorage, &NodeStorage::nodeAdded, [&callcount](LakosianNode *_, std::any) { // NOLINT
        (void) _;
        callcount += 1;
    });

    // Make sure we don't crash while querying an item that does not exist, returning a nullptr is ok.
    REQUIRE(nodeStorage.findById(UniqueId(DiagramType::PackageType, 15)) == nullptr);
    REQUIRE(nodeStorage.findByQualifiedName(DiagramType::PackageType, "non-existing-name") == nullptr);

    // Add a package on the nodeStorage.
    REQUIRE(nodeStorage.findByQualifiedName(DiagramType::PackageType, "tmz") == nullptr);
    REQUIRE(callcount == 0);
    auto *tmz = nodeStorage.addPackage("tmz", "tmz", nullptr, std::any()).value();
    REQUIRE(callcount == 1);
    REQUIRE(tmz != nullptr);
    REQUIRE(nodeStorage.findByQualifiedName(DiagramType::PackageType, "tmz") == tmz);
    REQUIRE(nodeStorage.findByQualifiedName(DiagramType::ComponentType, "tmz") == nullptr);
    REQUIRE(nodeStorage.findByQualifiedName(DiagramType::ClassType, "tmz") == nullptr);
    REQUIRE(tmz->name() == "tmz");
    REQUIRE(tmz->type() == DiagramType::PackageType);
    REQUIRE(tmz->qualifiedName() == "tmz");
    REQUIRE(tmz->children().empty());

    // Make sure that getting from nodeStorage in different ways will return the same thing
    REQUIRE(nodeStorage.findByQualifiedName(DiagramType::PackageType, "tmz") == tmz);
    REQUIRE(nodeStorage.findById(tmz->uid()) == tmz);

    // Add a component on the nodeStorage.
    REQUIRE(nodeStorage.findByQualifiedName(DiagramType::ComponentType, "tmz/tmzcmp") == nullptr);
    REQUIRE(callcount == 1);
    auto *tmzcmp = nodeStorage.addComponent("tmzcmp", "tmz/tmzcmp", tmz).value();
    REQUIRE(callcount == 2);
    REQUIRE(tmzcmp != nullptr);
    REQUIRE(nodeStorage.findByQualifiedName(DiagramType::PackageType, "tmz/tmzcmp") == nullptr);
    REQUIRE(nodeStorage.findByQualifiedName(DiagramType::ComponentType, "tmz/tmzcmp") == tmzcmp);
    {
        // Make sure that even in a node storage with empty cache, the entity can be found (will fetch from DB)
        NodeStorage nodeStorage2;
        nodeStorage2.setDatabaseSourcePath(dbPath.string());
        REQUIRE(nodeStorage2.findByQualifiedName(DiagramType::ComponentType, "tmz/tmzcmp") != nullptr);
    }
    REQUIRE(nodeStorage.findByQualifiedName(DiagramType::ClassType, "tmz/tmzcmp") == nullptr);
    REQUIRE(tmzcmp->name() == "tmzcmp");
    REQUIRE(tmzcmp->type() == DiagramType::ComponentType);
    REQUIRE(tmzcmp->qualifiedName() == "tmz/tmzcmp");
    REQUIRE(tmzcmp->children().empty());
    REQUIRE(tmz->children().size() == 1);
    REQUIRE(tmz->children()[0] == tmzcmp);

    // Add a component on the nodeStorage.
    REQUIRE(nodeStorage.findByQualifiedName(DiagramType::ComponentType, "tmzcmp::tmzcmpa") == nullptr);
    REQUIRE(callcount == 2);
    auto *tmzcmpa = [&]() {
        auto result = nodeStorage.addLogicalEntity("tmzcmpa", "tmzcmp::tmzcmpa", tmzcmp, UDTKind::Class);
        REQUIRE_FALSE(result.has_error());
        return result.value();
    }();
    REQUIRE(callcount == 3);
    REQUIRE(tmzcmpa != nullptr);
    REQUIRE(nodeStorage.findByQualifiedName(DiagramType::PackageType, "tmzcmp::tmzcmpa") == nullptr);
    REQUIRE(nodeStorage.findByQualifiedName(DiagramType::ComponentType, "tmzcmp::tmzcmpa") == nullptr);
    REQUIRE(nodeStorage.findByQualifiedName(DiagramType::ClassType, "tmzcmp::tmzcmpa") == tmzcmpa);
    {
        // Make sure that even in a node storage with empty cache, the entity can be found (will fetch from DB)
        NodeStorage nodeStorage2;
        nodeStorage2.setDatabaseSourcePath(dbPath.string());
        REQUIRE(nodeStorage2.findByQualifiedName(DiagramType::ClassType, "tmzcmp::tmzcmpa") != nullptr);
    }
    REQUIRE(tmzcmpa->name() == "tmzcmpa");
    REQUIRE(tmzcmpa->type() == DiagramType::ClassType);
    REQUIRE(tmzcmpa->qualifiedName() == "tmzcmp::tmzcmpa");
    REQUIRE(tmzcmpa->children().empty());
    REQUIRE(tmzcmp->children().size() == 1);
    REQUIRE(tmzcmp->children()[0] == tmzcmpa);

    auto parentHierarchy = tmzcmpa->parentHierarchy();
    REQUIRE(parentHierarchy.size() == 3);
    REQUIRE(parentHierarchy[0] == tmz);
    REQUIRE(parentHierarchy[1] == tmzcmp);
    REQUIRE(parentHierarchy[2] == tmzcmpa);

    {
        // Cannot add a logical entity in a package
        auto result = nodeStorage.addLogicalEntity("somepkg", "somepkg", tmz, UDTKind::Class);
        REQUIRE(result.has_error());
        REQUIRE(nodeStorage.findByQualifiedName(DiagramType::ClassType, "somepkg") == nullptr);
    }

    REQUIRE(nodeStorage.findByQualifiedName(DiagramType::ClassType, "tmzcmp::tmzcmpa") == tmzcmpa);
    REQUIRE(nodeStorage.findById(tmzcmpa->uid()) == tmzcmpa);
    auto tmzcmpaUid = tmzcmpa->uid();
    REQUIRE_FALSE(nodeStorage.removeLogicalEntity(tmzcmpa).has_error());
    // Can't use tmzcmpa object from this point, since it's deleted
    tmzcmpa = nullptr;
    REQUIRE(nodeStorage.findByQualifiedName(DiagramType::ClassType, "tmzcmp::tmzcmpa") == nullptr);
    REQUIRE(nodeStorage.findById(tmzcmpaUid) == nullptr);

    auto *tmzcmp_class =
        nodeStorage.addLogicalEntity("tmzcmp_class", "tmzcmp::tmzcmp_class", tmzcmp, UDTKind::Class).value();
    REQUIRE(nodeStorage.findByQualifiedName(DiagramType::ClassType, "tmzcmp::tmzcmp_class") == tmzcmp_class);
    REQUIRE(dynamic_cast<TypeNode *>(tmzcmp_class)->kind() == UDTKind::Class);

    auto *tmzcmp_struct =
        nodeStorage.addLogicalEntity("tmzcmp_struct", "tmzcmp::tmzcmp_struct", tmzcmp, UDTKind::Struct).value();
    REQUIRE(nodeStorage.findByQualifiedName(DiagramType::ClassType, "tmzcmp::tmzcmp_struct") == tmzcmp_struct);
    REQUIRE(dynamic_cast<TypeNode *>(tmzcmp_struct)->kind() == UDTKind::Struct);

    auto *tmzcmp_enum =
        nodeStorage.addLogicalEntity("tmzcmp_enum", "tmzcmp::tmzcmp_enum", tmzcmp, UDTKind::Enum).value();
    REQUIRE(nodeStorage.findByQualifiedName(DiagramType::ClassType, "tmzcmp::tmzcmp_enum") == tmzcmp_enum);
    REQUIRE(dynamic_cast<TypeNode *>(tmzcmp_enum)->kind() == UDTKind::Enum);

    // It is ok for a class to be child of another class
    auto *tmzcmp_class2 =
        nodeStorage
            .addLogicalEntity("tmzcmp_class2", "tmzcmp::tmzcmp_class::tmzcmp_class2", tmzcmp_class, UDTKind::Class)
            .value();
    REQUIRE(nodeStorage.findByQualifiedName(DiagramType::ClassType, "tmzcmp::tmzcmp_class::tmzcmp_class2")
            == tmzcmp_class2);
    REQUIRE(dynamic_cast<TypeNode *>(tmzcmp_class2)->kind() == UDTKind::Class);

    // It is *NOT* ok for a class to be child of an enum
    REQUIRE(nodeStorage.addLogicalEntity("some_klass", "tmzcmp::tmzcmp_enum::some_klass", tmzcmp_enum, UDTKind::Class)
                .has_error());

    // Cannot add a component in a package group
    {
        auto *grp = nodeStorage.addPackage("grp", "grp", nullptr).value();
        REQUIRE_FALSE(grp->isPackageGroup());
        (void) nodeStorage.addPackage("pkg", "grp/pkg", grp);
        REQUIRE(grp->isPackageGroup());
        auto result = nodeStorage.addComponent("component", "grp::component", grp);
        REQUIRE(result.has_error());
        REQUIRE(result.error().kind == ErrorAddComponent::Kind::CannotAddComponentToPkgGroup);
    }

    // Cannot add a package in a group that already contains a component
    {
        auto *pkg = nodeStorage.addPackage("pkg2", "pkg2", nullptr).value();
        (void) nodeStorage.addComponent("component", "pkg2/component", pkg);
        auto result = nodeStorage.addPackage("inner", "pkg2/inner", pkg);
        REQUIRE(result.has_error());
        REQUIRE(result.error().kind == ErrorAddPackage::Kind::CannotAddPackageToStandalonePackage);
    }
}

TEST_CASE("Package groups and standalone packages interaction")
{
    auto tmpDir = TmpDir{"pkgs_std_pkg_interact_test"};
    auto dbPath = tmpDir.path() / "codedb.db";
    auto nodeStorage = NodeStorageTestUtils::createEmptyNodeStorage(dbPath);

    auto *grp = nodeStorage.addPackage("grp", "grp", nullptr).value();
    auto *pkg = nodeStorage.addPackage("pkg", "grp/pkg", grp).value();
    auto *a = nodeStorage.addComponent("a", "grp/pkg/a", pkg).value();
    auto *std = nodeStorage.addPackage("std", "std", nullptr).value();
    auto *b = nodeStorage.addComponent("b", "std/b", std).value();

    // Rule: A pkg group is allowed to depend on a standalone pkg.
    {
        auto result = nodeStorage.addPhysicalDependency(grp, std);
        REQUIRE_FALSE(result.has_error());
        REQUIRE(grp->hasProvider(std));
    }

    // Rule: A pkg group that depends on a standalone pkg can consume any component from the standalone pkg.
    {
        auto result = nodeStorage.addPhysicalDependency(a, b);
        REQUIRE_FALSE(result.has_error());
    }
}

TEST_CASE("Remove Packages")
{
    using ErrorKind = ErrorRemovePackage::Kind;

    {
        auto tmpDir = TmpDir{"remove_packages_basic"};
        auto dbPath = tmpDir.path() / "codedb.db";
        auto nodeStorage = NodeStorageTestUtils::createEmptyNodeStorage(dbPath);

        auto *pkg1 = nodeStorage.addPackage("pkg1", "pkg1", nullptr, std::any()).value();
        auto *pkg2 = nodeStorage.addPackage("pkg2", "pkg2", nullptr, std::any()).value();

        REQUIRE(nodeStorage.getTopLevelPackages().size() == 2);
        auto ret = nodeStorage.removePackage(pkg1);
        REQUIRE_FALSE(ret.has_error());
        REQUIRE(nodeStorage.getTopLevelPackages().size() == 1);

        ret = nodeStorage.removePackage(pkg2);
        REQUIRE_FALSE(ret.has_error());
        REQUIRE(nodeStorage.getTopLevelPackages().empty());
    }

    // we currently don't allow removing a package that has children
    {
        auto tmpDir = TmpDir{"remove_packages_with_childs"};
        auto dbPath = tmpDir.path() / "codedb.db";
        auto nodeStorage = NodeStorageTestUtils::createEmptyNodeStorage(dbPath);

        auto *pkg1 = nodeStorage.addPackage("pkg1", "pkg1", nullptr, std::any()).value();
        auto *pkg2 = nodeStorage.addPackage("pkg2", "pkg2", pkg1, std::any()).value();

        // pkg1 contains pkg2, first we need to remove pkg2, to be able to remove pkg1.
        REQUIRE(pkg1->children().size() == 1);
        REQUIRE(nodeStorage.getTopLevelPackages().size() == 1);
        auto ret = nodeStorage.removePackage(pkg1);
        REQUIRE(ret.has_error());
        REQUIRE(ret.error().kind == ErrorKind::CannotRemovePackageWithChildren);
        REQUIRE(nodeStorage.getTopLevelPackages().size() == 1);

        ret = nodeStorage.removePackage(pkg2);
        REQUIRE_FALSE(ret.has_error());
        REQUIRE(pkg1->children().empty());

        ret = nodeStorage.removePackage(pkg1);
        REQUIRE_FALSE(ret.has_error());
        REQUIRE(nodeStorage.getTopLevelPackages().empty());
    }

    // we currently don't allow removing packages with dependencies
    {
        auto tmpDir = TmpDir{"remove_packages_with_deps"};
        auto dbPath = tmpDir.path() / "codedb.db";
        auto nodeStorage = NodeStorageTestUtils::createEmptyNodeStorage(dbPath);

        auto *pkg1 = nodeStorage.addPackage("pkg1", "pkg1", nullptr).value();
        auto *pkg2 = nodeStorage.addPackage("pkg2", "pkg2", nullptr).value();
        nodeStorage.addPhysicalDependency(pkg1, pkg2).expect("");

        // pkg1 contains pkg2, first we need to remove pkg2, to be able to remove pkg1.
        REQUIRE(nodeStorage.getTopLevelPackages().size() == 2);
        auto ret1 = nodeStorage.removePackage(pkg1);
        REQUIRE(ret1.has_error());
        REQUIRE(ret1.error().kind == ErrorKind::CannotRemovePackageWithProviders);
        REQUIRE(nodeStorage.getTopLevelPackages().size() == 2);
        auto ret2 = nodeStorage.removePackage(pkg2);
        REQUIRE(ret2.has_error());
        REQUIRE(ret2.error().kind == ErrorKind::CannotRemovePackageWithClients);
        REQUIRE(nodeStorage.getTopLevelPackages().size() == 2);
    }
}

TEST_CASE("Cyclic dependencies aren't allowed")
{
    {
        auto tmpDir = TmpDir{"cyclic_deps_not_allowed_test_1"};
        auto dbPath = tmpDir.path() / "codedb.db";
        auto nodeStorage = NodeStorageTestUtils::createEmptyNodeStorage(dbPath);

        auto *pkg1 = nodeStorage.addPackage("pkg1", "pkg1", nullptr, std::any()).value();
        auto *pkg2 = nodeStorage.addPackage("pkg2", "pkg2", nullptr, std::any()).value();

        REQUIRE(pkg1->children().empty());
        auto ret = pkg1->addChild(pkg2);
        REQUIRE(!ret.has_error());
        REQUIRE(pkg1->children().size() == 1);

        // TODO: Shouldn't allow cyclic dependency
        REQUIRE(pkg2->children().empty());
        auto ret2 = pkg2->addChild(pkg1);
        REQUIRE(ret2.has_error());
        REQUIRE(pkg2->children().empty());
    }

    {
        auto tmpDir = TmpDir{"cyclic_deps_not_allowed_test_2"};
        auto dbPath = tmpDir.path() / "codedb.db";
        auto nodeStorage = NodeStorageTestUtils::createEmptyNodeStorage(dbPath);

        auto *pkg1 = nodeStorage.addPackage("pkg1", "pkg1", nullptr, std::any()).value();
        auto *pkg2 = nodeStorage.addPackage("pkg2", "pkg2", nullptr, std::any()).value();
        auto *pkg3 = nodeStorage.addPackage("pkg3", "pkg3", nullptr, std::any()).value();

        REQUIRE(pkg1->children().empty());
        auto ret = pkg1->addChild(pkg2);
        REQUIRE(!ret.has_error());
        REQUIRE(pkg1->children().size() == 1);

        REQUIRE(pkg2->children().empty());
        auto ret2 = pkg2->addChild(pkg3);
        REQUIRE(!ret2.has_error());
        REQUIRE(pkg2->children().size() == 1);

        // TODO [xxx]: Shouldn't allow cyclic dependency
        REQUIRE(pkg3->children().empty());
        (void) pkg3->addChild(pkg1);
        //  REQUIRE(ret3.has_error());
        REQUIRE(pkg3->children().empty());
    }
}

TEST_CASE("Add dependency relationship")
{
    using Kind = ErrorAddPhysicalDependency::Kind;

    auto tmpDir = TmpDir{"add_dep_relation_test"};
    auto dbPath = tmpDir.path() / "codedb.db";
    auto nodeStorage = NodeStorageTestUtils::createEmptyNodeStorage(dbPath);

    auto expectAddComponent = [&nodeStorage](const std::string& name, const std::string& qualifiedName, auto *parent) {
        auto result = nodeStorage.addComponent(name, qualifiedName, parent);
        REQUIRE_FALSE(result.has_error());
        return result.value();
    };

    auto *a = nodeStorage.addPackage("a", "a", nullptr, std::any()).value();
    auto *b = nodeStorage.addPackage("b", "b", nullptr, std::any()).value();
    auto *aa = nodeStorage.addPackage("aa", "aa", a, std::any()).value();
    auto *ab = nodeStorage.addPackage("ab", "ab", a, std::any()).value();
    auto *ba = nodeStorage.addPackage("ba", "ba", b, std::any()).value();
    auto *sln = nodeStorage.addPackage("sln", "standalones/sln", nullptr, std::any()).value();
    auto *aaa = expectAddComponent("aaa", "aaa", aa);
    auto *aab = expectAddComponent("aab", "aab", aa);
    auto *aba = expectAddComponent("aba", "aba", ab);
    auto *baa = expectAddComponent("baa", "baa", ba);

    // Its ok to components from the same package to depend on each other
    REQUIRE_FALSE(aaa->hasProvider(aab));
    REQUIRE_FALSE(aab->hasProvider(aaa));
    auto r = nodeStorage.addPhysicalDependency(aaa, aab);
    REQUIRE_FALSE(r.has_error());
    REQUIRE(aaa->hasProvider(aab));
    REQUIRE_FALSE(aab->hasProvider(aaa));

    // Self-dependency is not allowed
    REQUIRE_FALSE(aaa->hasProvider(aaa));
    r = nodeStorage.addPhysicalDependency(aaa, aaa);
    REQUIRE((r.has_error() && r.error().kind == Kind::SelfRelation));
    REQUIRE_FALSE(aaa->hasProvider(aaa));

    // Component from different packages cannot depend on each other if there's no dependency between the packages...
    REQUIRE_FALSE(aba->hasProvider(aaa));
    r = nodeStorage.addPhysicalDependency(aba, aaa);
    REQUIRE((r.has_error() && r.error().kind == Kind::MissingParentDependency));
    REQUIRE_FALSE(aba->hasProvider(aaa));

    // ... but if the parents have a dependency, then it is allowed
    r = nodeStorage.addPhysicalDependency(ab, aa);
    REQUIRE_FALSE(r.has_error());
    r = nodeStorage.addPhysicalDependency(aba, aaa);
    REQUIRE_FALSE(r.has_error());
    REQUIRE(aba->hasProvider(aaa));

    // It is not possible for a component to depend on a package
    REQUIRE_FALSE(aaa->hasProvider(ab));
    r = nodeStorage.addPhysicalDependency(aaa, ab);
    REQUIRE((r.has_error() && r.error().kind == Kind::HierarchyLevelMismatch));
    REQUIRE_FALSE(aaa->hasProvider(ab));

    // It is not possible for a package to depend on a component
    REQUIRE_FALSE(ab->hasProvider(aaa));
    r = nodeStorage.addPhysicalDependency(ab, aaa);
    REQUIRE((r.has_error() && r.error().kind == Kind::HierarchyLevelMismatch));
    REQUIRE_FALSE(ab->hasProvider(aaa));

    // In order for two components to depend on each other, their parents must have a dependency
    REQUIRE_FALSE(b->hasProvider(a));
    REQUIRE_FALSE(ba->hasProvider(aa));
    REQUIRE_FALSE(baa->hasProvider(aaa));

    // Not possible because `ba` doesn't depend on `aa`
    r = nodeStorage.addPhysicalDependency(baa, aaa);
    REQUIRE((r.has_error() && r.error().kind == Kind::MissingParentDependency));
    REQUIRE_FALSE(baa->hasProvider(aaa));

    // Not possible because `b` doesn't depend on `a`
    r = nodeStorage.addPhysicalDependency(ba, aa);
    REQUIRE((r.has_error() && r.error().kind == Kind::MissingParentDependency));
    r = nodeStorage.addPhysicalDependency(baa, aaa);
    REQUIRE((r.has_error() && r.error().kind == Kind::MissingParentDependency));
    REQUIRE_FALSE(baa->hasProvider(aaa));

    r = nodeStorage.addPhysicalDependency(b, a);
    REQUIRE_FALSE(r.has_error());
    // Not possible because `ba` doesnt depend on `aa`
    r = nodeStorage.addPhysicalDependency(baa, aaa);
    REQUIRE((r.has_error() && r.error().kind == Kind::MissingParentDependency));
    REQUIRE(b->hasProvider(a));
    REQUIRE_FALSE(baa->hasProvider(aaa));

    r = nodeStorage.addPhysicalDependency(ba, aa);
    REQUIRE_FALSE(r.has_error());
    r = nodeStorage.addPhysicalDependency(baa, aaa);
    REQUIRE_FALSE(r.has_error());
    REQUIRE(ba->hasProvider(aa));
    REQUIRE(baa->hasProvider(aaa));

    // Check that removing the relation is working properly
    REQUIRE(baa->hasProvider(aaa));
    nodeStorage.removePhysicalDependency(baa, aaa).expect("");
    REQUIRE_FALSE(baa->hasProvider(aaa));
    r = nodeStorage.addPhysicalDependency(baa, aaa);
    REQUIRE(baa->hasProvider(aaa));

    // It must not be possible to add a physical dependency relation between logical entities
    auto *x = nodeStorage.addLogicalEntity("x", "x", aaa, UDTKind::Class).value();
    auto *y = nodeStorage.addLogicalEntity("y", "y", aaa, UDTKind::Class).value();
    r = nodeStorage.addPhysicalDependency(x, y);
    REQUIRE((r.has_error() && r.error().kind == Kind::InvalidType));
    REQUIRE_FALSE(x->hasProvider(y));

    // There's no such thing as an "allowed dependency" between two components. Only between packages
    REQUIRE(baa->hasProvider(aaa));
    nodeStorage.removePhysicalDependency(baa, aaa).expect("");
    REQUIRE_FALSE(baa->hasProvider(aaa));
    r = nodeStorage.addPhysicalDependency(baa, aaa, PhysicalDependencyType::AllowedDependency);
    REQUIRE(r.has_error());
    REQUIRE(r.error().kind == Kind::InvalidType);

    // Check Allowed and Concrete dependency workflow
    nodeStorage.removePhysicalDependency(ba, aa).expect("");
    REQUIRE_FALSE(ba->hasProvider(aa));
    r = nodeStorage.addPhysicalDependency(ba, aa, PhysicalDependencyType::AllowedDependency);
    REQUIRE_FALSE(r.has_error());
    REQUIRE(ba->hasProvider(aa));
    REQUIRE(dynamic_cast<PackageNode *>(ba)->hasAllowedDependency(aa));
    REQUIRE_FALSE(dynamic_cast<PackageNode *>(ba)->hasConcreteDependency(aa));
    r = nodeStorage.addPhysicalDependency(ba, aa, PhysicalDependencyType::ConcreteDependency);
    REQUIRE_FALSE(r.has_error());
    REQUIRE(ba->hasProvider(aa));
    REQUIRE(dynamic_cast<PackageNode *>(ba)->hasAllowedDependency(aa));
    REQUIRE(dynamic_cast<PackageNode *>(ba)->hasConcreteDependency(aa));
    nodeStorage.removePhysicalDependency(ba, aa, PhysicalDependencyType::AllowedDependency).expect("");
    // Still have Concrete dependency
    REQUIRE(ba->hasProvider(aa));
    REQUIRE_FALSE(dynamic_cast<PackageNode *>(ba)->hasAllowedDependency(aa));
    REQUIRE(dynamic_cast<PackageNode *>(ba)->hasConcreteDependency(aa));
    nodeStorage.removePhysicalDependency(ba, aa, PhysicalDependencyType::ConcreteDependency).expect("");
    REQUIRE_FALSE(ba->hasProvider(aa));
    REQUIRE_FALSE(dynamic_cast<PackageNode *>(ba)->hasAllowedDependency(aa));
    REQUIRE_FALSE(dynamic_cast<PackageNode *>(ba)->hasConcreteDependency(aa));

    // It is ok for a package group to depend on a standalone package
    REQUIRE_FALSE(a->hasProvider(sln));
    nodeStorage.addPhysicalDependency(a, sln, PhysicalDependencyType::ConcreteDependency).expect("");
    REQUIRE(a->hasProvider(sln));
    nodeStorage.removePhysicalDependency(a, sln, PhysicalDependencyType::ConcreteDependency).expect("");
    REQUIRE_FALSE(a->hasProvider(sln));

    // It is ok for a standalone package to depend on package groups
    REQUIRE_FALSE(sln->hasProvider(a));
    nodeStorage.addPhysicalDependency(sln, a, PhysicalDependencyType::ConcreteDependency).expect("");
    REQUIRE(sln->hasProvider(a));
    nodeStorage.removePhysicalDependency(sln, a, PhysicalDependencyType::ConcreteDependency).expect("");
    REQUIRE_FALSE(sln->hasProvider(a));
}

TEST_CASE("Renamed entity")
{
    auto tmpDir = TmpDir{"rename_entity_test"};
    auto dbPath = tmpDir.path() / "codedb.db";
    auto nodeStorage = NodeStorageTestUtils::createEmptyNodeStorage(dbPath);

    auto *somePkg = nodeStorage.addPackage("a", "a", nullptr, std::any()).value();
    REQUIRE(nodeStorage.findByQualifiedName(DiagramType::PackageType, "a"));
    somePkg->setName("b");
    REQUIRE_FALSE(nodeStorage.findByQualifiedName(DiagramType::PackageType, "a"));
    REQUIRE(nodeStorage.findByQualifiedName(DiagramType::PackageType, "b"));
}

TEST_CASE("Connect renamed entities")
{
    // There was a bug where renamed entities couldn't be connected (physical dependency).
    // This test has been added to avoid regression.
    auto tmpDir = TmpDir{"connect_renamed_ent_test"};
    auto dbPath = tmpDir.path() / "codedb.db";
    auto nodeStorage = NodeStorageTestUtils::createEmptyNodeStorage(dbPath);

    auto *a = nodeStorage.addPackage("a", "a", nullptr, std::any()).value();
    auto *b = nodeStorage.addPackage("b", "b", nullptr, std::any()).value();
    REQUIRE_FALSE(a->hasProvider(b));
    a->setName("c");
    auto r = nodeStorage.addPhysicalDependency(a, b);
    REQUIRE_FALSE(r.has_error());
    REQUIRE(a->hasProvider(b));
}

TEST_CASE("Wrong entity parenting")
{
    // There was a bug where renamed entities was wrongly creating ghost children packages
    // This test has been added to avoid regression.
    auto tmpDir = TmpDir{"wrong_ent_parent_test"};
    auto dbPath = tmpDir.path() / "codedb.db";
    auto nodeStorage = NodeStorageTestUtils::createEmptyNodeStorage(dbPath);

    auto *a = nodeStorage.addPackage("a", "a", nullptr, std::any()).value();
    a->setName("c");
    auto *aa = nodeStorage.addPackage("aa", "aa", a, std::any()).value();
    (void) aa;

    REQUIRE(nodeStorage.getTopLevelPackages().size() == 1);
}

TEST_CASE("Dependency relationship crash")
{
    // There was a bug where relationship between components raised a crash in this specific situation
    // This test has been added to avoid regression.
    auto tmpDir = TmpDir{"dep_rel_test"};
    auto dbPath = tmpDir.path() / "codedb.db";
    auto nodeStorage = NodeStorageTestUtils::createEmptyNodeStorage(dbPath);

    auto *pkg = nodeStorage.addPackage("pkg", "pkg", nullptr, std::any()).value();
    auto *a = nodeStorage.addComponent("a", "a", pkg).value();
    auto *b = nodeStorage.addComponent("b", "b", pkg).value();

    {
        REQUIRE_FALSE(a->hasProvider(b));
        auto r = nodeStorage.addPhysicalDependency(a, b);
        REQUIRE_FALSE(r.has_error());
        REQUIRE(a->hasProvider(b));
    }

    {
        REQUIRE(a->hasProvider(b));
        nodeStorage.removePhysicalDependency(a, b).expect("");
        REQUIRE_FALSE(a->hasProvider(b));
    }

    {
        REQUIRE_FALSE(a->hasProvider(b));
        auto r = nodeStorage.addPhysicalDependency(a, b);
        REQUIRE_FALSE(r.has_error());
        REQUIRE(a->hasProvider(b));
    }
}

TEST_CASE("User defined types")
{
    auto tmpDir = TmpDir{"udt_test"};
    auto dbPath = tmpDir.path() / "codedb.db";
    auto nodeStorage = NodeStorageTestUtils::createEmptyNodeStorage(dbPath);

    auto *zoo = nodeStorage.addPackage("zoo", "zoo", nullptr).value();

    auto *animal = nodeStorage.addComponent("animal", "animal", zoo).value();
    auto *domesticAnimalInterface = dynamic_cast<TypeNode *>(
        nodeStorage.addLogicalEntity("IDomesticAnimal", "cmp::IDomesticAnimal", animal, UDTKind::Class).value());
    auto *catClass =
        dynamic_cast<TypeNode *>(nodeStorage.addLogicalEntity("Cat", "animal::Cat", animal, UDTKind::Class).value());
    auto *dogClass =
        dynamic_cast<TypeNode *>(nodeStorage.addLogicalEntity("Dog", "animal::Dog", animal, UDTKind::Class).value());

    REQUIRE_FALSE(catClass->hasProvider(domesticAnimalInterface));
    REQUIRE_FALSE(dogClass->hasProvider(domesticAnimalInterface));
    nodeStorage.addLogicalRelation(catClass, domesticAnimalInterface, LakosRelationType::IsA)
        .expect("Unexpected error");
    nodeStorage.addLogicalRelation(dogClass, domesticAnimalInterface, LakosRelationType::IsA)
        .expect("Unexpected error");
    REQUIRE(catClass->hasProvider(domesticAnimalInterface));
    REQUIRE(dogClass->hasProvider(domesticAnimalInterface));

    auto *animalBuilder = nodeStorage.addComponent("animal_builder", "animal_builder", zoo).value();
    auto *animalBuilderClass = dynamic_cast<TypeNode *>(
        nodeStorage.addLogicalEntity("AnimalBuilder", "animal_builder::AnimalBuilder", animalBuilder, UDTKind::Class)
            .value());
    {
        // Cannot add dependency between classes on different components if the components aren't depending on each
        // other.
        REQUIRE_FALSE(animalBuilderClass->hasProvider(catClass));
        auto r =
            nodeStorage.addLogicalRelation(animalBuilderClass, catClass, LakosRelationType::UsesInTheImplementation);
        REQUIRE(r.has_error());
        REQUIRE(r.error().kind == ErrorAddLogicalRelation::Kind::ComponentDependencyRequired);
        REQUIRE_FALSE(animalBuilderClass->hasProvider(catClass));
    }
    nodeStorage.addPhysicalDependency(animalBuilder, animal).expect("Unexpected error");
    REQUIRE_FALSE(animalBuilderClass->hasProvider(catClass));
    REQUIRE_FALSE(animalBuilderClass->hasProvider(dogClass));
    nodeStorage.addLogicalRelation(animalBuilderClass, catClass, LakosRelationType::UsesInTheImplementation)
        .expect("Unexpected error");
    nodeStorage.addLogicalRelation(animalBuilderClass, dogClass, LakosRelationType::UsesInTheImplementation)
        .expect("Unexpected error");
    REQUIRE(animalBuilderClass->hasProvider(catClass));
    REQUIRE(animalBuilderClass->hasProvider(dogClass));
    {
        // Cannot add a dependency twice
        auto r =
            nodeStorage.addLogicalRelation(animalBuilderClass, catClass, LakosRelationType::UsesInTheImplementation);
        REQUIRE(r.has_error());
        REQUIRE(r.error().kind == ErrorAddLogicalRelation::Kind::AlreadyHaveDependency);
    }

    {
        // Cannot add a dependency from a UDT to itself
        auto r = nodeStorage.addLogicalRelation(animalBuilderClass, animalBuilderClass, LakosRelationType::IsA);
        REQUIRE(r.has_error());
        REQUIRE(r.error().kind == ErrorAddLogicalRelation::Kind::SelfRelation);
        REQUIRE_FALSE(animalBuilderClass->hasProvider(animalBuilderClass));
    }
    {
        // Invalid relationship types must generate an error
        auto r = nodeStorage.addLogicalRelation(dogClass, animalBuilderClass, LakosRelationType::None);
        REQUIRE(r.has_error());
        REQUIRE(r.error().kind == ErrorAddLogicalRelation::Kind::InvalidLakosRelationType);
        REQUIRE_FALSE(dogClass->hasProvider(animalBuilderClass));
    }

    REQUIRE(catClass->hasProvider(domesticAnimalInterface));
    nodeStorage.removeLogicalRelation(catClass, domesticAnimalInterface, LakosRelationType::IsA)
        .expect("Unexpected error");
    REQUIRE_FALSE(catClass->hasProvider(domesticAnimalInterface));

    {
        // Cannot remove a relation that doesn't exist (There is an "Is A" relation between dog and domesticAnimal, but
        // there is no "UsesInTheImplementation" dependency between dog and domesticAnimal)
        REQUIRE(dogClass->hasProvider(domesticAnimalInterface));
        auto r = nodeStorage.removeLogicalRelation(dogClass,
                                                   domesticAnimalInterface,
                                                   LakosRelationType::UsesInTheImplementation);
        REQUIRE(r.has_error());
        REQUIRE(r.error().kind == ErrorRemoveLogicalRelation::Kind::InexistentRelation);
        REQUIRE(dogClass->hasProvider(domesticAnimalInterface));
    }
}

TEST_CASE("Reparent entity on NodeStorage")
{
    auto tmpDir = TmpDir{"reparent_entity_test"};
    auto dbPath = tmpDir.path() / "codedb.db";
    auto nodeStorage = NodeStorageTestUtils::createEmptyNodeStorage(dbPath);

    auto *a = nodeStorage.addPackage("a", "a").value();
    auto *b = nodeStorage.addPackage("b", "b").value();
    auto *aa = nodeStorage.addComponent("aa", "aa", a).value();
    (void) nodeStorage.addLogicalEntity("some_class", "some_class", aa, Codethink::lvtshr::UDTKind::Class).value();

    REQUIRE(aa->parent() == a);
    REQUIRE(a->children().size() == 1);
    REQUIRE(a->children()[0] == aa);
    REQUIRE(b->children().empty());

    nodeStorage.reparentEntity(aa, b).expect(".");

    REQUIRE(aa->parent() == b);
    REQUIRE(b->children().size() == 1);
    REQUIRE(b->children()[0] == aa);
    REQUIRE(a->children().empty());
}

TEST_CASE("Check Lakosian nodes")
{
    auto tmpDir = TmpDir{"ns_lakosian_node_test"};
    auto dbPath = tmpDir.path() / "codedb.db";

    {
        auto nodeStorage = NodeStorageTestUtils::createEmptyNodeStorage(dbPath);

        // High level package
        auto *aaa = nodeStorage.addPackage("aaa", "aaa").value();
        REQUIRE(aaa->isLakosian() == LakosianNode::IsLakosianResult::IsLakosian);
        auto *b = nodeStorage.addPackage("b", "b").value();
        REQUIRE_FALSE(b->isLakosian() == LakosianNode::IsLakosianResult::IsLakosian);

        // Package
        auto *aaaxxx = nodeStorage.addPackage("aaaxxx", "aaa/aaaxxx", aaa).value();
        REQUIRE(aaaxxx->isLakosian() == LakosianNode::IsLakosianResult::IsLakosian);
        auto *aaalele = nodeStorage.addPackage("aaalele", "aaa/aaalele", aaa).value();
        REQUIRE_FALSE(aaalele->isLakosian() == LakosianNode::IsLakosianResult::IsLakosian);
        auto *aaa_with_std = nodeStorage.addPackage("aaa+std", "aaa_std", aaa).value();
        REQUIRE(aaa_with_std->isLakosian() == LakosianNode::IsLakosianResult::IsLakosian);

        // Component
        auto *aaaxxx_bla = nodeStorage.addComponent("aaaxxx_bla", "aaaxxx/aaaxxx_bla", aaaxxx).value();
        REQUIRE(aaaxxx_bla->isLakosian() == LakosianNode::IsLakosianResult::IsLakosian);
        auto *aaaxxxnonlak = nodeStorage.addComponent("aaaxxxnonlak", "aaaxxx/aaaxxxnonlak", aaaxxx).value();
        REQUIRE_FALSE(aaaxxxnonlak->isLakosian() == LakosianNode::IsLakosianResult::IsLakosian);

        // UDT
        auto *klass =
            nodeStorage.addLogicalEntity("Klass", "aaaxxx::aaaxxx_bla::Klass", aaaxxx_bla, UDTKind::Class).value();
        REQUIRE(klass->isLakosian() == LakosianNode::IsLakosianResult::IsLakosian);
    }

    {
        NodeStorage nodeStorage;
        nodeStorage.setDatabaseSourcePath(dbPath.string());

        // Requesting an invalid component must not crash
        auto *invalid = nodeStorage.findByQualifiedName(DiagramType::ComponentType, "invalid");
        REQUIRE(invalid == nullptr);

        auto *aaaxxx_bla = nodeStorage.findByQualifiedName(DiagramType::ComponentType, "aaaxxx/aaaxxx_bla");
        REQUIRE(aaaxxx_bla);
        REQUIRE(aaaxxx_bla->qualifiedName() == "aaaxxx/aaaxxx_bla");
        REQUIRE(aaaxxx_bla->name() == "aaaxxx_bla");
    }
}

TEST_CASE("Test Project Database Loads Successfully")
{
    Codethink::lvtprj::ProjectFile project;

    auto err = project.createEmpty();
    REQUIRE_FALSE(err.has_error());

    Codethink::lvtmdb::ObjectStore store;
    store.withRWLock([&] {
        auto *pkg = store.getOrAddPackage("ble", "ble", "ble", nullptr, nullptr);
        REQUIRE(pkg);
    });

    const auto code_file = project.codeDatabasePath();
    Codethink::lvtmdb::SociWriter writer;
    writer.createOrOpen(code_file.string(), "codebase_db.sql");
    store.writeToDatabase(writer);

    auto res = project.resetCadDatabaseFromCodeDatabase();
    REQUIRE_FALSE(res.has_error());

    auto dbPath = project.cadDatabasePath();

    NodeStorage storage;
    storage.setDatabaseSourcePath(project.cadDatabasePath().string());
    REQUIRE_FALSE(storage.getTopLevelPackages().empty());

    // Test - setNotes used to crash when a database for cad
    // is created based on a code database.
    auto node = storage.getTopLevelPackages()[0];
    node->setNotes("ABC");
}
