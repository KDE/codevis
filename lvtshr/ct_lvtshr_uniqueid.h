// ct_lvtshr_uniqueid.h                                    -*-C++-*-

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

#ifndef INCLUDED_CT_LVTSHR_UNIQUEID
#define INCLUDED_CT_LVTSHR_UNIQUEID

#include <lvtshr_export.h>

#include <ct_lvtshr_graphenums.h>

#include <cstddef>
#include <ostream>

namespace Codethink::lvtshr {

class LVTSHR_EXPORT UniqueId {
  public:
    using RecordNumberType = long long;

    struct LVTSHR_EXPORT Hash {
        std::size_t operator()(UniqueId const& id) const noexcept;
    };

    UniqueId(DiagramType type, RecordNumberType recordNumber);
    bool operator==(UniqueId const& other) const;
    [[nodiscard]] DiagramType diagramType() const;
    [[nodiscard]] RecordNumberType recordNumber() const;

    LVTSHR_EXPORT friend std::ostream& operator<<(std::ostream& stream, const UniqueId& uid);

  private:
    DiagramType m_diagramType;
    RecordNumberType m_recordNumber;
};

} // namespace Codethink::lvtshr

#endif
