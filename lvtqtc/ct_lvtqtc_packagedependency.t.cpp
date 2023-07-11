// ct_lvtqtc_packagedependency.t.cpp                                  -*-C++-*-

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
#include <ct_lvtldr_nodestoragetestutils.h>
#include <ct_lvtqtc_nodestorage_testing_helpers.h>
#include <ct_lvtqtc_packagedependency.h>
#include <ct_lvttst_fixture_qt.h>
#include <ct_lvttst_tmpdir.h>
#include <optional>

using namespace Codethink::lvtqtc;
using namespace Codethink::lvtshr;
using namespace Codethink::lvtldr;

namespace Codethink::lvtqtc {

class PackageDependencyTestFriendAdapter {
  public:
    using Flavor = PackageDependency::PackageDependencyFlavor;

    static Flavor getFlavorFrom(PackageDependency const& p)
    {
        return p.d_flavor;
    }
};

} // namespace Codethink::lvtqtc

using TestAdapter = PackageDependencyTestFriendAdapter;
using Constraint = PackageDependency::PackageDependencyConstraint;

TEST_CASE_METHOD(QTApplicationFixture, "Package dependency rules")
{
    auto tmpDir = TmpDir{"pkg_dep_rules"};
    auto dbPath = tmpDir.path() / "codedb.db";
    auto ns = NodeStorageTestUtils::createEmptyNodeStorage(dbPath);

    // Rules for standalone Packages
    {
        auto source = ScopedPackageEntity(ns, "pkgA1");
        auto target = ScopedPackageEntity(ns, "pkgB1");
        {
            auto dep = ScopedPackageDependency(ns, source.value(), target.value(), Constraint::AllowedDependency);
            REQUIRE(TestAdapter::getFlavorFrom(dep.value()) == TestAdapter::Flavor::AllowedDependency);
            auto dep2 = ScopedPackageDependency(ns, source.value(), target.value(), Constraint::ConcreteDependency);
            dep.value().updateFlavor();
            REQUIRE(TestAdapter::getFlavorFrom(dep.value()) == TestAdapter::Flavor::ConcreteDependency);
        }
        {
            // A concrete dependency that's not allowed results in an invalid design
            auto dep = ScopedPackageDependency(ns, source.value(), target.value(), Constraint::ConcreteDependency);
            dep.value().updateFlavor();
            REQUIRE(TestAdapter::getFlavorFrom(dep.value()) == TestAdapter::Flavor::InvalidDesignDependency);
        }
    }

    // Rules for Packages inside same Package Groups
    {
        auto parent = ScopedPackageEntity(ns, "pkggrp");
        auto source = ScopedPackageEntity(ns, "pkgA2", parent.value());
        auto target = ScopedPackageEntity(ns, "pkgB2", parent.value());
        {
            auto dep = ScopedPackageDependency(ns, source.value(), target.value(), Constraint::AllowedDependency);
            REQUIRE(TestAdapter::getFlavorFrom(dep.value()) == TestAdapter::Flavor::AllowedDependency);
            auto dep2 = ScopedPackageDependency(ns, source.value(), target.value(), Constraint::ConcreteDependency);
            dep.value().updateFlavor();
            REQUIRE(TestAdapter::getFlavorFrom(dep.value()) == TestAdapter::Flavor::ConcreteDependency);
        }
    }
    {
        auto parent = ScopedPackageEntity(ns, "pkggrp");
        auto source = ScopedPackageEntity(ns, "pkgA2", parent.value());
        auto target = ScopedPackageEntity(ns, "pkgB2", parent.value());
        {
            // A concrete dependency that's not allowed results in an invalid design
            auto dep = ScopedPackageDependency(ns, source.value(), target.value(), Constraint::ConcreteDependency);
            REQUIRE(TestAdapter::getFlavorFrom(dep.value()) == TestAdapter::Flavor::InvalidDesignDependency);
        }
    }
    {
        auto parent = ScopedPackageEntity(ns, "pkggrp");
        auto source = ScopedPackageEntity(ns, "pkggrp/pkgA2", parent.value());
        auto componentATest = ScopedComponentEntity(ns, "pkgA2::componentA.t", source.value());
        auto target = ScopedPackageEntity(ns, "pkggrp/pkgB2", parent.value());
        auto componentB = ScopedComponentEntity(ns, "pkgB2::componentB", target.value());
        {
            // A concrete dependency that`s only consumed by tests will have a special flavor for it
            auto dep = ScopedPackageDependency(ns, source.value(), target.value(), Constraint::AllowedDependency);
            REQUIRE(TestAdapter::getFlavorFrom(dep.value()) == TestAdapter::Flavor::AllowedDependency);
            auto compDep =
                ScopedPackageDependency(ns, componentATest.value(), componentB.value(), Constraint::ConcreteDependency);
            auto dep2 = ScopedPackageDependency(ns, source.value(), target.value(), Constraint::ConcreteDependency);
            dep.value().updateFlavor();
            REQUIRE(TestAdapter::getFlavorFrom(dep.value()) == TestAdapter::Flavor::TestOnlyConcreteDependency);
        }
    }

    // Rules for Packages inside different Package Groups
    {
        auto parentA = ScopedPackageEntity(ns, "pkggrA");
        auto source = ScopedPackageEntity(ns, "pkgA", parentA.value());
        auto parentB = ScopedPackageEntity(ns, "pkggrB");
        auto target = ScopedPackageEntity(ns, "pkgB", parentB.value());
        {
            auto parentDep =
                ScopedPackageDependency(ns, parentA.value(), parentB.value(), Constraint::AllowedDependency);
            auto dep = ScopedPackageDependency(ns, source.value(), target.value(), Constraint::AllowedDependency);
            REQUIRE(TestAdapter::getFlavorFrom(dep.value()) == TestAdapter::Flavor::AllowedDependency);
            REQUIRE(TestAdapter::getFlavorFrom(parentDep.value()) == TestAdapter::Flavor::AllowedDependency);
            auto dep2 = ScopedPackageDependency(ns, source.value(), target.value(), Constraint::ConcreteDependency);
            dep.value().updateFlavor();
            parentDep.value().updateFlavor();
            REQUIRE(TestAdapter::getFlavorFrom(dep.value()) == TestAdapter::Flavor::ConcreteDependency);
            REQUIRE(TestAdapter::getFlavorFrom(parentDep.value()) == TestAdapter::Flavor::ConcreteDependency);
        }
    }

    // Rules for Packages inside non-lakosian package group
    {
        auto parentA = ScopedPackageEntity(ns, "non-lakosian group");
        auto source = ScopedPackageEntity(ns, "pkgA", parentA.value());
        auto target = ScopedPackageEntity(ns, "pkgB", parentA.value());
        {
            auto dep = ScopedPackageDependency(ns, source.value(), target.value(), Constraint::AllowedDependency);
            // packages within non-lakosian group are always implicitly allowed
            REQUIRE(TestAdapter::getFlavorFrom(dep.value()) == TestAdapter::Flavor::ConcreteDependency);
            auto dep2 = ScopedPackageDependency(ns, source.value(), target.value(), Constraint::ConcreteDependency);
            dep.value().updateFlavor();
            REQUIRE(TestAdapter::getFlavorFrom(dep.value()) == TestAdapter::Flavor::ConcreteDependency);
        }
    }

    // Rules for standalone packages interacting with package groups
    {
        auto parent = ScopedPackageEntity(ns, "groups/pkggrp");
        auto source = ScopedPackageEntity(ns, "groups/pkggrp/pkgA2", parent.value());
        auto target = ScopedPackageEntity(ns, "standalones/standalone");
        {
            auto dep = ScopedPackageDependency(ns, parent.value(), target.value(), Constraint::AllowedDependency);
            REQUIRE(TestAdapter::getFlavorFrom(dep.value()) == TestAdapter::Flavor::AllowedDependency);
            auto dep2 = ScopedPackageDependency(ns, parent.value(), target.value(), Constraint::ConcreteDependency);
            dep2.value().updateFlavor();
            REQUIRE(TestAdapter::getFlavorFrom(dep2.value()) == TestAdapter::Flavor::ConcreteDependency);
        }
    }
}
