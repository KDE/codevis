// ct_lvtshr_graphstorage.h                                          -*-C++-*-

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

#ifndef INCLUDED_CT_LVTSHR_GRAPHSTORAGE
#define INCLUDED_CT_LVTSHR_GRAPHSTORAGE

#include <lvtshr_export.h>

#include <ct_lvtshr_graphenums.h>
#include <deque>
#include <functional>

// I'm not proud of this code.
namespace Codethink::lvtgrps {
struct EdgeCollection;
class LakosRelation;
class LakosEntity;
} // namespace Codethink::lvtgrps

namespace Codethink::lvtqtc {
struct EdgeCollection;
class LakosRelation;
class LakosEntity;
} // namespace Codethink::lvtqtc

namespace Codethink::lvtshr {

using EdgeCollection = lvtqtc::EdgeCollection;
using LakosRelation = lvtqtc::LakosRelation;
using LakosEntity = lvtqtc::LakosEntity;

template<typename Relation>
Relation filterEdgeFromCollection(std::vector<Relation>& relations,
                                  Relation relation,
                                  // This trait should be called only on Qt, because this function needs to delete
                                  // the relation if not used.
                                  // TODO: this is a hack, should be removed as soon as we can.
                                  std::function<void(Relation)> deleter)
// This function removes / deletes a vertex if it already exists on the relation, and returns the same entry.
// if it does not find the relation, returns a nullpointer.
{
    for (auto& entry : relations) {
        if (relation->from() != entry->from() || relation->to() != entry->to()) {
            continue;
        }

        /*
         * If a relation of type UsesInTheInterface and same
         * direction already exists, then don't add the
         * UsesInTheImplementation relation
         */
        if (relation->relationType() == lvtshr::LakosRelationType::UsesInTheImplementation
            && entry->relationType() == lvtshr::LakosRelationType::UsesInTheInterface) {
            deleter(relation);
            return entry;
        }

        /*
         * If a relation of type UsesInTheImplementation and already
         * exists, and a relation of type UsesInTheInterface with the
         * same direction is being inserted, then overwrite the
         * UsesInTheImplementation relation
         */
        if (relation->relationType() == lvtshr::LakosRelationType::UsesInTheImplementation
            && entry->relationType() == lvtshr::LakosRelationType::UsesInTheInterface) {
            deleter(entry);
            entry = relation;
            return entry;
        }

        /*
         * Don't add a new relation if a relation of the
         * same type and direction already exists
         */
        if (entry->relationType() == relation->relationType()) {
            deleter(relation);
            return entry;
        }
    }
    return nullptr;
}

} // end namespace Codethink::lvtshr

#endif
