// ct_lvtmdl_treefiltermodel.cpp                                     -*-C++-*-

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

#include <ct_lvtmdl_treefiltermodel.h>

#include <ct_lvtshr_fuzzyutil.h>

namespace Codethink::lvtmdl {

// --------------------------------------------
// struct TreeFilterModelPrivate
// --------------------------------------------
struct TreeFilterModel::TreeFilterModelPrivate {
    std::string filter;
};

// --------------------------------------------
// class TreeFilterModel
// --------------------------------------------
TreeFilterModel::TreeFilterModel(): d(std::make_unique<TreeFilterModelPrivate>())
{
}

TreeFilterModel::~TreeFilterModel() noexcept = default;

void TreeFilterModel::setFilter(const QString& filter)
{
    d->filter = filter.toStdString();
    invalidate();
}

bool TreeFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    // Don't try to filter if there's no filter.
    if (d->filter.empty()) {
        return true; // RETURN
    }

    auto data = sourceModel()->index(sourceRow, 0, sourceParent).data(Qt::DisplayRole).toString().toStdString();

    const bool show = data.find(d->filter) != std::string::npos;

    if (show) {
        return true; // RETURN
    }

    // If we don't have an inner string, use the levensteinDistance.
    // we accept an error factor of 2 letters
    bool foundFuzzy = lvtshr::FuzzyUtil::levensteinDistance(data, d->filter) < 3;
    if (foundFuzzy) {
        return true; // RETURN
    }

    // If we don't have an exact match, and we don't have a fuzzy match,
    // this can be a folder. and an item inside of the folder could be
    // visible.
    auto currIdx = sourceModel()->index(sourceRow, 0, sourceParent);
    if (sourceModel()->rowCount(currIdx) != 0) {
        const bool showFolder = data.find(d->filter) != std::string::npos;

        if (showFolder) {
            return true; // RETURN
        }

        // maybe this is slow, we need to test.
        for (int i = 0; i < sourceModel()->rowCount(currIdx); i++) {
            if (filterAcceptsRow(i, currIdx)) {
                return true; // RETURN
            }
        }
        return false; // RETURN
    }

    return false; // RETURN
}

} // end namespace Codethink::lvtmdl
