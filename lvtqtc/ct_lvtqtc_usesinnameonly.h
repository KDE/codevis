// ct_lvtqtc_usesinnameonly.h                                       -*-C++-*-

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

#ifndef INCLUDED_CT_LVTGRPS_USESNAMEONLY
#define INCLUDED_CT_LVTGRPS_USESNAMEONLY

#include <lvtqtc_export.h>

#include <QPainter>

#include <ct_lvtqtc_lakosrelation.h>

namespace Codethink::lvtqtc {

/*! \class UsesInNameOnly uses_in_name_only.cpp uses_in_name_only.h
 *  \brief Represents and draws a UsesInNameOnly LakosRelation
 *
 * %UsesInNameOnly draws a Use-In-Name-Only LakosRelation
 */
class LVTQTC_EXPORT UsesInNameOnly : public LakosRelation {
  public:
    /*! \brief Constructs an UsesInNameOnly relation
     *
     * The source vertex is where the circle goes
     */
    UsesInNameOnly(LakosEntity *source, LakosEntity *target);

    /*! \brief Draws the relation
     */
    // This is not called internally, looks wrong.
    // void drawWithRadius(QPainter &painter, double radius);

    /*! \brief The relationType of a particular instance of sub class LakosRelation
     */
    [[nodiscard]] lvtshr::LakosRelationType relationType() const override;
};

} // end namespace Codethink::lvtqtc

#endif
