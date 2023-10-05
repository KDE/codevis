// ct_lvtqtc_inspect_dependency_window.t.cpp                               -*-C++-*-

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
#include <ct_lvtldr_nodestoragetestutils.h>
#include <ct_lvtqtc_inspect_dependency_window.h>
#include <ct_lvtqtc_nodestorage_testing_helpers.h>
#include <ct_lvtqtc_testing_utils.h>

#include <catch2-local-includes.h>

#include <ct_lvttst_fixture_qt.h>
#include <ct_lvttst_tmpdir.h>

using namespace Codethink::lvtqtc;
using namespace Codethink::lvtprj;
using namespace Codethink::lvtldr;

class InspectDependencyWindowForTesting : public InspectDependencyWindow {
  public:
    using InspectDependencyWindow::InspectDependencyWindow;

    [[nodiscard]] int nRows()
    {
        return d_contentsTable->rowCount();
    }

    void showFileNotFoundWarning(QString const& filepath) const override
    {
        if (onFileNotFound) {
            (*onFileNotFound)(filepath);
        }
    }

    std::optional<std::function<void(QString const&)>> onFileNotFound = std::nullopt;
};

TEST_CASE_METHOD(QTApplicationFixture, "Basic workflow")
{
    auto tmpDir = TmpDir{"basic_workflow"};
    auto dbPath = tmpDir.path() / "codedb.db";
    auto ns = NodeStorageTestUtils::createEmptyNodeStorage(dbPath);

    auto projectFile = ProjectFileForTesting{};

    auto parent = ScopedPackageEntity(ns, "pkggrp");
    auto source = ScopedPackageEntity(ns, "pkgA", parent.value());
    auto componentA = ScopedComponentEntity(ns, "componentA", source.value());
    auto target = ScopedPackageEntity(ns, "pkgB", parent.value());
    auto componentB = ScopedComponentEntity(ns, "componentB", target.value());
    auto dep = ScopedPackageDependency(ns, source.value(), target.value());
    auto compDep = ScopedPackageDependency(ns, componentA.value(), componentB.value());

    //    auto window = InspectDependencyWindowForTesting{projectFile, dep.value()};
    //    window.show();
    //    REQUIRE(window.nRows() == 1);
    //    auto expectedFilepath = std::filesystem::path{};
    //    auto triggerCount = 0;
    //    window.onFileNotFound = [&triggerCount, &expectedFilepath](auto filepath) {
    //        REQUIRE(filepath.toStdString() == expectedFilepath.string());
    //        triggerCount += 1;
    //    };
}
