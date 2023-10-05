// ct_lvtqtc_packagedependency.cpp                                  -*-C++-*-

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

#include <ct_lvtldr_packagenode.h>
#include <ct_lvtqtc_lakosentity.h>
#include <ct_lvtqtc_packagedependency.h>

#include <QPainterPath>
#include <preferences.h>

namespace Codethink::lvtqtc {

PackageDependency::PackageDependency(LakosEntity *source, LakosEntity *target): LakosRelation(source, target)
{
    setHead(LakosRelation::defaultArrow());
    setThickness(3);
    setDashed(false);
}

lvtshr::LakosRelationType PackageDependency::relationType() const
{
    return lvtshr::LakosRelationType::PackageDependency;
}

std::string PackageDependency::relationTypeAsString() const
{
    return "Package Dependency";
}

QColor PackageDependency::hoverColor() const
{
    static const auto HOVER_COLOR = QColor{10, 10, 200};
    return HOVER_COLOR;
}

} // end namespace Codethink::lvtqtc
