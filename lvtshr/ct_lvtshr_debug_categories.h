// ct_lvtshr_debug_categories.h                                         -*-C++-*-

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

#include <ct_lvtshr_stringhelpers.h>
#include <lvtshr_export.h>

namespace Codethink::lvtshr {

enum class LoggingCategory {
    Parsing = 0,
    Graphics = 1,
    DebugModel = 2,
    // Should be removed after every uncategorised qDebugs are sorted into a category.
    Uncategorised = 9999,
    _End
};

class LVTSHR_EXPORT CategoryManager {
  public:
    static CategoryManager& instance();
    void add(LoggingCategory cat, const QLoggingCategory *);
    const QLoggingCategory *getCategory(LoggingCategory cat);
    QList<const QLoggingCategory *> getCategories();

  private:
    CategoryManager();
    QMap<LoggingCategory, const QLoggingCategory *> categories;
};

} // namespace Codethink::lvtshr

// Use this only on .cpp - not on .h. I'm not sure if the best approach is
// a stringfied name or an enum, but this works on small tests.'
#define CODEVIS_LOGGING_CATEGORIES(category, name, loggingCategory)                                                    \
    Q_LOGGING_CATEGORY(category, name);                                                                                \
    namespace {                                                                                                        \
    auto __val__##category = []() -> bool {                                                                            \
        Codethink::lvtshr::CategoryManager::instance().add(loggingCategory, &category());                              \
        return true;                                                                                                   \
    }();                                                                                                               \
    }

#endif