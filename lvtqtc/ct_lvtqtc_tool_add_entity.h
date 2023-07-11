// ct_lvtqtw_tool_add_entity.h                                            -*-C++-*-

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

#ifndef INCLUDED_LVTQTC_TOOL_ADD_ENTITY
#define INCLUDED_LVTQTC_TOOL_ADD_ENTITY

#include <ct_lvtqtc_inputdialog.h>
#include <ct_lvtqtc_itool.h>
#include <lvtqtc_export.h>

#include <QGraphicsView>

#include <memory>
#include <result/result.hpp>

namespace Codethink::lvtqtc {
class LakosEntity;

class LVTQTC_EXPORT BaseAddEntityTool : public ITool {
    // Creates a logical entity, for instance, a class or a struct.

    Q_OBJECT
  public:
    explicit BaseAddEntityTool(const QString& name, const QString& tooltip, const QIcon& icon, GraphicsView *gv);
    ~BaseAddEntityTool() override;

    InputDialog *inputDialog();
    // overrides the internal dialog used to fetch the name

  protected:
    InputDialog *m_nameDialog = nullptr;
};

} // namespace Codethink::lvtqtc
#endif
