// ct_lvtshr_functional.h                                         -*-C++-*-

/*
 * // Copyright 2023 Codethink Ltd <codethink@codethink.co.uk>
 * // SPDX-License-Identifier: Apache-2.0
 * //
 * // Licensed under the Apache License, Version 2.0 (the "License");
 * // you may not use this file except in compliance with the License.
 * // You may obtain a copy of the License at
 * //
 * //     http://www.apache.org/licenses/LICENSE-2.0
 * //
 * // Unless required by applicable law or agreed to in writing, software
 * // distributed under the License is distributed on an "AS IS" BASIS,
 * // WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * // See the License for the specific language governing permissions and
 * // limitations under the License.
 */
#ifndef INCLUDED_CT_DEBUG_CATEGORIES_H
#define INCLUDED_CT_DEBUG_CATEGORIES_H

#include <QLoggingCategory>
#include <map>

#include <lvtshr_export.h>

namespace Codethink::lvtshr {

enum class LoggingCategory { Parsing = 0, BackgroundGraphics = 1, TreeView = 2, Graphics = 3, Interface = 4, _End };

LVTSHR_EXPORT const std::map<LoggingCategory, std::unique_ptr<QLoggingCategory>>& logCategories();
LVTSHR_EXPORT const QLoggingCategory& logCategory(LoggingCategory const& catEnum);

} // namespace Codethink::lvtshr

#endif
