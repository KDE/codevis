// ct_lvtqtc_repositoryentity.h                                        -*-C++-*-

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

#ifndef INCLUDED_CT_LVTGRPS_REPOSITORYENTITY
#define INCLUDED_CT_LVTGRPS_REPOSITORYENTITY

#include <lvtqtc_export.h>

#include <vector>

#include <QBrush>
#include <QPen>

#include <ct_lvtqtc_lakosentity.h>

namespace Codethink::lvtqtc {

class LakosEntity;

class LVTQTC_EXPORT RepositoryEntity : public LakosEntity {
  public:
    explicit RepositoryEntity(lvtldr::LakosianNode *node, lvtshr::LoaderInfo info);
    ~RepositoryEntity();
    void updateTooltip() final;
    static std::string getUniqueId(long long id);
    [[nodiscard]] lvtshr::DiagramType instanceType() const override;
    void updateBackground() override;
};

} // namespace Codethink::lvtqtc

#endif
