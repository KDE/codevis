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

static auto const SPACE_BETWEEN_LEVELS = 40.;
static auto const SPACE_BETWEEN_SUBLEVELS = 10.;
static auto const SPACE_BETWEEN_ENTITIES = 10.;
static auto const MAX_ENTITY_PER_LEVEL = 8;

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

void limitNumberOfEntitiesPerLevel(std::unordered_map<LakosEntity *, int> const& entityToLevel,
                                   LakosEntity::LevelizationLayoutType type,
                                   int direction)
{
    if (type == LakosEntity::LevelizationLayoutType::Vertical) {
        auto entitiesFromLevel = std::map<int, std::vector<LakosEntity *>>{};
        for (auto const& [e, level] : entityToLevel) {
            entitiesFromLevel[level].push_back(e);
        }
        for (auto& [level, entities] : entitiesFromLevel) {
            std::sort(entities.begin(), entities.end(), [](auto *e0, auto *e1) {
                return e0->pos().x() < e1->pos().x();
            });
        }

        auto globalOffset = 0.;
        for (auto& [level, entities] : entitiesFromLevel) {
            for (auto& e : entities) {
                e->setPos(e->pos().x(), e->pos().y() + direction * globalOffset);
            }

            auto localOffset = 0.;
            auto localNumberOfEntities = 0;
            auto maxHeightOnCurrentLevel = 0.;
            for (auto& e : entities) {
                if (localNumberOfEntities == MAX_ENTITY_PER_LEVEL) {
                    localOffset += maxHeightOnCurrentLevel + SPACE_BETWEEN_SUBLEVELS;
                    localNumberOfEntities = 0;
                    maxHeightOnCurrentLevel = 0.;
                }

                e->setPos(e->pos().x(), e->pos().y() + direction * localOffset);
                maxHeightOnCurrentLevel = std::max(maxHeightOnCurrentLevel, e->rect().height());
                localNumberOfEntities += 1;
            }
            globalOffset += localOffset;
        }
    }
}

void centralizeLayout(std::unordered_map<LakosEntity *, int> const& entityToLevel,
                      LakosEntity::LevelizationLayoutType type,
                      int direction)
{
    if (type == LakosEntity::LevelizationLayoutType::Vertical) {
        // Warning: A "line" is not the same as a "level". One level may be composed by multiple lines.
        auto lineToLineTotalWidth = std::unordered_map<int, double>{};
        auto maxWidth = 0.;
        for (auto& [e, _] : entityToLevel) {
            // The use of "integer" for a "line position" is only to avoid having to deal with real numbers, and
            // thus being able to make easy buckets for the lines.
            auto yRepr = (int) e->pos().y();

            lineToLineTotalWidth[yRepr] = lineToLineTotalWidth[yRepr] == 0.0
                ? e->rect().width()
                : lineToLineTotalWidth[yRepr] + e->rect().width() + SPACE_BETWEEN_ENTITIES;

            maxWidth = std::max(lineToLineTotalWidth[yRepr], maxWidth);
        }

        auto lineCurrentXPos = std::map<int, double>{};
        for (auto& [e, _] : entityToLevel) {
            auto yRepr = (int) e->pos().y();

            auto currentXPos = lineCurrentXPos[yRepr];
            e->setPos(currentXPos + (maxWidth - lineToLineTotalWidth[yRepr]) / 2., e->pos().y());
            lineCurrentXPos[yRepr] += e->rect().width() + SPACE_BETWEEN_ENTITIES;
        }
    }
}

void prepareEntityPositionForEachLevel(std::unordered_map<LakosEntity *, int> const& entityToLevel,
                                       LakosEntity::LevelizationLayoutType type,
                                       int direction)
{
    if (type == LakosEntity::LevelizationLayoutType::Vertical) {
        auto heightForLvl = std::map<int, double>{};
        for (auto const& [e, level] : entityToLevel) {
            heightForLvl[level] = std::max(heightForLvl[level], e->rect().height());
        }

        auto yPosPerLevel = std::map<int, double>{};
        for (auto const& [level, _] : heightForLvl) {
            // clang-format off
            yPosPerLevel[level] = (
                level == 0 ? 0.0 : yPosPerLevel[level - 1] + direction * (heightForLvl[level - 1] + SPACE_BETWEEN_LEVELS)
            );
            // clang-format on
        }

        for (auto [e, level] : entityToLevel) {
            e->setPos(0.0, yPosPerLevel[level]);
        }
    } else {
        auto widthForLvl = std::map<int, double>{};
        for (auto const& [e, level] : entityToLevel) {
            widthForLvl[level] = std::max(widthForLvl[level], e->rect().width());
        }

        auto xPosPerLevel = std::map<int, double>{};
        for (auto const& [level, _] : widthForLvl) {
            // clang-format off
            xPosPerLevel[level] = (
                level == 0 ? 0.0 : xPosPerLevel[level - 1] + direction * (widthForLvl[level - 1] + SPACE_BETWEEN_LEVELS)
            );
            // clang-format on
        }

        for (auto [e, level] : entityToLevel) {
            e->setPos(xPosPerLevel[level], 0.0);
        }
    }
}

void runLevelizationLayout(std::unordered_map<LakosEntity *, int> const& entityToLevel,
                           LakosEntity::LevelizationLayoutType type,
                           int direction)
{
    prepareEntityPositionForEachLevel(entityToLevel, type, direction);
    limitNumberOfEntitiesPerLevel(entityToLevel, type, direction);
    centralizeLayout(entityToLevel, type, direction);
}

} // namespace Codethink::lvtqtc
