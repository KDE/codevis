// ct_twodmo_top.cpp                                                  -*-C++-*-

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

#include <ct_twodmo_top.h>

namespace Codethink::twodmo {

                               // =============================
                               // struct TopUtil
                               // =============================

void TopUtil::doSomething()
{
    onepkg::Circle c(1.0);
    double r2;
    if (c.getName()[0] == 42) {
        r2 = 2.0;
    } else {
        r2 = 3.0;
    }
    onepkg::Circle c2(r2);
}

} // end namespace Codethink::onepkg
