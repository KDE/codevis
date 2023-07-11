// ct_lvtqtw_tool_reparent_entity.h                                                                            -*-C++-*-

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

#ifndef INCLUDED_LVTQTC_TOOL_REPARENT_ENTITY
#define INCLUDED_LVTQTC_TOOL_REPARENT_ENTITY

#include <ct_lvtqtc_tool_add_entity.h>
#include <lvtqtc_export.h>

#include <QGraphicsView>

#include <memory>

namespace Codethink::lvtqtc {

class LVTQTC_EXPORT ToolReparentEntity : public ITool {
    Q_OBJECT
  public:
    ToolReparentEntity(GraphicsView *gv, Codethink::lvtldr::NodeStorage& nodeStorage);
    ~ToolReparentEntity() override;

    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void deactivate() override;

  private:
    void updateCurrentItemTo(LakosEntity *newItem);
    void updateTargetItemTo(LakosEntity *newItem);

    struct Private;
    std::unique_ptr<Private> d;
};

} // namespace Codethink::lvtqtc

#endif
