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

// clang-format off
template<LakosEntity::LevelizationLayoutType LTS>
struct LayoutTypeStrategy
{
};

template<>
struct LayoutTypeStrategy<LakosEntity::LevelizationLayoutType::Vertical>
{
    static inline double getPosOnReferenceDirection(LakosEntity *entity) { return entity->pos().y(); }
    static inline double getPosOnOrthoDirection(LakosEntity *entity) { return entity->pos().x(); }
    static inline void setPosOnReferenceDirection(LakosEntity *entity, double pos) { entity->setPos(entity->pos().x(), pos); }
    static inline void setPosOnOrthoDirection(LakosEntity *entity, double pos) { entity->setPos(pos, entity->pos().y()); }
    static inline double rectSize(LakosEntity *entity) { return entity->rect().height(); }
    static inline double rectOrthoSize(LakosEntity *entity) { return entity->rect().width(); }
};

template<>
struct LayoutTypeStrategy<LakosEntity::LevelizationLayoutType::Horizontal>
{
    static inline double getPosOnReferenceDirection(LakosEntity *entity) { return entity->pos().x(); }
    static inline double getPosOnOrthoDirection(LakosEntity *entity) { return entity->pos().y(); }
    static inline void setPosOnReferenceDirection(LakosEntity *entity, double pos) { entity->setPos(pos, entity->pos().y()); }
    static inline void setPosOnOrthoDirection(LakosEntity *entity, double pos) { entity->setPos(entity->pos().x(), pos); }
    static inline double rectSize(LakosEntity *entity) { return entity->rect().width(); }
    static inline double rectOrthoSize(LakosEntity *entity) { return entity->rect().height(); }
};
// clang-format on

void copyAllDependentNodes(std::optional<const LakosEntity *>& commonParentEntity,
                           std::unordered_map<LakosEntity *, std::unordered_set<LakosEntity *>>& entitiesVisitHistory,
                           LakosEntity *entity,
                           std::set<LakosEntity *>& processEntities)
{
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
        entitiesVisitHistory[toNode].insert(entitiesVisitHistory[entity].begin(), entitiesVisitHistory[entity].end());

        processEntities.insert(toNode);
    }
};

bool hasDependenciesWithinThisParent(std::optional<const LakosEntity *>& commonParentEntity, LakosEntity *entity)
{
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

std::unordered_map<LakosEntity *, int> computeLevelForEntities(std::vector<LakosEntity *> const& entities,
                                                               std::optional<const LakosEntity *> commonParentEntity)
{
    std::cout << "Computing the level for " << entities.size() << "entities" << std::endl;
    auto entityToLevel = std::unordered_map<LakosEntity *, int>{};

    // Keep track of history for cycle detection
    auto entitiesVisitHistory = std::unordered_map<LakosEntity *, std::unordered_set<LakosEntity *>>{};
    std::vector<LakosEntity *> entitiesWithoutConnections;
    for (auto *entity : entities) {
        entitiesVisitHistory[entity].insert(entity);

        if (entity->edgesCollection().empty() && entity->targetCollection().empty()) {
            entitiesWithoutConnections.push_back(entity);
        }
    }
    auto entitiesOnNextLevel = std::set<LakosEntity *>{};

    // Finds the "first level", where the entities don't have any dependency.
    // Also prepares "processEntities" with the entities that will be on the next level.
    auto currentLevel = 0;
    for (auto *entity : entities) {
        entityToLevel[entity] = currentLevel;
        if (!hasDependenciesWithinThisParent(commonParentEntity, entity)) {
            copyAllDependentNodes(commonParentEntity, entitiesVisitHistory, entity, entitiesOnNextLevel);
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
            copyAllDependentNodes(commonParentEntity, entitiesVisitHistory, entity, entitiesOnNextLevel);
        }

        currentLevel += 1;
        entitiesOnCurrentLevel = entitiesOnNextLevel;
    }

    // move elements that have no edge to a higher level.
    for (auto *entity : entitiesWithoutConnections) {
        entityToLevel[entity] = currentLevel;
    }

    std::cout << "Finished" << std::endl;
    return entityToLevel;
}

template<LakosEntity::LevelizationLayoutType LT>
void limitNumberOfEntitiesPerLevel(std::unordered_map<LakosEntity *, int> const& entityToLevel,
                                   RunLevelizationLayoutConfig const& config)
{
    using LTS = LayoutTypeStrategy<LT>;

    auto entitiesFromLevel = std::map<int, std::vector<LakosEntity *>>{};
    for (auto const& [e, level] : entityToLevel) {
        entitiesFromLevel[level].push_back(e);
    }
    for (auto& [level, entities] : entitiesFromLevel) {
        std::sort(entities.begin(), entities.end(), [](auto *e0, auto *e1) {
            return LTS::getPosOnOrthoDirection(e0) < LTS::getPosOnOrthoDirection(e1);
        });
    }

    auto globalOffset = 0.;
    for (auto& [level, entities] : entitiesFromLevel) {
        for (auto& e : entities) {
            auto currentPos = LTS::getPosOnReferenceDirection(e);
            LTS::setPosOnReferenceDirection(e, currentPos + config.direction * globalOffset);
        }

        auto localOffset = 0.;
        auto localNumberOfEntities = 0;
        auto maxSizeOnCurrentLevel = 0.;
        for (auto& e : entities) {
            if (localNumberOfEntities == config.maxEntitiesPerLevel) {
                localOffset += maxSizeOnCurrentLevel + config.spaceBetweenSublevels;
                localNumberOfEntities = 0;
                maxSizeOnCurrentLevel = 0.;
            }

            auto currentReferencePos = LTS::getPosOnReferenceDirection(e);
            LTS::setPosOnReferenceDirection(e, currentReferencePos + config.direction * localOffset);
            maxSizeOnCurrentLevel = std::max(maxSizeOnCurrentLevel, LTS::rectSize(e));
            localNumberOfEntities += 1;
        }
        globalOffset += localOffset;
    }
}

template<LakosEntity::LevelizationLayoutType LT>
void centralizeLayout(std::unordered_map<LakosEntity *, int> const& entityToLevel,
                      RunLevelizationLayoutConfig const& config)
{
    using LTS = LayoutTypeStrategy<LT>;

    // Warning: A "line" is not the same as a "level". One level may be composed by multiple lines.
    auto lineToLineTotalWidth = std::unordered_map<int, double>{};
    auto maxSize = 0.;
    for (auto& [e, _] : entityToLevel) {
        // The use of "integer" for a "line position" is only to avoid having to deal with real numbers, and
        // thus being able to make easy buckets for the lines.
        auto lineRepr = (int) LTS::getPosOnReferenceDirection(e);

        lineToLineTotalWidth[lineRepr] = lineToLineTotalWidth[lineRepr] == 0.0
            ? LTS::rectOrthoSize(e)
            : lineToLineTotalWidth[lineRepr] + LTS::rectOrthoSize(e) + config.spaceBetweenEntities;

        maxSize = std::max(lineToLineTotalWidth[lineRepr], maxSize);
    }

    auto lineCurrentPos = std::map<int, double>{};
    for (auto& [e, _] : entityToLevel) {
        auto lineRepr = (int) LTS::getPosOnReferenceDirection(e);
        auto currentPos = lineCurrentPos[lineRepr];
        LTS::setPosOnOrthoDirection(e, currentPos + (maxSize - lineToLineTotalWidth[lineRepr]) / 2.);
        lineCurrentPos[lineRepr] += LTS::rectOrthoSize(e) + config.spaceBetweenEntities;
    }
}

template<LakosEntity::LevelizationLayoutType LT>
void prepareEntityPositionForEachLevel(std::unordered_map<LakosEntity *, int> const& entityToLevel,
                                       RunLevelizationLayoutConfig const& config)
{
    using LTS = LayoutTypeStrategy<LT>;

    auto sizeForLvl = std::map<int, double>{};
    for (auto const& [e, level] : entityToLevel) {
        sizeForLvl[level] = std::max(sizeForLvl[level], LTS::rectSize(e));
    }

    auto posPerLevel = std::map<int, double>{};
    for (auto const& [level, _] : sizeForLvl) {
        // clang-format off
        posPerLevel[level] = (
            level == 0 ? 0.0 : posPerLevel[level - 1] + config.direction * (sizeForLvl[level - 1] + config.spaceBetweenLevels)
        );
        // clang-format on
    }

    for (auto [e, level] : entityToLevel) {
        LTS::setPosOnOrthoDirection(e, 0.0);
        LTS::setPosOnReferenceDirection(e, posPerLevel[level]);
    }
}

void runLevelizationLayout(std::unordered_map<LakosEntity *, int> const& entityToLevel,
                           RunLevelizationLayoutConfig const& config)
{
    if (config.type == LakosEntity::LevelizationLayoutType::Vertical) {
        static auto const LT = LakosEntity::LevelizationLayoutType::Vertical;
        prepareEntityPositionForEachLevel<LT>(entityToLevel, config);
        limitNumberOfEntitiesPerLevel<LT>(entityToLevel, config);
        centralizeLayout<LT>(entityToLevel, config);
    } else {
        static auto const LT = LakosEntity::LevelizationLayoutType::Horizontal;
        prepareEntityPositionForEachLevel<LT>(entityToLevel, config);
        limitNumberOfEntitiesPerLevel<LT>(entityToLevel, config);
        centralizeLayout<LT>(entityToLevel, config);
    }
}

} // namespace Codethink::lvtqtc
