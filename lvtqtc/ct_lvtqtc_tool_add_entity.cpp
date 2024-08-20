// ct_lvtqtw_tool_add_entity.cpp                                            -*-C++-*-

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

#include <ct_lvtqtc_tool_add_entity.h>

#include <ct_lvtqtc_graphicsscene.h>
#include <ct_lvtqtc_graphicsview.h>
#include <ct_lvtqtc_inputdialog.h>
#include <ct_lvtqtc_lakosentity.h>
#include <ct_lvtqtc_packageentity.h>

namespace Codethink::lvtqtc {

BaseAddEntityTool::BaseAddEntityTool(const QString& name, const QString& tooltip, const QIcon& icon, GraphicsView *gv):
    ITool(name, tooltip, icon, gv), m_nameDialog(new InputDialog(name, gv))
{
}

BaseAddEntityTool::~BaseAddEntityTool() = default;

InputDialog *BaseAddEntityTool::inputDialog()
{
    return m_nameDialog;
}

} // namespace Codethink::lvtqtc

#include "moc_ct_lvtqtc_tool_add_entity.cpp"
