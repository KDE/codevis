// ct_lvtshr_stringhelpers.cpp                                   -*-C++-*-

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

#include <ct_lvtshr_stringhelpers.h>

namespace Codethink::lvtshr {

bool StrUtil::beginsWith(const std::string& str, const std::string& beginStr)
{
    return str.find(beginStr, 0) == 0;
}

} // namespace Codethink::lvtshr

QDebug operator<<(QDebug dbg, const std::string& str)
{
    dbg << QString::fromStdString(str);
    return dbg;
}

std::string to_string(const QVariant& v)
{
    return v.toString().toStdString();
}
