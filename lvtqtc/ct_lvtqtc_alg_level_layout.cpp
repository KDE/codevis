// ct_lvtqtc_alg_level_layout.cpp                                    -*-C++-*-

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
#include <ct_lvtqtc_alg_level_layout.h>
#include <ct_lvtqtc_lakosentity.h>
#include <set>
#include <unordered_set>

namespace Codethink::lvtqtc {

std::unordered_map<LakosEntity *, int> computeLevelForEntities(std::vector<LakosEntity *> const& entities,
                                                               std::optional<const LakosEntity *> commonParentEntity)
{
    auto entityToLevel = std::unordered_map<LakosEntity *, int>{};

    // Keep track of history for cycle detection
    auto entitiesVisitHistory = std::unordered_map<LakosEntity *, std::unordered_set<LakosEntity *>>{};
    for (auto *entity : entities) {
        entitiesVisitHistory[entity].insert(entity);
    }

    auto copyAllDependentNodes =
        [&commonParentEntity, &entitiesVisitHistory](LakosEntity *entity, std::set<LakosEntity *>& processEntities) {
            for (auto const& edge : entity->targetCollection()) {
                auto *const toNode = edge->from();
                auto *const parentNode = toNode->internalNode()->parent();
                if (commonParentEntity) {
                    // Only consider packages/components and dependencies within the common parent entity
                    auto parentEntityName = (*commonParentEntity)->name();
                    if (!parentNode || parentNode->name() != parentEntityName) {
                        continue;
                    }
                } else {
                    // If no common parent is provided, only consider top level entities (Ignore those with parent node)
                    if (parentNode) {
                        continue;
                    }
                }

                // Avoid creating cycles
                auto const& history = entitiesVisitHistory[entity];
                if (std::find(history.cbegin(), history.cend(), toNode) != history.cend()) {
                    continue;
                }
                entitiesVisitHistory[toNode].insert(entitiesVisitHistory[entity].begin(),
                                                    entitiesVisitHistory[entity].end());

                processEntities.insert(toNode);
            }
        };

    auto hasDependenciesWithinThisParent = [&commonParentEntity](LakosEntity *entity) {
        auto const& deps = entity->edgesCollection();

        if (commonParentEntity) {
            // Only consider packages/components and dependencies within the common parent entity
            auto parentEntityName = (*commonParentEntity)->name();
            return std::any_of(deps.cbegin(), deps.cend(), [&parentEntityName](auto& dep) {
                auto const *depParent = dep->to()->internalNode()->parent();
                return depParent && depParent->name() == parentEntityName;
            });
        }

        // If no common parent is provided, only consider top level entities (With no parent)
        return std::any_of(deps.cbegin(), deps.cend(), [](auto& dep) {
            auto const *depParent = dep->to()->internalNode()->parent();
            return !depParent;
        });
    };

    auto entitiesOnNextLevel = std::set<LakosEntity *>{};

    // Finds the "first level", where the entities don't have any dependency.
    // Also prepares "processEntities" with the entities that will be on the next level.
    auto currentLevel = 0;
    for (auto *entity : entities) {
        entityToLevel[entity] = currentLevel;
        if (!hasDependenciesWithinThisParent(entity)) {
            copyAllDependentNodes(entity, entitiesOnNextLevel);
        }
    }

    // While there are entities in the current level, process the entities while finding the entities for the next
    // level, until eventually we have processed all the levels.
    auto entitiesOnCurrentLevel = entitiesOnNextLevel;
    currentLevel += 1;
    while (!entitiesOnCurrentLevel.empty()) {
        entitiesOnNextLevel.clear();
        for (auto const& entity : entitiesOnCurrentLevel) {
            entityToLevel[entity] = currentLevel;
            copyAllDependentNodes(entity, entitiesOnNextLevel);
        }

        currentLevel += 1;
        entitiesOnCurrentLevel = entitiesOnNextLevel;
    }
    return entityToLevel;
}

void runLevelizationLayout(std::unordered_map<LakosEntity *, int> const& entityToLevel,
                           LakosEntity::LevelizationLayoutType type,
                           int direction)
{
    auto currentLevelReferencePosition = std::unordered_map<int, double>{};
    if (type == LakosEntity::LevelizationLayoutType::Vertical) {
        auto const X_SPACING = 10.;
        auto const Y_SPACING = 40.;

        auto referenceHeightPerLvl = std::map<int, double>{};
        for (auto const& [e, level] : entityToLevel) {
            referenceHeightPerLvl[level] = std::max(referenceHeightPerLvl[level], e->rect().height());
        }

        auto yPosPerLevel = std::map<int, double>{};
        for (auto const& [level, referenceHeight] : referenceHeightPerLvl) {
            if (level == 0) {
                yPosPerLevel[level] = 0.0;
            } else {
                yPosPerLevel[level] =
                    yPosPerLevel[level - 1] + direction * (referenceHeightPerLvl[level - 1] + Y_SPACING);
            }
        }

        auto& currentLevelXPosition = currentLevelReferencePosition;
        for (auto [e, level] : entityToLevel) {
            auto width = e->rect().width();
            e->setPos(currentLevelXPosition[level], yPosPerLevel[level]);
            currentLevelXPosition[level] += width + X_SPACING;
        }
    } else {
        auto const X_SPACING = 40.;
        auto const Y_SPACING = 10.;

        auto referenceWidthPerLvl = std::map<int, double>{};
        for (auto const& [e, level] : entityToLevel) {
            referenceWidthPerLvl[level] = std::max(referenceWidthPerLvl[level], e->rect().width());
        }

        auto xPosPerLevel = std::map<int, double>{};
        for (auto const& [level, referenceWidth] : referenceWidthPerLvl) {
            if (level == 0) {
                xPosPerLevel[level] = 0.0;
            } else {
                xPosPerLevel[level] =
                    xPosPerLevel[level - 1] + direction * (referenceWidthPerLvl[level - 1] + X_SPACING);
            }
        }

        auto& currentLevelYPosition = currentLevelReferencePosition;
        for (auto [e, level] : entityToLevel) {
            auto height = e->rect().height();
            e->setPos(xPosPerLevel[level], currentLevelYPosition[level]);
            currentLevelYPosition[level] += height + Y_SPACING;
        }
    }

    // Centralize the generated layout
    if (type == LakosEntity::LevelizationLayoutType::Vertical) {
        auto& currentLevelXPosition = currentLevelReferencePosition;
        auto maxWidth = 0.;
        for (auto const& [_, maxXPosition] : currentLevelXPosition) {
            (void) _;
            maxWidth = std::max(maxWidth, maxXPosition);
        }

        auto currentLevelXOffsetToCentralize = std::unordered_map<int, double>{};
        for (auto const& [level, maxXPosition] : currentLevelXPosition) {
            currentLevelXOffsetToCentralize[level] = (maxWidth - maxXPosition) / 2;
        }

        for (auto [e, level] : entityToLevel) {
            e->setPos(e->pos().x() + currentLevelXOffsetToCentralize[level], e->pos().y());
        }
    } else {
        auto& currentLevelYPosition = currentLevelReferencePosition;
        auto maxHeight = 0.;
        for (auto const& [_, maxYPosition] : currentLevelYPosition) {
            (void) _;
            maxHeight = std::max(maxHeight, maxYPosition);
        }

        auto currentLevelYOffsetToCentralize = std::unordered_map<int, double>{};
        for (auto const& [level, maxYPosition] : currentLevelYPosition) {
            currentLevelYOffsetToCentralize[level] = (maxHeight - maxYPosition) / 2;
        }

        for (auto [e, level] : entityToLevel) {
            e->setPos(e->pos().x(), e->pos().y() + currentLevelYOffsetToCentralize[level]);
        }
    }
}

} // namespace Codethink::lvtqtc
