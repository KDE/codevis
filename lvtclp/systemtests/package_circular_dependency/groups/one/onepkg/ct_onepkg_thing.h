// ct_onepkg_thing.h                                                  -*-C++-*-

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

#ifndef INCLUDED_CT_ONEPKG_THING
#define INCLUDED_CT_ONEPKG_THING

//@PURPOSE: Demonstrate circular dependency detection in lvt
//
//@CLASSES:
//  Codethink::onepkg::Thing

namespace Codethink::onepkg {

                               // =============================
                               // class Thing
                               // =============================

class Thing {
  private:
    // PRIAVATE DATA
    const char *d_name_p;

  public:
    // CREATORS
    Thing(const char *name);

    // ACCESSORS
    const char *getName() const;
};

} // end namespace Codethink::onepkg

#endif // !INCLUDED_CT_ONEPKG_THING
