// ct_lvtqtc_componententity.h                                        -*-C++-*-

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

#ifndef INCLUDED_CT_LVTQTC_COMPONENTENTITY
#define INCLUDED_CT_LVTQTC_COMPONENTENTITY

#include <lvtqtc_export.h>

#include <ct_lvtqtc_lakosentity.h>

#include <string>

namespace Codethink {

namespace lvtqtc {

class LakosEntity;

/*! \class ComponentEntity
 *  \brief Represents and draws a Lakos Physical Entity
 *
 * %PackageEntity draws a Lakos Physical Entity, and handles tool tips
 * and mouse clicks
 */
class LVTQTC_EXPORT ComponentEntity : public LakosEntity {
  public:
    explicit ComponentEntity(lvtldr::LakosianNode *node, lvtshr::LoaderInfo info);

    void updateTooltip() final;
    void setText(const std::string& text) override;

    /*! \brief Returns a unique string identifying the SourceComponent instance
     */
    static std::string getUniqueId(long long id);

    [[nodiscard]] lvtshr::DiagramType instanceType() const override;
};

} // end namespace lvtqtc
} // end namespace Codethink

#endif
