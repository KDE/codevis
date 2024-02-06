// ct_lvtmdl_usesintheimpltablemodel.h                                         -*-C++-*-

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

#ifndef DEFINED_CT_LVTMDL_USESINTHEINTHEIMPLTABLEMODEL
#define DEFINED_CT_LVTMDL_USESINTHEINTHEIMPLTABLEMODEL

#include <lvtmdl_export.h>

#include <ct_lvtmdl_basetablemodel.h>

#include <memory>

namespace Codethink::lvtmdl {

class LVTMDL_EXPORT UsesInTheImplTableModel : public BaseTableModel {
    Q_OBJECT
  public:
    UsesInTheImplTableModel();
    ~UsesInTheImplTableModel() override;
    void refreshData() override;

  private:
    struct Private;
    std::unique_ptr<Private> d;
};

} // end namespace Codethink::lvtmdl

#endif
