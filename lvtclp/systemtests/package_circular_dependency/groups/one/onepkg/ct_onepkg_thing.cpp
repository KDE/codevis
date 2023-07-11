// ct_onepkg_thing.cpp                                                -*-C++-*-

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

#include <ct_onepkg_thing.h>

namespace Codethink::onepkg {

                               // =============================
                               // class Thing
                               // =============================

Thing::Thing(const char *name):
    d_name_p(name)
{
}

const char *Thing::getName() const
{
    return d_name_p;
}

} // end namespace Codethink::onepkg
