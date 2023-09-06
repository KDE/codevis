// ct_lvtqtc_packagedependency.cpp                                  -*-C++-*-

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

#include <ct_lvtldr_packagenode.h>
#include <ct_lvtqtc_lakosentity.h>
#include <ct_lvtqtc_packagedependency.h>

#include <QPainterPath>
#include <preferences.h>

namespace Codethink::lvtqtc {

PackageDependency::PackageDependency(LakosEntity *source, LakosEntity *target): LakosRelation(source, target)
{
    setHead(LakosRelation::defaultArrow());
    updateFlavor();
}

lvtshr::LakosRelationType PackageDependency::relationType() const
{
    return lvtshr::LakosRelationType::PackageDependency;
}

void PackageDependency::updateFlavor()
{
    if (!Preferences::self()->useDependencyTypes()) {
        setFlavor(PackageDependencyFlavor::ConcreteDependency);
        return;
    }

    // TODO: Make a different type of dependency for components (ComponentDependency)
    //       or rename PackageDependency to something else.
    // Components
    if (from()->internalNode()->type() != lvtshr::DiagramType::PackageType) {
        setFlavor(PackageDependencyFlavor::ConcreteDependency);
        return;
    }

    auto *fromNode = dynamic_cast<lvtldr::PackageNode *>(from()->internalNode());
    auto *toNode = dynamic_cast<lvtldr::PackageNode *>(to()->internalNode());
    auto *fromParent = dynamic_cast<lvtldr::PackageNode *>(fromNode->parent());
    auto *toParent = dynamic_cast<lvtldr::PackageNode *>(toNode->parent());
    auto isNonLakosian = [](auto *package) {
        return package && package->name() == "non-lakosian group";
    };

    // Special handlings for non lakosian groups
    if (isNonLakosian(fromParent) && isNonLakosian(toParent)) {
        // All dependencies within non-lakosian pseudogroup are implicitly allowed
        setFlavor(PackageDependencyFlavor::ConcreteDependency);
        return;
    }
    if (isNonLakosian(fromNode) && !isNonLakosian(toNode)) {
        // All dependencies from non-lakosian pseudogroup are invalid
        setFlavor(PackageDependencyFlavor::InvalidDesignDependency);
        return;
    }
    if (!isNonLakosian(fromNode) && (isNonLakosian(toNode) || isNonLakosian(toParent))) {
        // All dependencies to non-lakosian pseudogroup are implicitly allowed
        setFlavor(PackageDependencyFlavor::ConcreteDependency);
        return;
    }

    auto *packageNode = dynamic_cast<lvtldr::PackageNode *>(fromNode);
    assert(packageNode);
    auto constraints = PackageDependencyConstraint::Unknown;
    if (packageNode->hasAllowedDependency(toNode)) {
        constraints = constraints | PackageDependencyConstraint::AllowedDependency;
    }
    if (packageNode->hasConcreteDependency(toNode)) {
        constraints = constraints | PackageDependencyConstraint::ConcreteDependency;
    }

    if (static_cast<bool>(constraints & PackageDependencyConstraint::ConcreteDependency)
        && static_cast<bool>(constraints & PackageDependencyConstraint::AllowedDependency)) {
        // If two packages are concrete and allowed, but their parents aren't allowed, that`s an invalid design
        if (fromParent && toParent && fromParent != toParent) {
            if (!fromParent->hasAllowedDependency(toNode->parent())) {
                setFlavor(PackageDependencyFlavor::InvalidDesignDependency);
                return;
            }
        }

        bool hasOnlyTestDependencyBetweenPackages = [&]() {
            if (fromNode->children().empty()) {
                return false;
            }

            for (auto const& component : fromNode->children()) {
                if (component->type() != lvtshr::DiagramType::ComponentType) {
                    return false;
                }

                for (auto const& edge : component->providers()) {
                    if (edge.other()->parent() == toNode && !QString::fromStdString(component->name()).contains(".t")) {
                        return false;
                    }
                }
            }
            return true;
        }();
        if (hasOnlyTestDependencyBetweenPackages) {
            setFlavor(PackageDependencyFlavor::TestOnlyConcreteDependency);
        } else {
            setFlavor(PackageDependencyFlavor::ConcreteDependency);
        }
    } else if (static_cast<bool>(constraints & PackageDependencyConstraint::ConcreteDependency)
               && !static_cast<bool>(constraints & PackageDependencyConstraint::AllowedDependency)) {
        // Two packages are implicitly allowed to depend on each other if there's an allowed dependency between their
        // package groups (and the package groups are not the same)
        if (fromParent && toParent && fromParent != toParent) {
            if (fromParent->hasAllowedDependency(toParent)) {
                setFlavor(PackageDependencyFlavor::ConcreteDependency);
                return;
            }
        }

        // Two packages are implicitly allowed to depend on each other if the source is a standalone package and the
        // target is a package from which it's parent is a dependency from the standalone package.
        if (fromNode->isStandalone()) {
            if (toParent && fromNode->hasAllowedDependency(toParent)) {
                setFlavor(PackageDependencyFlavor::ConcreteDependency);
                return;
            }
        }

        // Two packages are implicitly allowed to depend on each other if the target is a standalone package and the
        // source is a package from which it's parent has a dependency to the standalone package.
        if (toNode->isStandalone()) {
            if (fromParent && fromParent->hasAllowedDependency(toNode)) {
                setFlavor(PackageDependencyFlavor::ConcreteDependency);
                return;
            }
        }

        if (dynamic_cast<lvtldr::PackageNode *>(fromNode)->hasAllowedDependency(toNode)) {
            setFlavor(PackageDependencyFlavor::ConcreteDependency);
        } else {
            setFlavor(PackageDependencyFlavor::InvalidDesignDependency);
        }
    } else if (static_cast<bool>(constraints & PackageDependencyConstraint::AllowedDependency)
               && !static_cast<bool>(constraints & PackageDependencyConstraint::ConcreteDependency)) {
        if (fromNode->isPackageGroup() && toNode->isPackageGroup()) {
            bool hasConcreteDependenciesOnChildren = [&]() {
                if (fromNode->children().empty() || toNode->children().empty()) {
                    return false;
                }

                for (auto const& entity : fromNode->children()) {
                    if (entity->type() != lvtshr::DiagramType::PackageType) {
                        return false;
                    }

                    for (auto const& edge : entity->providers()) {
                        if (edge.other()->parent() == toNode) {
                            return true;
                        }
                    }
                }
                return false;
            }();
            if (hasConcreteDependenciesOnChildren) {
                setFlavor(PackageDependencyFlavor::ConcreteDependency);
                return;
            }
        }

        setFlavor(PackageDependencyFlavor::AllowedDependency);
    } else {
        setFlavor(PackageDependencyFlavor::Unknown);
    }
}

void PackageDependency::setFlavor(PackageDependencyFlavor newFlavor)
{
    if (newFlavor == d_flavor) {
        return;
    }
    d_flavor = newFlavor;

    switch (newFlavor) {
    case PackageDependencyFlavor::Unknown: {
        setThickness(1);
        setDashed(false);
        setColor(QColor(100, 100, 100));
        break;
    }
    case PackageDependencyFlavor::ConcreteDependency: {
        setThickness(3);
        setDashed(false);
        // TODO [576]: Get colors from preferences
        setColor(QColor(20, 20, 150));
        break;
    }
    case PackageDependencyFlavor::AllowedDependency: {
        setThickness(3);
        setDashed(true);
        // TODO [576]: Get colors from preferences
        setColor(QColor(20, 150, 20));
        break;
    }
    case PackageDependencyFlavor::TestOnlyConcreteDependency: {
        setThickness(3);
        setDashed(false);
        // TODO [576]: Get colors from preferences
        setColor(QColor(20, 150, 20));
        break;
    }
    case PackageDependencyFlavor::InvalidDesignDependency: {
        setThickness(3);
        setDashed(false);
        // TODO [576]: Get colors from preferences
        setColor(QColor(150, 20, 20));
        break;
    }
    }
}

std::string PackageDependency::relationTypeAsString() const
{
    return "Package Dependency";
}

QColor PackageDependency::hoverColor() const
{
    switch (d_flavor) {
    case PackageDependencyFlavor::Unknown: {
        return {100, 100, 100};
    }
    case PackageDependencyFlavor::ConcreteDependency: {
        // TODO [576]: Get colors from preferences
        return {10, 10, 200};
    }
    case PackageDependencyFlavor::TestOnlyConcreteDependency: {
        // TODO [576]: Get colors from preferences
        return {10, 100, 40};
    }
    case PackageDependencyFlavor::AllowedDependency: {
        // TODO [576]: Get colors from preferences
        return {10, 100, 40};
    }
    case PackageDependencyFlavor::InvalidDesignDependency: {
        // TODO [576]: Get colors from preferences
        return {200, 40, 40};
    }
    }
    assert(false && "Unreachable");
    return {};
}

void PackageDependency::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    LakosRelation::paint(painter, option, widget);

    auto drawImage = [&](QImage const& image) {
        static constexpr auto SIZE = 20;
        auto scaledImage = image.scaled(SIZE, SIZE);
        auto p1 = adjustedLine().p1();
        auto p2 = adjustedLine().p2();
        auto midPoint = ((p1 + p2) / 2.).toPoint();
        auto xPos = midPoint.x() - SIZE / 2;
        auto yPos = midPoint.y() - SIZE / 2;
        painter->drawImage(xPos, yPos, scaledImage);
    };

    if (d_flavor == PackageDependencyFlavor::InvalidDesignDependency) {
        static auto image = QImage(":/icons/invalid_design");
        drawImage(image);
    }
    if (d_flavor == PackageDependencyFlavor::Unknown) {
        static auto image = QImage(":/icons/debug");
        drawImage(image);
    }
}

} // end namespace Codethink::lvtqtc
