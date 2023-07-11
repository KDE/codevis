// ct_lvtshr_iterator.h                                                                                        -*-C++-*-

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

#ifndef DIAGRAM_SERVER_CT_LVTSHR_ITERATOR_H
#define DIAGRAM_SERVER_CT_LVTSHR_ITERATOR_H

#include <iterator>
#include <tuple>

namespace Codethink::lvtshr {

template<typename T,
         typename TIter = decltype(std::begin(std::declval<T>())),
         typename = decltype(std::end(std::declval<T>()))>
constexpr auto enumerate(T&& iterable)
{
    struct iterator {
        bool operator!=(const iterator& other) const
        {
            return iter != other.iter;
        }

        void operator++()
        {
            ++i;
            ++iter;
        }

        auto operator*() const
        {
            return std::tie(i, *iter);
        }

        std::size_t i;
        TIter iter;
    };
    struct enumerate_t {
        auto begin()
        {
            return iterator{0, std::begin(iterable)};
        }

        auto end()
        {
            return iterator{0, std::end(iterable)};
        }

        T iterable;
    };
    return enumerate_t{std::forward<T>(iterable)};
}

} // namespace Codethink::lvtshr

#endif // DIAGRAM_SERVER_CT_LVTSHR_ITERATOR_H
