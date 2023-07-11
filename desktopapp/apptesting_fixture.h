// apptesting_fixture.h                                      -*-C++-*-

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
#ifndef DIAGRAM_SERVER_APPTESTING_FIXTURE_H
#define DIAGRAM_SERVER_APPTESTING_FIXTURE_H

#include <QObject>
#include <QToolButton>
#include <QtTest/QTest>
#include <algorithm>
#include <ct_lvtldr_nodestorage.h>
#include <ct_lvtqtc_graphicsview.h>
#include <ct_lvtqtc_lakosentity.h>
#include <ct_lvtqtc_tool_add_component.h>
#include <ct_lvtqtc_tool_add_logical_entity.h>
#include <ct_lvtqtc_tool_add_package.h>
#include <ct_lvtqtc_tool_add_physical_dependency.h>
#include <ct_lvtqtc_undo_manager.h>
#include <ct_lvtqtw_graphtabelement.h>
#include <ct_lvtqtw_toolbox.h>
#include <ct_lvttst_fixture_qt.h>
#include <mainwindow.h>
#include <testmainwindow.h>
#include <ui_ct_lvtqtw_graphtabelement.h>

#include <any>
#include <variant>

using namespace Codethink::lvtldr;
using namespace Codethink::lvtqtw;
using namespace Codethink::lvtqtc;

// clang-format off
namespace Sidebar { namespace ManipulationTool { struct NewPackage { QString name; }; } }
namespace Sidebar { namespace ManipulationTool { struct NewComponent { QString name; }; } }
namespace Sidebar { namespace ManipulationTool { struct NewLogicalEntity { QString name; QString kind; }; } }
namespace Sidebar { namespace ManipulationTool { struct AddDependency {}; } }
namespace Sidebar { namespace ManipulationTool { struct AddIsA {}; } }
namespace Sidebar { namespace VisualizationTool { struct ResetZoom {}; } }
namespace Sidebar { struct ToggleApplicationMode {}; }
namespace Menubar { namespace File { struct NewProject {}; } }
namespace Menubar { namespace File { struct CloseProject {}; } }
namespace PackageTreeView { struct Package {std::string name; }; }

struct CurrentGraph {
    CurrentGraph(int x, int y) : x(x), y(y) {}
    explicit CurrentGraph(QPoint const& p) : x(p.x()), y(p.y()) {}

    int x;
    int y;
};
// clang-format on

using ClickableFeature = std::variant<Sidebar::ManipulationTool::NewPackage,
                                      Sidebar::ManipulationTool::NewComponent,
                                      Sidebar::ManipulationTool::NewLogicalEntity,
                                      Sidebar::ManipulationTool::AddDependency,
                                      Sidebar::ManipulationTool::AddIsA,
                                      Sidebar::VisualizationTool::ResetZoom,
                                      Sidebar::ToggleApplicationMode,
                                      Menubar::File::NewProject,
                                      Menubar::File::CloseProject,
                                      PackageTreeView::Package,
                                      CurrentGraph>;

template<class... Ts>
struct visitor : Ts... {
    using Ts::operator()...;
};
template<class... Ts>
visitor(Ts...) -> visitor<Ts...>;

auto constexpr DEFAULT_QT_DELAY = 50; // ms

// Those are hardcoded indexes, look at graphtabelement.cpp
// for the positions of tools in the d->tools vector
auto constexpr TOOL_ADD_DEPENDENCY_ID = 1;
auto constexpr TOOL_ADD_PACKAGE_ID = 2;
auto constexpr TOOL_ADD_COMPONENT_ID = 3;
auto constexpr TOOL_ADD_LOGICAL_ENTITY_ID = 4;
auto constexpr TOOL_ADD_ISA_RELATIONSHIP_ID = 5;

class CodeVisApplicationTestFixture : public QTApplicationFixture {
  public:
    CodeVisApplicationTestFixture();
    [[nodiscard]] bool hasDefaultTabWidget() const;
    [[nodiscard]] bool isShowingWelcomePage() const;
    [[nodiscard]] bool isShowingGraphPage() const;
    void forceRelayout();
    void clickOn(ClickableFeature const& feature);
    QPoint findElementTopLeftPosition(const std::string& qname);
    LakosEntity *findElement(const std::string& qualifiedName);
    void ctrlZ();
    void ctrlShiftZ();
    bool isAnyToolSelected();
    TestMainWindow& window()
    {
        return mainWindow;
    };

  private:
    Codethink::lvtqtc::UndoManager undoManager;
    NodeStorage sharedNodeStorage;
    TestMainWindow mainWindow;
};

#endif // DIAGRAM_SERVER_APPTESTING_FIXTURE_H
