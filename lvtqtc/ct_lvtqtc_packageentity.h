// ct_lvtqtc_packageentity.h                                        -*-C++-*-

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

#ifndef INCLUDED_CT_LVTGRPS_PACKAGEENTITY
#define INCLUDED_CT_LVTGRPS_PACKAGEENTITY

#include <lvtqtc_export.h>

#include <vector>

#include <QBrush>
#include <QPen>

#include <ct_lvtqtc_lakosentity.h>

namespace Codethink {

namespace lvtqtc {

class LakosEntity;

/*! \class PackageEntity physical_entity.cpp physical_entity.h
 *  \brief Represents and draws a Lakos Physical Entity
 *
 * %PackageEntity draws a Lakos Physical Entity, and handles tool tips
 * and mouse clicks
 */
class LVTQTC_EXPORT PackageEntity : public LakosEntity {
  public:
    explicit PackageEntity(lvtldr::LakosianNode *node, lvtshr::LoaderInfo info);

    void updateTooltip() final;

    /*! \brief Returns a unique string identifying the SourcePackage instance
     */
    static std::string getUniqueId(long long id);

    [[nodiscard]] lvtshr::DiagramType instanceType() const override;
};

} // end namespace lvtqtc
} // end namespace Codethink

#endif
