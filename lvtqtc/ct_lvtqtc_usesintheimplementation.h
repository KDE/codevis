// ct_lvtqtc_usesintheimplementation.h                              -*-C++-*-

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

#ifndef INCLUDED_CT_LVTGRPS_USESINTHEIMPLEMENTATION
#define INCLUDED_CT_LVTGRPS_USESINTHEIMPLEMENTATION

#include <lvtqtc_export.h>

#include <QPainter>

#include <ct_lvtqtc_lakosrelation.h>

namespace Codethink::lvtqtc {

/*! \class UsesInTheImplementation uses_in_the_implementation.cpp uses_in_the_implementation.h
 *  \brief Represents and draws a UsesInTheImplementation LakosRelation
 *
 * %UsesInTheImplementation draws a Uses-In-The-Implementation LakosRelation
 */
class LVTQTC_EXPORT UsesInTheImplementation : public LakosRelation {
  public:
    /*! \brief Constructs an UsesInTheImplementation relation
     *
     * The source vertex is where the circle goes
     */
    UsesInTheImplementation(LakosEntity *source, LakosEntity *target);

    [[nodiscard]] lvtshr::LakosRelationType relationType() const override;
    [[nodiscard]] std::string relationTypeAsString() const override;
};

} // end namespace Codethink::lvtqtc

#endif
