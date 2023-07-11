// ct_lvtshr_stringhelpers.h                                    -*-C++-*-

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

#ifndef INCLUDED_DIAGRAM_SERVER_CT_LVTSHR_STRINGHELPERS_H
#define INCLUDED_DIAGRAM_SERVER_CT_LVTSHR_STRINGHELPERS_H

#include <lvtshr_export.h>
#include <string>

#include <QDebug>
#include <QString>
#include <QVariant>

namespace Codethink::lvtshr {

struct LVTSHR_EXPORT StrUtil {
    static bool beginsWith(const std::string& str, const std::string& beginStr);
};

} // namespace Codethink::lvtshr

LVTSHR_EXPORT QDebug operator<<(QDebug dbg, const std::string& str);
LVTSHR_EXPORT std::string to_string(const QVariant& v);

#endif // INCLUDED_DIAGRAM_SERVER_CT_LVTSHR_STRINGHELPERS_H
