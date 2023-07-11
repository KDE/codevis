// ct_lvtqtwc_tool_add_logical_relation.h                                                                      -*-C++-*-

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

#ifndef INCLUDED_LVTQTC_TOOL_ADD_LOGICAL_RELATION
#define INCLUDED_LVTQTC_TOOL_ADD_LOGICAL_RELATION

#include <ct_lvtqtc_edge_based_tool.h>
#include <ct_lvtshr_graphenums.h>

#include <QGraphicsView>

#include <memory>

namespace Codethink::lvtldr {
class NodeStorage;
}

namespace Codethink::lvtqtc {

class LVTQTC_EXPORT ToolAddLogicalRelation : public EdgeBasedTool {
    Q_OBJECT
  protected:
    ToolAddLogicalRelation(const QString& name,
                           const QString& tooltip,
                           const QIcon& icon,
                           GraphicsView *gv,
                           Codethink::lvtldr::NodeStorage& nodeStorage);
    ~ToolAddLogicalRelation() override;

    bool doRun(LakosEntity *fromItem, LakosEntity *toItem, lvtshr::LakosRelationType type);

  private:
    struct Private;
    std::unique_ptr<Private> d;
};

class LVTQTC_EXPORT ToolAddIsARelation : public ToolAddLogicalRelation {
    Q_OBJECT
  public:
    ToolAddIsARelation(GraphicsView *gv, Codethink::lvtldr::NodeStorage& nodeStorage);

  protected:
    bool run(LakosEntity *source, LakosEntity *target) override;
};

class LVTQTC_EXPORT ToolAddUsesInTheImplementation : public ToolAddLogicalRelation {
    Q_OBJECT
  public:
    ToolAddUsesInTheImplementation(GraphicsView *gv, Codethink::lvtldr::NodeStorage& nodeStorage);

  protected:
    bool run(LakosEntity *source, LakosEntity *target) override;
};

class LVTQTC_EXPORT ToolAddUsesInTheInterface : public ToolAddLogicalRelation {
    Q_OBJECT
  public:
    ToolAddUsesInTheInterface(GraphicsView *gv, Codethink::lvtldr::NodeStorage& nodeStorage);

  protected:
    bool run(LakosEntity *source, LakosEntity *target) override;
};

} // namespace Codethink::lvtqtc
#endif
