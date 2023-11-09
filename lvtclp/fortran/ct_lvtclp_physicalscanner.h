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

#ifndef CODEVIS_CT_LVTCLP_PHYSICALSCANNER_H
#define CODEVIS_CT_LVTCLP_PHYSICALSCANNER_H

#include <ct_lvtmdb_objectstore.h>
#include <flang/Frontend/FrontendActions.h>
#include <lvtclp_export.h>
#include <string>
#include <tuple>
#include <unordered_set>

namespace Codethink::lvtclp::fortran {

class LVTCLP_EXPORT PhysicalParseAction : public Fortran::frontend::PrescanAction {
  public:
    PhysicalParseAction(lvtmdb::ObjectStore& memDb);

  private:
    void executeAction() override;

    lvtmdb::ObjectStore& memDb;
};

} // namespace Codethink::lvtclp::fortran

#endif // CODEVIS_CT_LVTCLP_PHYSICALSCANNER_H
