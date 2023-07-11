// ct_lvtqtwc_tool_add_physical_dependency.h                                            -*-C++-*-

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

#ifndef INCLUDED_LVTQTC_TOOL_ADD_RELATIONSHIP
#define INCLUDED_LVTQTC_TOOL_ADD_RELATIONSHIP

#include <ct_lvtqtc_edge_based_tool.h>

#include <QGraphicsView>

#include <memory>

namespace Codethink::lvtldr {
class NodeStorage;
}

namespace Codethink::lvtqtc {

class LVTQTC_EXPORT ToolAddPhysicalDependency : public EdgeBasedTool {
    Q_OBJECT
  public:
    ToolAddPhysicalDependency(GraphicsView *gv, Codethink::lvtldr::NodeStorage& nodeStorage);
    ~ToolAddPhysicalDependency() override;

  protected:
    bool run(LakosEntity *fromItem, LakosEntity *toItem) override;

  private:
    struct Private;
    std::unique_ptr<Private> d;
};

} // namespace Codethink::lvtqtc
#endif
