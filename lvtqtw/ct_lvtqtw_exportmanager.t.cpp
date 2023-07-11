// ct_lvtqtw_exportmanager.t.cpp                           -*-C++-*-

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

#include <ct_lvtqtc_graphicsview.h>
#include <ct_lvtqtc_testing_utils.h>

#include <ct_lvtclr_colormanagement.h>
#include <ct_lvtldr_nodestorage.h>
#include <ct_lvtqtc_lakosrelation.h>
#include <ct_lvtqtw_exportmanager.h>
#include <ct_lvttst_fixture_qt.h>

#include <catch2/catch.hpp>

#include <QFileInfo>
#include <QTemporaryDir>

using namespace Codethink::lvtqtc;
using namespace Codethink::lvtqtw;
using namespace Codethink::lvtclr;
using namespace Codethink::lvtshr;
using namespace Codethink::lvtprj;

TEST_CASE_METHOD(QTApplicationFixture, "Export graphicsview contents to image files")
{
    Codethink::lvtldr::NodeStorage storage;
    auto projectFile = ProjectFileForTesting{};
    auto *view = new Codethink::lvtqtc::GraphicsView(storage, projectFile);

    view->show();
    QApplication::processEvents();

    Codethink::lvtqtw::ExportManager manager(view);

    QTemporaryDir dir;
    QString filename = dir.path() + "/file.svg";

    auto ret = manager.exportSvg(filename);
    REQUIRE(ret.has_value());

    QFileInfo info(filename);
    REQUIRE(info.exists());
}
