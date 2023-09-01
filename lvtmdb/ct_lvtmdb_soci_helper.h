// ct_lvtmdb_soci_helper.h                                         -*-C++-*-

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

#ifndef INCLUDED_CT_LVTMDB_SOCI_HELPER
#define INCLUDED_CT_LVTMDB_SOCI_HELPER

#include <any>
#include <soci/soci.h>
#include <soci/sqlite3/soci-sqlite3.h>
#include <string>
#include <vector>

namespace Codethink::lvtmdb {

using RawDBData = std::tuple<std::any, bool>;
using RawDBCols = std::vector<RawDBData>;
using RawDBRows = std::vector<RawDBCols>;

namespace detail {

template<typename T>
static RawDBData _getDBData(soci::row& row, size_t pos)
{
    if (row.get_indicator(pos) != soci::i_null) {
        return RawDBData{row.get<T>(pos), false};
    }
    return RawDBData{T{}, true};
}

static RawDBData getDBData(soci::row& row, size_t pos)
{
    auto const& props = row.get_properties(pos);
    switch (props.get_data_type()) {
    case soci::dt_string:
        return _getDBData<std::string>(row, pos);
    case soci::dt_double:
        return _getDBData<double>(row, pos);
    case soci::dt_integer:
        return _getDBData<int>(row, pos);
    case soci::dt_long_long:
        return _getDBData<long long>(row, pos);
    case soci::dt_unsigned_long_long:
        return _getDBData<unsigned long long>(row, pos);
    case soci::dt_date:
        /* Ignored */
        break;
    case soci::dt_blob:
        /* Ignored */
        break;
    case soci::dt_xml:
        /* Ignored */
        break;
    }

    throw std::runtime_error{"Unexpected data type"};
};

} // namespace detail

struct SociHelper {
    //  Small helper methods and enums to be used from soci writer and
    // soci reader.

    enum class Key : int {
        Invalid = 0,
        DatabaseState = 1,
        Version = 2,
    };

    enum class Version : int {
        Unknown = 0,
        Jan22 = 1,
        March22 = 2,
        March23 = 3,
    };

    static constexpr int CURRENT_VERSION = static_cast<int>(Version::March23);

    static RawDBRows runSingleQuery(soci::session& db, std::string const& query)
    {
        soci::transaction tr(db);
        auto rowset = soci::rowset<soci::row>{db.prepare << query};
        tr.commit();

        // It is not possible to return the `rowset` directly since it'll lose internal references and most probably
        // crash. Currently, the approach is to copy all results, but this _may_ be a performance issue in some
        // contexts, but it seems the most user-friendly approach. One alternative would be to wrap the necessary
        // objects in a struct and yield rows either using an iterator or a coroutine (C++20). This may be done in the
        // future, if anyone ever has a plugin that is struggling due to those copies.
        auto resultRows = RawDBRows{};
        for (auto&& rowdata : rowset) {
            auto cols = RawDBCols{};
            for (decltype(rowdata.size()) i = 0; i < rowdata.size(); ++i) {
                cols.emplace_back(detail::getDBData(rowdata, i));
            }
            resultRows.emplace_back(cols);
        }
        return resultRows;
    }
};

} // namespace Codethink::lvtmdb
#endif
