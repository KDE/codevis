// ct_onepkg_circle.cpp                                               -*-C++-*-

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

#include <ct_onepkg_circle.h>

#include <ct_twodmo_top.h> // BUG

namespace Codethink::onepkg {

                               // =============================
                               // class Circle
                               // =============================

Circle::Circle(double radius):
    Thing("Circle"),
    d_radius(radius)
{
}

double Circle::getRadius() const
{
    return d_radius;
}

} // end namespace Codethink::onepkg
