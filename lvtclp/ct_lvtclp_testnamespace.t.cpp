// ct_lvtclp_testnamespace.t.cpp                                       -*-C++-*-

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

// This file is home to tests for adding NamespaceDeclarations to the database

#include <ct_lvtmdb_fileobject.h>
#include <ct_lvtmdb_namespaceobject.h>
#include <ct_lvtmdb_objectstore.h>

#include <ct_lvtclp_testutil.h>

#include <algorithm>
#include <iostream>

#include <catch2-local-includes.h>

using namespace Codethink::lvtclp;
using namespace Codethink::lvtmdb;

const PyDefaultGilReleasedContext defaultGilContextForTesting;

TEST_CASE("Namespace declaration")
{
    const static char *source = "namespace foo {}";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testNamespaceDeclaration.cpp"));

    session.withROLock([&] {
        NamespaceObject *nmspc = session.getNamespace("foo");
        REQUIRE(nmspc);
    });
}

TEST_CASE("Anon namespace")
{
    const static char *source = "namespace {}";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testAnonNamespace.cpp"));

    // no namespace should have been added
    session.withROLock([&] {
        REQUIRE(session.namespaces().empty());
    });
}

TEST_CASE("Parent namespace")
{
    const static char *source = "namespace foo { namespace bar {} } namespace foo {}";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testParentNamespace.cpp"));

    NamespaceObject *foo = nullptr;
    NamespaceObject *bar = nullptr;
    FileObject *file = nullptr;

    session.withROLock([&] {
        foo = session.getNamespace("foo");
        bar = session.getNamespace("foo::bar");
        file = session.getFile("testParentNamespace.cpp");
    });

    REQUIRE(foo);
    REQUIRE(bar);
    REQUIRE(file);

    foo->withROLock([&] {
        REQUIRE(foo->name() == "foo");
    });

    bar->withROLock([&] {
        REQUIRE(bar->name() == "bar");
        REQUIRE(bar->parent() == foo);
    });

    foo->withROLock([&] {
        const auto children = foo->children();
        const auto childIt = std::find(children.begin(), children.end(), bar);
        REQUIRE(childIt != children.end());
    });

    // check both namespaces were added to the file
    std::vector<NamespaceObject *> namespaces;
    file->withROLock([&] {
        namespaces = file->namespaces();
    });

    auto it = std::find(namespaces.begin(), namespaces.end(), foo);
    REQUIRE(it != namespaces.end());
    it = std::find(namespaces.begin(), namespaces.end(), bar);
    REQUIRE(it != namespaces.end());

    // check the second namespace foo wasn't added all over again
    REQUIRE(namespaces.size() == 2);
}

TEST_CASE("Nested namespace")
{
    static const char *source = "namespace foo::bar {}";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testNestedNamespace.cpp"));
    NamespaceObject *foo = nullptr;
    NamespaceObject *bar = nullptr;

    session.withROLock([&] {
        foo = session.getNamespace("foo");
        bar = session.getNamespace("foo::bar");
    });

    foo->withROLock([&] {
        REQUIRE(foo->name() == "foo");
    });

    bar->withROLock([&] {
        REQUIRE(bar->name() == "bar");
        REQUIRE(bar->parent() == foo);
    });
}

TEST_CASE("No unnamed namespace")
{
    static const char *source = R"(
namespace {
namespace _u {
}
})";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testNoUnnamednamespace.cpp"));

    // no namespace should have been added
    session.withROLock([&] {
        REQUIRE(session.namespaces().empty());
    });
}
