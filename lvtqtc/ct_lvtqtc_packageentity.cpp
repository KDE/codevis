// ct_lvtqtc_packageentity.cpp                                      -*-C++-*-

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

#include <ct_lvtqtc_packageentity.h>

#include <ct_lvtldr_lakosiannode.h>

#include <preferences.h>

namespace Codethink::lvtqtc {

PackageEntity::PackageEntity(lvtldr::LakosianNode *node, lvtshr::LoaderInfo info):
    LakosEntity(getUniqueId(node->id()), node, info)
{
    updateTooltip();
    setText(node->name());

    if (!node->parent() || node->parent()->type() == lvtshr::DiagramType::RepositoryType) {
        // Top level package (Package group or standalone)
        setNumOutlines(3);
    } else if (!node->parent()->parent() || node->parent()->parent()->type() == lvtshr::DiagramType::RepositoryType) {
        // Package inside package group
        setNumOutlines(2);
    } else {
        // Subpackages
        setNumOutlines(1);

        QPen currentPen = pen();
        currentPen.setStyle(Qt::PenStyle::DashLine);
        setPen(currentPen);
        forceHideLevelNumbers();
    }

    setFont(Preferences::pkgFont());
    connect(Preferences::self(), &Preferences::pkgFontChanged, this, [this] {
        setFont(Preferences::pkgFont());
    });

    connect(this, &LakosEntity::formFactorChanged, this, [this] {
        if (isExpanded()) {
            truncateTitle(EllipsisTextItem::Truncate::No);
        } else {
            truncateTitle(EllipsisTextItem::Truncate::Yes);
        }
    });
}

void PackageEntity::updateTooltip()
{
    makeToolTip("(no namespace)");
}

std::string PackageEntity::getUniqueId(const long long id)
{
    return "source_package#" + std::to_string(id);
}

lvtshr::DiagramType PackageEntity::instanceType() const
{
    return lvtshr::DiagramType::PackageType;
}

} // end namespace Codethink::lvtqtc
