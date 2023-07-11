// ct_lvtqtw_tool_zoom.h                                            -*-C++-*-

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

#ifndef INCLUDED_LVTQTC_TOOL_ZOOM
#define INCLUDED_LVTQTC_TOOL_ZOOM

#include <lvtqtc_export.h>

#include <ct_lvtqtc_tool_rubberband.h>

#include <QGraphicsView>

#include <memory>

namespace Codethink::lvtqtc {

class LVTQTC_EXPORT ToolZoom final : public ToolRubberband {
    // Controls the Drag Zoom / Rubberband Zoom.

    Q_OBJECT
  public:
    ToolZoom(GraphicsView *gv);
    ~ToolZoom() noexcept override;

    void rubberbandChanged(const QPoint& topLeft, const QPoint& bottomRight) override;
    void rubberbandFinished(const QPoint& topLeft, const QPoint& bottomRight) override;
};

} // namespace Codethink::lvtqtc

#endif
