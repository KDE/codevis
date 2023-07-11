// ct_lvtqtc_logicalentity.cpp                                      -*-C++-*-

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

#include <ct_lvtqtc_logicalentity.h>

#include <ct_lvtldr_lakosiannode.h>
#include <ct_lvtldr_typenode.h>

#include <QGraphicsSimpleTextItem>

// TODO: Create a TextItem that handles ellipsis when too big.
namespace Codethink::lvtqtc {

struct LogicalEntity::Private {
    QGraphicsSimpleTextItem *typeText = nullptr;
};

LogicalEntity::LogicalEntity(lvtldr::LakosianNode *node, lvtshr::LoaderInfo info):
    LakosEntity(getUniqueId(node->id()), node, info), d(std::make_unique<Private>())
{
    updateTooltip();
    setRoundRadius(20);
    setText(node->name());

    auto *logicalNode = dynamic_cast<lvtldr::TypeNode *>(node);
    assert(logicalNode);

    d->typeText = new QGraphicsSimpleTextItem(this);

    const QString text = [logicalNode]() -> QString {
        switch (logicalNode->kind()) {
        case lvtshr::UDTKind::Class:
            return tr("class");
        case lvtshr::UDTKind::Enum:
            return tr("Enum");
        case lvtshr::UDTKind::Struct:
            return tr("Struct");
        case lvtshr::UDTKind::TypeAlias:
            return tr("Type Alias");
        case lvtshr::UDTKind::Union:
            return tr("Union");
        case lvtshr::UDTKind::Unknown:
            return tr("Unknown");
        }
        return {};
    }();

    d->typeText->setText(text);

    connect(this, &LogicalEntity::rectangleChanged, this, [this] {
        constexpr int SPACING = 5;
        d->typeText->setPos(rect().topLeft().x() + SPACING, rect().topLeft().y());
    });
}

LogicalEntity::~LogicalEntity() = default;

void LogicalEntity::updateTooltip()
{
    makeToolTip("(no parent package)");
}

std::string LogicalEntity::getUniqueId(const long long id)
{
    return "class_declaration#" + std::to_string(id);
}

std::string LogicalEntity::colorIdText() const
{
    if (colorId().empty()) {
        return name() + " is not in a package";
    }
    return name() + " is in package " + colorId();
}

lvtshr::DiagramType LogicalEntity::instanceType() const
{
    return lvtshr::DiagramType::ClassType;
}

} // end namespace Codethink::lvtqtc
