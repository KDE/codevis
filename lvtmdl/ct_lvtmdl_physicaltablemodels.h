// ct_lvtmdl_physicaltablemodels.h                                     -*-C++-*-

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

#ifndef INCLUDED_CT_LVTMDL_PHYSICALTABLEMODELS
#define INCLUDED_CT_LVTMDL_PHYSICALTABLEMODELS

#include <lvtmdl_export.h>

#include <ct_lvtmdl_basetablemodel.h>

#include <memory>
#include <string>
#include <vector>

namespace Codethink::lvtmdl {

// ================================
// class PhysicalDepsBaseTableModel
// ================================

class LVTMDL_EXPORT PhysicalDepsBaseTableModel : public BaseTableModel {
    // Shares code between models for forward and reverse
    // physical dependencies

    Q_OBJECT
  public:
    PhysicalDepsBaseTableModel();
    void refreshData() override;

  protected:
    virtual std::vector<std::string> componentRows() = 0;
    // Return rows for this table for type() == ComponentType

    virtual std::vector<std::string> packageRows() = 0;
    // Return rows for this table for type() == PackageType
};

// ===============================
// class PhysicalProvidersTableModel
// ===============================

class LVTMDL_EXPORT PhysicalProvidersTableModel : public PhysicalDepsBaseTableModel {
    // Concrete implementation of PhysicalDepsBaseTableModel for forward
    // dependencies

    Q_OBJECT
  protected:
    std::vector<std::string> componentRows() override;
    std::vector<std::string> packageRows() override;
};

// ===============================
// class PhysicalClientsTableModel
// ===============================

class LVTMDL_EXPORT PhysicalClientsTableModel : public PhysicalDepsBaseTableModel {
    // Concrete implementation of PhysicalDepsBaseTableModel for reverse
    // dependencies

    Q_OBJECT
  protected:
    std::vector<std::string> componentRows() override;
    std::vector<std::string> packageRows() override;
};

} // namespace Codethink::lvtmdl

#endif // INCLUDED_CT_LVTMDL_PHYSICALTABLEMODELS
