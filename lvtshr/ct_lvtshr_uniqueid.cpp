// ct_lvtshr_uniqueid.cpp                                    -*-C++-*-

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

#include <ct_lvtshr_uniqueid.h>

#include <functional>

namespace Codethink::lvtshr {

std::size_t UniqueId::Hash::operator()(UniqueId const& id) const noexcept
{
    std::size_t lhs = std::hash<decltype(id.m_diagramType)>{}(id.m_diagramType);
    std::size_t rhs = std::hash<decltype(id.m_recordNumber)>{}(id.m_recordNumber);
    // Based on the work from boost::hash_combine
    lhs ^= rhs + 0x9e3779b9 + (lhs << 6) + (lhs >> 2);
    return lhs;
}

UniqueId::UniqueId(DiagramType diagramType, long long recordNumber):
    m_diagramType(diagramType), m_recordNumber(recordNumber)
{
}

bool UniqueId::operator==(UniqueId const& other) const
{
    return this->m_diagramType == other.m_diagramType && this->m_recordNumber == other.m_recordNumber;
}

DiagramType UniqueId::diagramType() const
{
    return m_diagramType;
}

long long UniqueId::recordNumber() const
{
    return m_recordNumber;
}

std::ostream& operator<<(std::ostream& stream, const UniqueId& uid)
{
    stream << "{" << static_cast<int>(uid.m_diagramType) << ", " << uid.m_recordNumber << "}";
    return stream;
}

} // namespace Codethink::lvtshr
