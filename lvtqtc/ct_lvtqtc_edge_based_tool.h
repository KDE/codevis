// ct_lvtqtc_edge_based_tool.h                                                                                 -*-C++-*-

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

#ifndef INCLUDED_LVTQTC_EDGE_BASED_TOOL
#define INCLUDED_LVTQTC_EDGE_BASED_TOOL

#include <ct_lvtqtc_itool.h>
#include <lvtqtc_export.h>
#include <memory>

namespace Codethink::lvtqtc {
class LakosEntity;
}

namespace Codethink::lvtqtc {

class LVTQTC_EXPORT EdgeBasedTool : public ITool {
    Q_OBJECT
  public:
    ~EdgeBasedTool() override;

    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void activate() override;
    void deactivate() override;
    static std::vector<std::pair<LakosEntity *, LakosEntity *>> calculateHierarchy(LakosEntity *source,
                                                                                   LakosEntity *target);

  protected:
    EdgeBasedTool(const QString& name, const QString& tooltip, const QIcon& icon, GraphicsView *gv);
    virtual bool run(LakosEntity *source, LakosEntity *target) = 0;

  private:
    struct Private;
    std::unique_ptr<Private> d;
};

} // namespace Codethink::lvtqtc

#endif
