// ct_lvtqtc_logicalentity.h                                        -*-C++-*-

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

#ifndef INCLUDED_CT_LVTGRPS_LOGICAL
#define INCLUDED_CT_LVTGRPS_LOGICAL

#include <lvtqtc_export.h>

#include <ct_lvtqtc_lakosentity.h>
#include <memory>

namespace Codethink {

namespace lvtqtc {

/*! \class LogicalEntity logical_entity.cpp logical_entity.h
 *  \brief Represents and draws a Lakos Logical Entity
 *
 * %LogicalEntity draws a Lakos Logical Entity, and handles tool tips
 * and mouse clicks
 */
class LVTQTC_EXPORT LogicalEntity : public LakosEntity {
  public:
    explicit LogicalEntity(lvtldr::LakosianNode *node, lvtshr::LoaderInfo info);
    ~LogicalEntity() override;

    void updateTooltip() final;

    /*! \brief returns a unique string identifying the UserDefinedType instance
     */
    static std::string getUniqueId(long long id);

    [[nodiscard]] std::string colorIdText() const override;

    [[nodiscard]] lvtshr::DiagramType instanceType() const override;

  private:
    struct Private;
    std::unique_ptr<Private> d;
};

} // end namespace lvtqtc
} // end namespace Codethink

#endif
