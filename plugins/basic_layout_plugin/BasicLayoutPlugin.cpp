#include "BasicLayoutPlugin.h"
#include "ICodevisPlugin.h"
#include "IGraphicsLayoutPlugin.h"
#include "ct_lvtqtc_graphicsscene.h"
#include "ct_lvtqtc_lakosentity.h"

#include <KPluginFactory>
#include <qnamespace.h>
#include <variant>

BasicLayoutPlugin::BasicLayoutPlugin(QObject *parent): Codevis::PluginSystem::ICodevisPlugin(parent)
{
}

// IGraphicsLayoutPlugin
QList<QString> BasicLayoutPlugin::layoutAlgorithms()
{
    return {tr("Top to Bottom"), tr("Bottom to Top"), tr("Left to Right"), tr("Right to Left")};
}

BasicLayoutPluginConfig config;

QWidget *BasicLayoutPlugin::configureWidget()
{
    return nullptr;
}

namespace {

// clang-format off
template<BasicLayoutPluginConfig::LevelizationLayoutType LTS>
struct LayoutTypeStrategy
{
};

template<>
struct LayoutTypeStrategy<BasicLayoutPluginConfig::LevelizationLayoutType::Vertical>
{
    static inline double getPosOnReferenceDirection(LakosEntity* entity) { return entity->pos().y(); }
    static inline double getPosOnOrthoDirection(LakosEntity* entity) { return entity->pos().x(); }
    static inline void setPosOnReferenceDirection(LakosEntity* entity, double pos) { entity->setPos(QPointF(entity->pos().x(), pos)); }
    static inline void setPosOnOrthoDirection(LakosEntity* entity, double pos) { entity->setPos(QPointF(pos, entity->pos().y())); }
    static inline double rectSize(LakosEntity* entity) { return entity->rect().height(); }
    static inline double rectOrthoSize(LakosEntity* entity) { return entity->rect().width(); }
};

template<>
struct LayoutTypeStrategy<BasicLayoutPluginConfig::LevelizationLayoutType::Horizontal>
{
    static inline double getPosOnReferenceDirection(LakosEntity* entity) { return entity->pos().x(); }
    static inline double getPosOnOrthoDirection(LakosEntity* entity) { return entity->pos().y(); }
    static inline void setPosOnReferenceDirection(LakosEntity* entity, double pos) { entity->setPos(QPointF(pos, entity->pos().y())); }
    static inline void setPosOnOrthoDirection(LakosEntity* entity, double pos) { entity->setPos(QPointF(entity->pos().x(), pos)); }
    static inline double rectSize(LakosEntity* entity) { return entity->rect().width(); }
    static inline double rectOrthoSize(LakosEntity* entity) { return entity->rect().height(); }
};

template<BasicLayoutPluginConfig::LevelizationLayoutType LT>
void limitNumberOfEntitiesPerLevel(const std::vector<LakosEntity*>& nodes,
                                   BasicLayoutPluginConfig const& config)
{
    using LTS = LayoutTypeStrategy<LT>;

    auto entitiesFromLevel = std::map<int, std::vector<LakosEntity*>>{};
    for (auto& node : nodes) {
        entitiesFromLevel[node->lakosianLevel()].push_back(node);
    }

    for (auto& [level, entities] : entitiesFromLevel) {
        std::sort(entities.begin(), entities.end(), [](LakosEntity* e0, LakosEntity* e1) {
            return LTS::getPosOnOrthoDirection(e0) < LTS::getPosOnOrthoDirection(e1);
        });
    }

    auto globalOffset = 0.;
    for (auto& [level, entities] : entitiesFromLevel) {
        for (LakosEntity *e : entities) {
            auto currentPos = LTS::getPosOnReferenceDirection(e);
            LTS::setPosOnReferenceDirection(e, currentPos + config.direction * globalOffset);
        }

        double localOffset = 0.0;
        double maxSizeOnCurrentLevel = 0.0;
        auto localNumberOfEntities = 0;

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

template<BasicLayoutPluginConfig::LevelizationLayoutType LT>
void centralizeLayout(const std::vector<LakosEntity*>& nodes,
                      const BasicLayoutPluginConfig& config)
{
    using LTS = LayoutTypeStrategy<LT>;

    // Warning: A "line" is not the same as a "level". One level may be composed by multiple lines.
    auto lineToLineTotalWidth = std::unordered_map<int, double>{};
    auto maxSize = 0.;
    for (auto& node : nodes) {
        // The use of "integer" for a "line position" is only to avoid having to deal with real numbers, and
        // thus being able to make easy buckets for the lines.
        auto lineRepr = (int) LTS::getPosOnReferenceDirection(node);

        lineToLineTotalWidth[lineRepr] = lineToLineTotalWidth[lineRepr] == 0.0
        ? LTS::rectSize(node)
        : lineToLineTotalWidth[lineRepr] + LTS::rectOrthoSize(node) + config.spaceBetweenEntities;

        maxSize = std::max(lineToLineTotalWidth[lineRepr], maxSize);
    }

    auto lineCurrentPos = std::map<int, double>{};
    for (auto& e : nodes) {
        auto lineRepr = (int) LTS::getPosOnReferenceDirection(e);
        auto currentPos = lineCurrentPos[lineRepr];
        LTS::setPosOnOrthoDirection(e, currentPos + (maxSize - lineToLineTotalWidth[lineRepr]) / 2.0);
        lineCurrentPos[lineRepr] += LTS::rectOrthoSize(e) + config.spaceBetweenEntities;
    }
}

template<BasicLayoutPluginConfig::LevelizationLayoutType LT>
void prepareEntityPositionForEachLevel(const std::vector<LakosEntity*>& nodes,
                                       BasicLayoutPluginConfig const& config)
{
    using LTS = LayoutTypeStrategy<LT>;

    auto sizeForLvl = std::map<int, double>{};
    for (auto const& node : nodes) {
        node->setPos(0,0);
        sizeForLvl[node->lakosianLevel()] = std::max(sizeForLvl[node->lakosianLevel()], LTS::rectSize(node));
    }

    auto posPerLevel = std::map<int, double>{};
    for (auto const& [level, size] : sizeForLvl) {
        // clang-format off
        posPerLevel[level] = (
            level == 0 ? 0.0 : posPerLevel[level - 1] + config.direction * (sizeForLvl[level - 1] + config.spaceBetweenLevels)
        );
        // clang-format on
    }

    for (auto node : nodes) {
        LTS::setPosOnOrthoDirection(node, 0.0);
        LTS::setPosOnReferenceDirection(node, posPerLevel[node->lakosianLevel()]);
    }
}

void runLevelizationLayout(const std::vector<LakosEntity *>& nodes, BasicLayoutPluginConfig const& config)
{
    if (nodes.empty()) {
        return;
    }

    for (auto *node : nodes) {
        runLevelizationLayout(node->lakosEntities(), config);
    }

    if (config.type == BasicLayoutPluginConfig::LevelizationLayoutType::Vertical) {
        static auto const LT = BasicLayoutPluginConfig::LevelizationLayoutType::Vertical;
        prepareEntityPositionForEachLevel<LT>(nodes, config);
        limitNumberOfEntitiesPerLevel<LT>(nodes, config);
        centralizeLayout<LT>(nodes, config);
    } else {
        static auto const LT = BasicLayoutPluginConfig::LevelizationLayoutType::Horizontal;
        prepareEntityPositionForEachLevel<LT>(nodes, config);
        limitNumberOfEntitiesPerLevel<LT>(nodes, config);
        centralizeLayout<LT>(nodes, config);
    }
}
} // namespace

void BasicLayoutPlugin::executeLayout(const QString& algorithmName, IGraphicsLayoutPlugin::ExecuteLayoutParams params)
{
    auto direction = (algorithmName == tr("Top to Bottom") || algorithmName == tr("Left to Right")) ? +1 : -1;
    auto type = (algorithmName == tr("Top to Bottom") || algorithmName == tr("Bottom to Top"))
        ? BasicLayoutPluginConfig::LevelizationLayoutType::Vertical
        : BasicLayoutPluginConfig::LevelizationLayoutType::Horizontal;

    BasicLayoutPluginConfig config;

    struct visitor {
        BasicLayoutPluginConfig config;
        visitor(int direction, BasicLayoutPluginConfig::LevelizationLayoutType type)
        {
            config.direction = direction, config.maxEntitiesPerLevel = 10;
            config.spaceBetweenLevels = 20;
            config.type = type;
        }

        void operator()(GraphicsScene *scene)
        {
            std::vector<LakosEntity *> topLevel;
            for (LakosEntity *node : scene->allEntities()) {
                if (!node->parentItem()) {
                    topLevel.push_back(node);
                }
            }
            runLevelizationLayout(topLevel, config);
            scene->setSceneRect(scene->itemsBoundingRect());
        }

        void operator()(LakosEntity *parent)
        {
            runLevelizationLayout(parent->lakosEntities(), config);
        }
    };

    std::visit(visitor{direction, type}, params);
}

K_PLUGIN_CLASS_WITH_JSON(BasicLayoutPlugin, "codevis_basiclayoutplugin.json")

#include "BasicLayoutPlugin.moc"

#include "moc_BasicLayoutPlugin.cpp"
