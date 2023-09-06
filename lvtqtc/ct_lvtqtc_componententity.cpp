// ct_lvtqtc_componententity.cpp                                     -*-C++-*-

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

#include <ct_lvtqtc_componententity.h>

#include <ct_lvtldr_lakosiannode.h>

#include <ct_lvtqtc_logicalentity.h>

#include <QBrush>
#include <QPen>

#include <preferences.h>

namespace Codethink::lvtqtc {

std::string nodeNameWithoutParentPrefix(lvtldr::LakosianNode *node, const std::string& name)
{
    auto *parent = node->parent();
    if (!parent) {
        return name;
    }

    if (name.find(parent->name()) == 0) {
        auto newName = name;
        newName.erase(0, parent->name().size() + 1);
        return newName;
    }

    return name;
}

ComponentEntity::ComponentEntity(lvtldr::LakosianNode *node, lvtshr::LoaderInfo loaderInfo):
    LakosEntity(getUniqueId(node->id()), node, loaderInfo)
{
    updateTooltip();
    ComponentEntity::setText(node->name());
    truncateTitle(EllipsisTextItem::Truncate::No);

    setFont(Preferences::self()->componentFont());
    connect(Preferences::self(), &Preferences::componentFontChanged, this, [this] {
        setFont(Preferences::self()->componentFont());
    });

    connect(Preferences::self(), &Preferences::hidePackagePrefixOnComponentsChanged, this, [this] {
        setText(name());
    });
}

void ComponentEntity::setText(const std::string& text)
{
    auto newText =
        Preferences::self()->hidePackagePrefixOnComponents() ? nodeNameWithoutParentPrefix(internalNode(), text) : text;
    LakosEntity::setText(newText);
}

void ComponentEntity::updateTooltip()
{
    makeToolTip("(no namespace)");
}

std::string ComponentEntity::getUniqueId(const long long id)
{
    return "source_component#" + std::to_string(id);
}

lvtshr::DiagramType ComponentEntity::instanceType() const
{
    return lvtshr::DiagramType::ComponentType;
}

} // end namespace Codethink::lvtqtc
