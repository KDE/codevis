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

#include <ct_lvtclp_threadstringmap.h>
#include <filesystem>

using namespace Codethink::lvtclp;

template<class Map, class Key, class Func>
typename Map::mapped_type& get_else_compute(Map& map, Key const& key, Func func)
{
    typedef typename Map::mapped_type V;
    std::pair<typename Map::iterator, bool> r = map.insert(typename Map::value_type(key, {}));
    V& v = r.first->second;
    if (r.second) {
        func(v);
    }
    return v;
}

std::string ThreadStringMap::get_or_add(const std::string& key)
{
    std::lock_guard<std::mutex> guard(d_map_mutex);

    return get_else_compute(d_map, key, [&key](std::string& path) {
        path = std::filesystem::weakly_canonical(key).generic_string();
    });
}
