// ct_lvtqtw_tool_add_logical_entity.h                                            -*-C++-*-

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

#ifndef INCLUDED_LVTQTC_TOOL_ADD_LOGICAL_ENTITY
#define INCLUDED_LVTQTC_TOOL_ADD_LOGICAL_ENTITY

#include <ct_lvtqtc_tool_add_entity.h>
#include <lvtqtc_export.h>

#include <QGraphicsView>

#include <memory>
#include <result/result.hpp>

class CodeVisApplicationTestFixture;
namespace Codethink::lvtldr {
class NodeStorage;
}

namespace Codethink::lvtqtc {
class ComponentEntity;

struct InvalidLogicalEntityError {
    QString what;
};

/* Trait implementation for error checking
 * cannot be used directly as there's no implementation of checkName.
 */
struct AddLogicalEntityVerifyRulesTrait {
    static cpp::result<void, InvalidLogicalEntityError>
    checkName(bool hasParent, const std::string& parentName, const std::string& name);
};

/* Error checking for the Lakosian Name Rules */
struct LVTQTC_EXPORT LogicalEntityNameRules { // implements AddPackageVerifyRulesTrait
    static cpp::result<void, InvalidLogicalEntityError>
    checkName(bool hasParent, const std::string& name, const std::string& parentName);
};

class LVTQTC_EXPORT ToolAddLogicalEntity : public BaseAddEntityTool {
    // Creates a logical entity, for instance, a class or a struct.

    Q_OBJECT
  public:
    friend class ::CodeVisApplicationTestFixture;

    ToolAddLogicalEntity(GraphicsView *gv, Codethink::lvtldr::NodeStorage& nodeStorage);
    ~ToolAddLogicalEntity() override;

    void activate() override;
    void mousePressEvent(QMouseEvent *event) override;

  private:
    struct Private;
    std::unique_ptr<Private> d;
};

} // namespace Codethink::lvtqtc
#endif
