// ct_lvtldr_lakosiannode.t.cpp                                       -*-C++-*-

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

#include <ct_lvtldr_componentnode.h>
#include <ct_lvtldr_lakosiannode.h>
#include <ct_lvtldr_nodestorage.h>

#include <ct_lvtmdb_soci_writer.h>

#include <ct_lvtclp_testutil.h>
#include <ct_lvtclp_tool.h>

#include <ct_lvtshr_graphenums.h>
#include <ct_lvtshr_uniqueid.h>

#include <ct_lvttst_tmpdir.h>

#include <filesystem>
#include <iostream>

#include <catch2/catch.hpp>

PyDefaultGilReleasedContext _;

namespace {

using namespace Codethink;
using namespace Codethink::lvtclp;
using namespace Codethink::lvtldr;
using namespace Codethink::lvtshr;

void createTop(const std::filesystem::path& topLevel)
{
    std::filesystem::create_directories(topLevel / "groups/one/onetop");

    Test_Util::createFile(topLevel / "groups/one/onetop/onetop_top.h", R"(
// onetop_top.h

namespace onetop {

struct Top {
    using NestedType = int;

    static NestedType method();
};

})");
    Test_Util::createFile(topLevel / "groups/one/onetop/onetop_top.cpp", R"(
// onetop_top.cpp

#include <onetop_top.h>

namespace onetop {

Top::NestedType Top::method()
{
    return 2;
}

})");
}

void createDep(const std::filesystem::path& topLevel)
{
    std::filesystem::create_directories(topLevel / "groups/two/twodep");

    Test_Util::createFile(topLevel / "groups/two/twodep/twodep_dep.h", R"(
// twodep_dep.h

#include <onetop_top.h>

namespace twodep {

class Dep {
  public:
    onetop::Top::NestedType method();
};

class IsADep : public Dep {
};

})");
    Test_Util::createFile(topLevel / "groups/two/twodep/twodep_dep.cpp", R"(
// twodep_dep.cpp

#include <twodep_dep.h>

namespace twodep {

int Dep::method()
{
    return onetop::Top::method();
}

})");
}

void createTestEnv(const std::filesystem::path& topLevel)
{
    if (std::filesystem::exists(topLevel)) {
        REQUIRE(std::filesystem::remove_all(topLevel));
    }

    createTop(topLevel);
    createDep(topLevel);
}

void checkPackageGroups(NodeStorage& store)
{
    LakosianNode *oneNode = store.findByQualifiedName(DiagramType::PackageType, "groups/one");
    REQUIRE(oneNode);
    LakosianNode *twoNode = store.findByQualifiedName(DiagramType::PackageType, "groups/two");
    REQUIRE(twoNode);

    // package group qualified name
    REQUIRE(oneNode->qualifiedName() == "groups/one");
    REQUIRE(twoNode->qualifiedName() == "groups/two");

    // package group type
    REQUIRE(oneNode->type() == lvtshr::DiagramType::PackageType);
    REQUIRE(twoNode->type() == lvtshr::DiagramType::PackageType);

    // package group forward dependencies: two -> one
    const std::vector<LakosianEdge> expectedProviders{
        LakosianEdge(lvtshr::PackageDependency, oneNode),
    };
    REQUIRE(oneNode->providers().empty());
    REQUIRE(twoNode->providers() == expectedProviders);

    // package group reverse dependencies: one <- two
    const std::vector<LakosianEdge> expectedClients{
        LakosianEdge(lvtshr::PackageDependency, twoNode),
    };
    REQUIRE(oneNode->clients() == expectedClients);
    REQUIRE(twoNode->clients().empty());

    // package group parents should be null
    REQUIRE(!oneNode->parent());
    REQUIRE(!twoNode->parent());

    // package group children
    LakosianNode *onetopNode = store.findByQualifiedName(DiagramType::PackageType, "groups/one/onetop");
    REQUIRE(onetopNode);
    LakosianNode *twodepNode = store.findByQualifiedName(DiagramType::PackageType, "groups/two/twodep");
    REQUIRE(twodepNode);

    const std::vector<LakosianNode *> oneChildren{
        onetopNode,
    };
    REQUIRE(oneNode->children() == oneChildren);

    const std::vector<LakosianNode *> twoChildren{
        twodepNode,
    };
    REQUIRE(twoNode->children() == twoChildren);
}

void checkPackages(NodeStorage& store)
{
    LakosianNode *onetopNode = store.findByQualifiedName(DiagramType::PackageType, "groups/one/onetop");
    REQUIRE(onetopNode);
    LakosianNode *twodepNode = store.findByQualifiedName(DiagramType::PackageType, "groups/two/twodep");
    REQUIRE(twodepNode);

    // pacakge qualified name
    REQUIRE(onetopNode->qualifiedName() == "groups/one/onetop");
    REQUIRE(twodepNode->qualifiedName() == "groups/two/twodep");

    // package type
    REQUIRE(onetopNode->type() == lvtshr::DiagramType::PackageType);
    REQUIRE(twodepNode->type() == lvtshr::DiagramType::PackageType);

    // package forward dependencies: twodep -> onetop
    const std::vector<LakosianEdge> expectedproviders{
        LakosianEdge(lvtshr::PackageDependency, onetopNode),
    };
    REQUIRE(onetopNode->providers().empty());
    REQUIRE(twodepNode->providers() == expectedproviders);

    // package reverse dependencies: onetop <- twodep
    const std::vector<LakosianEdge> expectedclients{
        LakosianEdge(lvtshr::PackageDependency, twodepNode),
    };
    REQUIRE(onetopNode->clients() == expectedclients);
    REQUIRE(twodepNode->clients().empty());

    // package parents
    LakosianNode *oneNode = store.findByQualifiedName(DiagramType::PackageType, "groups/one");
    REQUIRE(oneNode);
    LakosianNode *twoNode = store.findByQualifiedName(DiagramType::PackageType, "groups/two");
    REQUIRE(twoNode);

    REQUIRE(onetopNode->parent() == oneNode);
    REQUIRE(twodepNode->parent() == twoNode);

    // package children
    LakosianNode *onetopCompNode =
        store.findByQualifiedName(DiagramType::ComponentType, "groups/one/onetop/onetop_top");
    REQUIRE(onetopCompNode);
    LakosianNode *twodepCompNode =
        store.findByQualifiedName(DiagramType::ComponentType, "groups/two/twodep/twodep_dep");
    REQUIRE(twodepCompNode);

    const std::vector<LakosianNode *> onetopChildren{
        onetopCompNode,
    };
    REQUIRE(onetopNode->children() == onetopChildren);

    const std::vector<LakosianNode *> twodepChildren{
        twodepCompNode,
    };
    REQUIRE(twodepNode->children() == twodepChildren);
}

void checkComponents(NodeStorage& store)
{
    LakosianNode *onetopCompNode =
        store.findByQualifiedName(DiagramType::ComponentType, "groups/one/onetop/onetop_top");
    REQUIRE(onetopCompNode);
    LakosianNode *twodepCompNode =
        store.findByQualifiedName(DiagramType::ComponentType, "groups/two/twodep/twodep_dep");
    REQUIRE(twodepCompNode);

    // component qualified name
    REQUIRE(onetopCompNode->qualifiedName() == "groups/one/onetop/onetop_top");
    REQUIRE(twodepCompNode->qualifiedName() == "groups/two/twodep/twodep_dep");

    // component type
    REQUIRE(onetopCompNode->type() == lvtshr::DiagramType::ComponentType);
    REQUIRE(twodepCompNode->type() == lvtshr::DiagramType::ComponentType);

    // component forward dependencies twodep_dep -> onetop_top
    const std::vector<LakosianEdge> expectedproviders{
        LakosianEdge(lvtshr::PackageDependency, onetopCompNode),
    };
    REQUIRE(onetopCompNode->providers().empty());
    REQUIRE(twodepCompNode->providers() == expectedproviders);

    // component reverse dependencies onetop_top <- twodep_dep
    const std::vector<LakosianEdge> expectedclients{
        LakosianEdge(lvtshr::PackageDependency, twodepCompNode),
    };
    REQUIRE(onetopCompNode->clients() == expectedclients);
    REQUIRE(twodepCompNode->clients().empty());

    // component parents
    LakosianNode *onetopNode = store.findByQualifiedName(DiagramType::PackageType, "groups/one/onetop");
    REQUIRE(onetopNode);
    LakosianNode *twodepNode = store.findByQualifiedName(DiagramType::PackageType, "groups/two/twodep");
    REQUIRE(twodepNode);

    REQUIRE(onetopCompNode->parent() == onetopNode);
    REQUIRE(twodepCompNode->parent() == twodepNode);

    // component children
    LakosianNode *topNode = store.findByQualifiedName(DiagramType::ClassType, "onetop::Top");
    REQUIRE(topNode);
    LakosianNode *depNode = store.findByQualifiedName(DiagramType::ClassType, "twodep::Dep");
    REQUIRE(depNode);
    LakosianNode *isADepNode = store.findByQualifiedName(DiagramType::ClassType, "twodep::IsADep");
    REQUIRE(isADepNode);

    const std::vector<LakosianNode *> onetopChildren{
        topNode,
    };
    REQUIRE(onetopCompNode->children() == onetopChildren);

    const std::vector<LakosianNode *> twodepChildren{
        isADepNode,
        depNode,
    };
    REQUIRE(twodepCompNode->children() == twodepChildren);
}

void checkUDTs(NodeStorage& store)
{
    LakosianNode *topNode = store.findByQualifiedName(DiagramType::ClassType, "onetop::Top");
    REQUIRE(topNode);
    LakosianNode *nestedNode = store.findByQualifiedName(DiagramType::ClassType, "onetop::Top::NestedType");
    REQUIRE(nestedNode);
    LakosianNode *depNode = store.findByQualifiedName(DiagramType::ClassType, "twodep::Dep");
    REQUIRE(depNode);
    LakosianNode *isADepNode = store.findByQualifiedName(DiagramType::ClassType, "twodep::IsADep");
    REQUIRE(isADepNode);

    // type qualifiedName
    REQUIRE(topNode->qualifiedName() == "onetop::Top");
    REQUIRE(nestedNode->qualifiedName() == "onetop::Top::NestedType");
    REQUIRE(depNode->qualifiedName() == "twodep::Dep");
    REQUIRE(isADepNode->qualifiedName() == "twodep::IsADep");

    // udt type
    REQUIRE(topNode->type() == lvtshr::DiagramType::ClassType);
    REQUIRE(nestedNode->type() == lvtshr::DiagramType::ClassType);
    REQUIRE(depNode->type() == lvtshr::DiagramType::ClassType);
    REQUIRE(isADepNode->type() == lvtshr::DiagramType::ClassType);

    // udt fwd deps: twodep::Dep uses-in-impl onetop::Top
    // TODO: test isA and usesInTheInterface
    const std::vector<LakosianEdge> expectedproviders{
        LakosianEdge(lvtshr::UsesInTheInterface, nestedNode),
        LakosianEdge(lvtshr::UsesInTheImplementation, topNode),
    };
    const std::vector<LakosianEdge> isAproviders{
        LakosianEdge(lvtshr::IsA, depNode),
    };

    for (auto&& p : topNode->providers()) {
        std::cout << p.type() << "\n";
        std::cout << p.other()->qualifiedName() << "\n";
    }

    REQUIRE(topNode->providers().empty());
    REQUIRE(nestedNode->providers().empty());
    REQUIRE(depNode->providers() == expectedproviders);
    REQUIRE(isADepNode->providers() == isAproviders);

    // udt rev deps: twodep::Dbo uses-in-impl onetop::Top
    const std::vector<LakosianEdge> expectedclients{
        LakosianEdge(lvtshr::UsesInTheImplementation, depNode),
    };
    const std::vector<LakosianEdge> nestedclients{
        LakosianEdge(lvtshr::UsesInTheInterface, depNode),
    };
    const std::vector<LakosianEdge> isAclients{
        LakosianEdge(lvtshr::IsA, isADepNode),
    };
    REQUIRE(topNode->clients() == expectedclients);
    REQUIRE(nestedNode->clients() == nestedclients);
    REQUIRE(depNode->clients() == isAclients);
    REQUIRE(isADepNode->clients().empty());
    // udt parents
    LakosianNode *onetopCompNode =
        store.findByQualifiedName(DiagramType::ComponentType, "groups/one/onetop/onetop_top");
    REQUIRE(onetopCompNode);
    LakosianNode *twodepCompNode =
        store.findByQualifiedName(DiagramType::ComponentType, "groups/two/twodep/twodep_dep");
    REQUIRE(twodepCompNode);

    REQUIRE(topNode->parent() == onetopCompNode);
    REQUIRE(nestedNode->parent() == topNode);
    REQUIRE(depNode->parent() == twodepCompNode);
    REQUIRE(isADepNode->parent() == twodepCompNode);

    // udt children
    const std::vector<LakosianNode *> topChildren{
        nestedNode,
    };
    REQUIRE(topNode->children() == topChildren);
    REQUIRE(nestedNode->children().empty());
    REQUIRE(depNode->children().empty());
    REQUIRE(isADepNode->children().empty());
}

struct LakosianNodeTestFixture {
    LakosianNodeTestFixture(): topLevel(std::filesystem::temp_directory_path() / "ct_lvtldr_lakosiannode_test")
    {
        createTestEnv(topLevel);
    }

    ~LakosianNodeTestFixture()
    {
        REQUIRE(std::filesystem::remove_all(topLevel));
    }

    std::filesystem::path topLevel;
};
} // namespace

TEST_CASE_METHOD(LakosianNodeTestFixture, "Lakosian nodes test")
{
    StaticCompilationDatabase cmds({{"groups/one/onetop/onetop_top.cpp", "build/onetop_top.o"},
                                    {"groups/two/twodep/twodep_dep.cpp", "build/onedep_dep.o"}},
                                   "placeholder",
                                   {"-Igroups/one/onetop", "-Igroups/two/twodep"},
                                   topLevel);

    auto tmpDir = TmpDir{"lakosian_nodes_test"};
    auto dbPath = tmpDir.path() / "codedb.db";
    Tool tool(topLevel, cmds, dbPath);
    REQUIRE(tool.runFull());

    lvtmdb::SociWriter writer;
    writer.createOrOpen(dbPath.string(), "cad_db.sql");
    tool.getObjectStore().writeToDatabase(writer);

    NodeStorage store;
    store.setDatabaseSourcePath(dbPath.string());

    checkPackageGroups(store);
    checkPackages(store);
    checkComponents(store);
    checkUDTs(store);
}

TEST_CASE_METHOD(LakosianNodeTestFixture, "find entities")
{
    StaticCompilationDatabase cmds({{"groups/one/onetop/onetop_top.cpp", "build/onetop_top.o"},
                                    {"groups/two/twodep/twodep_dep.cpp", "build/onedep_dep.o"}},
                                   "placeholder",
                                   {"-Igroups/one/onetop", "-Igroups/two/twodep"},
                                   topLevel);

    auto tmpDir = TmpDir{"find_entities_test"};
    auto dbPath = tmpDir.path() / "codedb.db";
    Tool tool(topLevel, cmds, dbPath);
    REQUIRE(tool.runFull());

    lvtmdb::SociWriter writer;
    writer.createOrOpen(dbPath.string(), "cad_db.sql");
    tool.getObjectStore().writeToDatabase(writer);

    NodeStorage store;
    store.setDatabaseSourcePath(dbPath.string());

    auto *some_class = store.findById(UniqueId{DiagramType::ClassType, 1});
    auto *some_component = store.findById(UniqueId{DiagramType::ComponentType, 1});
    auto *some_package = store.findById(UniqueId{DiagramType::PackageType, 1});

    REQUIRE(some_class != nullptr);
    REQUIRE(some_component != nullptr);
    REQUIRE(some_package != nullptr);

    // Even though they have the same record number, different diagram types will yield return data
    REQUIRE(some_class != some_component);
    REQUIRE(some_class != some_package);

    // Getting the same object twice will return the same pointer to it
    REQUIRE(some_class == store.findById(UniqueId{DiagramType::ClassType, 1}));
    REQUIRE(some_component == store.findById(UniqueId{DiagramType::ComponentType, 1}));
    REQUIRE(some_package == store.findById(UniqueId{DiagramType::PackageType, 1}));

    // Getting them from findByQualifiedName will return the same pointer
    REQUIRE(some_class == store.findByQualifiedName(some_class->type(), some_class->qualifiedName()));
    REQUIRE(some_component == store.findByQualifiedName(some_component->type(), some_component->qualifiedName()));
    REQUIRE(some_package == store.findByQualifiedName(some_package->type(), some_package->qualifiedName()));
}

TEST_CASE_METHOD(LakosianNodeTestFixture, "retrieve child structures")
{
    StaticCompilationDatabase cmds({{"groups/one/onetop/onetop_top.cpp", "build/onetop_top.o"},
                                    {"groups/two/twodep/twodep_dep.cpp", "build/onedep_dep.o"}},
                                   "placeholder",
                                   {"-Igroups/one/onetop", "-Igroups/two/twodep"},
                                   topLevel);
    auto tmpDir = TmpDir{"find_entities_test"};
    auto dbPath = tmpDir.path() / "codedb.db";
    Tool tool(topLevel, cmds, dbPath);
    REQUIRE(tool.runFull());

    lvtmdb::SociWriter writer;
    writer.createOrOpen(dbPath.string(), "cad_db.sql");
    tool.getObjectStore().writeToDatabase(writer);

    NodeStorage store;
    store.setDatabaseSourcePath(dbPath.string());

    {
        auto pkgGroups = store.getTopLevelPackages();
        REQUIRE(pkgGroups.size() == 2);
        // Retrieving from `findById` returns the _same pointer_ as the one in the package group
        REQUIRE(pkgGroups[0] == store.findById({DiagramType::PackageType, pkgGroups[0]->id()}));
    }

    {
        auto pkgGroups = store.getTopLevelPackages();
        auto packages = pkgGroups[0]->children();
        auto components = packages[0]->children();
        auto *someComponent = dynamic_cast<ComponentNode *>(components[0]);
        REQUIRE(someComponent != nullptr);
    }
}

TEST_CASE_METHOD(LakosianNodeTestFixture, "parent hierarchy")
{
    StaticCompilationDatabase cmds({{"groups/one/onetop/onetop_top.cpp", "build/onetop_top.o"},
                                    {"groups/two/twodep/twodep_dep.cpp", "build/onedep_dep.o"}},
                                   "placeholder",
                                   {"-Igroups/one/onetop", "-Igroups/two/twodep"},
                                   topLevel);

    auto tmpDir = TmpDir{"parent_hierarchy_test"};
    auto dbPath = tmpDir.path() / "codedb.db";
    Tool tool(topLevel, cmds, dbPath);
    REQUIRE(tool.runFull());

    lvtmdb::SociWriter writer;
    writer.createOrOpen(dbPath.string(), "cad_db.sql");
    tool.getObjectStore().writeToDatabase(writer);

    NodeStorage ns;
    ns.setDatabaseSourcePath(dbPath.string());

    auto rootPackages = ns.getTopLevelPackages();
    REQUIRE(rootPackages.size() == 2);

    auto *rootPkgGrp = [&]() -> LakosianNode * {
        for (auto&& r : rootPackages) {
            if (r->qualifiedName() == "groups/one") {
                return r;
            }
        }
        return nullptr;
    }();
    auto childPackages = rootPkgGrp->children();
    REQUIRE(childPackages.size() == 1);

    auto packageQualifiedName = childPackages[0]->qualifiedName();
    REQUIRE(packageQualifiedName == "groups/one/onetop");
    auto components = childPackages[0]->children();
    REQUIRE(components.size() == 1);

    auto *childComponent = components[0];
    auto componentQualifiedName = childComponent->qualifiedName();
    REQUIRE(componentQualifiedName == "groups/one/onetop/onetop_top");

    auto hierarchy = childComponent->parentHierarchy();
    REQUIRE(hierarchy[0]->qualifiedName() == "groups/one");
    REQUIRE(hierarchy[1]->qualifiedName() == "groups/one/onetop");
    REQUIRE(hierarchy[2]->qualifiedName() == "groups/one/onetop/onetop_top");
}

TEST_CASE_METHOD(LakosianNodeTestFixture, "changing node storage")
{
    StaticCompilationDatabase cmds({{"groups/one/onetop/onetop_top.cpp", "build/onetop_top.o"},
                                    {"groups/two/twodep/twodep_dep.cpp", "build/onedep_dep.o"}},
                                   "placeholder",
                                   {"-Igroups/one/onetop", "-Igroups/two/twodep"},
                                   topLevel);

    auto tmpDir = TmpDir{"changing_node_storage_test"};
    auto dbPath = tmpDir.path() / "codedb.db";
    Tool tool(topLevel, cmds, dbPath);
    REQUIRE(tool.runFull());

    lvtmdb::SociWriter writer;
    writer.createOrOpen(dbPath.string(), "cad_db.sql");
    tool.getObjectStore().writeToDatabase(writer);

    NodeStorage ns;
    ns.setDatabaseSourcePath(dbPath.string());

    int n_changes = 0;
    ns.registerNodeNameChangedCallback(&n_changes, [&n_changes](const LakosianNode *node) {
        REQUIRE(node != nullptr);
        n_changes += 1;
    });

    auto rootPackages = ns.getTopLevelPackages();
    REQUIRE(rootPackages.size() == 2);

    auto *rootPkgGrp = [&]() -> LakosianNode * {
        for (auto&& r : rootPackages) {
            if (r->qualifiedName() == "groups/one") {
                return r;
            }
        }
        return nullptr;
    }();
    auto childPackages = rootPkgGrp->children();
    REQUIRE(childPackages.size() == 1);

    auto packageQualifiedName = childPackages[0]->qualifiedName();
    REQUIRE(packageQualifiedName == "groups/one/onetop");
    auto components = childPackages[0]->children();
    REQUIRE(components.size() == 1);

    auto componentQualifiedName = components[0]->qualifiedName();
    REQUIRE(componentQualifiedName == "groups/one/onetop/onetop_top");

    auto changeNode = [&](auto *node) {
        auto oldName = node->name();
        auto oldQualifiedName = node->qualifiedName();

        node->setName("othername");

        REQUIRE(node->name() != oldName);
        REQUIRE(node->qualifiedName() != oldQualifiedName);
    };

    REQUIRE(n_changes == 0);
    changeNode(rootPackages[0]);
    REQUIRE(n_changes == 1);
    changeNode(childPackages[0]);
    REQUIRE(n_changes == 2);
    changeNode(components[0]);
    REQUIRE(n_changes == 3);

    ns.unregisterAllCallbacksTo(&n_changes);
    // Changing things after this point won't affect the receiver
    components[0]->setName("someothername");
    REQUIRE(n_changes == 3);
}

TEST_CASE("qualified name building")
{
    {
        auto prefixParts = NamingUtils::buildQualifiedNamePrefixParts("xxx::yyyy::zz", "::");
        REQUIRE(prefixParts == std::vector<std::string>{"xxx", "yyyy"});
        REQUIRE(NamingUtils::buildQualifiedName(prefixParts, "other", "::") == "xxx::yyyy::other");
        REQUIRE(NamingUtils::buildQualifiedName(prefixParts, "other", "/") == "xxx/yyyy/other");
    }

    {
        auto prefixParts = NamingUtils::buildQualifiedNamePrefixParts("pkg/comp/udt", "/");
        REQUIRE(prefixParts == std::vector<std::string>{"pkg", "comp"});
        REQUIRE(NamingUtils::buildQualifiedName(prefixParts, "klass", "::") == "pkg::comp::klass");
        REQUIRE(NamingUtils::buildQualifiedName(prefixParts, "klass", "/") == "pkg/comp/klass");
    }

    // Edge case: There's no prefix
    {
        auto prefixParts = NamingUtils::buildQualifiedNamePrefixParts("myKlass", "::");
        REQUIRE(prefixParts.empty());
        REQUIRE(NamingUtils::buildQualifiedName(prefixParts, "otherKlass", "::") == "otherKlass");
    }

    // Edge case: Empty string prefix
    {
        auto prefixParts = NamingUtils::buildQualifiedNamePrefixParts("::myKlass", "::");
        REQUIRE(prefixParts == std::vector<std::string>{""});
        REQUIRE(NamingUtils::buildQualifiedName(prefixParts, "otherKlass", "::") == "::otherKlass");
    }

    // Edge case: There's no string at all!
    {
        auto prefixParts = NamingUtils::buildQualifiedNamePrefixParts("", "::");
        REQUIRE(prefixParts.empty());
        REQUIRE(NamingUtils::buildQualifiedName(prefixParts, "bla", "::") == "bla");
    }
}
