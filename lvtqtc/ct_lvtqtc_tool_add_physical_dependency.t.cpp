// ct_lvtqtc_tool_add_physical_dependency.t.cpp                               -*-C++-*-

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

#include <ct_lvtclr_colormanagement.h>
#include <ct_lvtldr_lakosiannode.h>
#include <ct_lvtldr_nodestoragetestutils.h>
#include <ct_lvtqtc_testing_utils.h>
#include <ct_lvtqtc_tool_add_physical_dependency.h>
#include <ct_lvttst_fixture_qt.h>
#include <ct_lvttst_tmpdir.h>

#include <memory>

using namespace Codethink::lvtqtc;
using namespace Codethink::lvtldr;
using namespace Codethink::lvtclr;
using namespace Codethink::lvtshr;

TEST_CASE_METHOD(QTApplicationFixture, "Add dependency")
{
    auto tmpDir = TmpDir{"add_dependency_test"};
    auto dbPath = tmpDir.path() / "codedb.db";
    auto nodeStorage = NodeStorageTestUtils::createEmptyNodeStorage(dbPath);

    auto expectAddComponent = [&nodeStorage](const std::string& name, const std::string& qualifiedName, auto *parent) {
        auto result = nodeStorage.addComponent(name, qualifiedName, parent);
        REQUIRE_FALSE(result.has_error());
        return result.value();
    };

    auto *a = nodeStorage.addPackage("a", "a").value();
    auto *aa = nodeStorage.addPackage("aa", "aa", a).value();
    auto *aaa = expectAddComponent("aaa", "aaa", aa);
    auto *aab = expectAddComponent("aab", "aab", aa);

    GraphicsViewWrapperForTesting gv{nodeStorage};
    gv.updatePackageGraph(QString::fromStdString(aa->qualifiedName()));
    gv.show();

    auto tool = ToolAddPhysicalDependency{&gv, nodeStorage};
    std::string lastMessage;
    QObject::connect(&tool,
                     &ToolAddPhysicalDependency::sendMessage,
                     &tool,
                     [&lastMessage](const QString& s, KMessageWidget::MessageType) {
                         qDebug() << "Message Received" << s;
                         lastMessage = s.toStdString();
                     });

    // Using the tool ar arbitrary place won't do anything (including no crashing)
    REQUIRE_FALSE(mousePressAt(tool, {0, 0}));
    REQUIRE(lastMessage.empty());

    // Basic tool usage
    tool.activate();
    REQUIRE_FALSE(aaa->hasProvider(aab));
    mousePressAt(tool, gv.getEntityPosition(aaa->uid()));
    mouseReleaseAt(tool, gv.getEntityPosition(aaa->uid()));
    REQUIRE(lastMessage == "Source element: <strong>aaa</strong>, Select the target Element");

    mousePressAt(tool, gv.getEntityPosition(aab->uid()));
    mouseReleaseAt(tool, gv.getEntityPosition(aab->uid()));
    tool.deactivate();
    REQUIRE(lastMessage.empty());

    tool.activate();
    mousePressAt(tool, gv.getEntityPosition(aaa->uid()));
    mouseReleaseAt(tool, gv.getEntityPosition(aaa->uid()));
    mousePressAt(tool, gv.getEntityPosition(aab->uid()));
    mouseReleaseAt(tool, gv.getEntityPosition(aab->uid()));
    tool.deactivate();
    REQUIRE(lastMessage == "You can't connect aaa to aab, they already have a connection.");
    REQUIRE(aaa->hasProvider(aab));

    tool.activate();
    mousePressAt(tool, gv.getEntityPosition(aaa->uid()));
    mouseReleaseAt(tool, gv.getEntityPosition(aaa->uid()));
    mousePressAt(tool, gv.getEntityPosition(aab->uid()));
    mouseReleaseAt(tool, gv.getEntityPosition(aab->uid()));
    tool.deactivate();
    REQUIRE(lastMessage == "You can't connect aaa to aab, they already have a connection.");

    tool.activate();
    mousePressAt(tool, gv.getEntityPosition(aaa->uid()));
    mouseReleaseAt(tool, gv.getEntityPosition(aaa->uid()));
    mousePressAt(tool, gv.getEntityPosition(aaa->uid()));
    mouseReleaseAt(tool, gv.getEntityPosition(aaa->uid()));
    REQUIRE(lastMessage == "Cannot create self-dependency");
    tool.deactivate();
}
