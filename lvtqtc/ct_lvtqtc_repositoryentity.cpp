// ct_lvtqtc_repositoryentity.cpp                                      -*-C++-*-

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

#include <ct_lvtldr_lakosiannode.h>
#include <ct_lvtqtc_repositoryentity.h>
#include <preferences.h>

namespace Codethink::lvtqtc {

RepositoryEntity::RepositoryEntity(lvtldr::LakosianNode *node, lvtshr::LoaderInfo info):
    LakosEntity(getUniqueId(node->id()), node, info)
{
    updateTooltip();
    setText(node->name());

    setNumOutlines(1);
    QPen currentPen = pen();
    currentPen.setStyle(Qt::PenStyle::DashLine);
    setPen(currentPen);

    auto *fontPrefs = Preferences::self()->window()->fonts();
    setFont(fontPrefs->pkgFont());
    connect(fontPrefs, &Fonts::pkgFontChanged, this, &RepositoryEntity::setFont);

    connect(this, &LakosEntity::formFactorChanged, this, [this] {
        if (isExpanded()) {
            truncateTitle(EllipsisTextItem::Truncate::No);
        } else {
            truncateTitle(EllipsisTextItem::Truncate::Yes);
        }
    });
}

RepositoryEntity::~RepositoryEntity() = default;

void RepositoryEntity::updateTooltip()
{
    makeToolTip("(no namespace)");
}

std::string RepositoryEntity::getUniqueId(const long long id)
{
    return "repository#" + std::to_string(id);
}

lvtshr::DiagramType RepositoryEntity::instanceType() const
{
    return lvtshr::DiagramType::RepositoryType;
}

void RepositoryEntity::updateBackground()
{
    /* Intentionally left blank */
}

} // end namespace Codethink::lvtqtc
