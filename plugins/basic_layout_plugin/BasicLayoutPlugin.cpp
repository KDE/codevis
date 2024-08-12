#include "BasicLayoutPlugin.h"
#include "ICodevisPlugin.h"

#include <KPluginFactory>
#include <qnamespace.h>

BasicLayoutPlugin::BasicLayoutPlugin(QObject *parent, const QVariantList& args):
    Codevis::PluginSystem::ICodevisPlugin(parent, args)
{
}

QString BasicLayoutPlugin::pluginName()
{
    return "Basic Layout";
}

QString BasicLayoutPlugin::pluginDescription()
{
    return "Applies a basic layout on a given graph";
}

QList<QString> BasicLayoutPlugin::pluginAuthors()
{
    return {"Tomaz Canabrava - tcanabrava@kde.org"};
}

// IGraphicsLayoutPlugin
QList<QString> BasicLayoutPlugin::layoutAlgorithms()
{
    return {tr("Top to Bottom"), tr("Bottom to Top"), tr("Left to Right"), tr("Right to Left")};
}

using Node = Codevis::PluginSystem::IGraphicsLayoutPlugin::Node;
using Graph = Codevis::PluginSystem::IGraphicsLayoutPlugin::Graph;

BasicLayoutPluginConfig config;

QWidget *BasicLayoutPlugin::configureWidget()
{
    return nullptr;
}

void levelizationLayout(Node& n,
                        BasicLayoutPluginConfig::LevelizationLayoutType type,
                        int direction,
                        std::optional<QPointF> moveToPosition = std::nullopt);

void recursiveLevelLayout(Node& n, int direction)
{
    for (auto& node : n.children) {
        recursiveLevelLayout(node, direction);
    }

    levelizationLayout(n, BasicLayoutPluginConfig::LevelizationLayoutType::Vertical, direction);
};

// clang-format off
template<BasicLayoutPluginConfig::LevelizationLayoutType LTS>
struct LayoutTypeStrategy
{
};

template<>
struct LayoutTypeStrategy<BasicLayoutPluginConfig::LevelizationLayoutType::Vertical>
{
    static inline double getPosOnReferenceDirection(const Node& entity) { return entity.pos.y(); }
    static inline double getPosOnOrthoDirection(const Node& entity) { return entity.pos.x(); }
    static inline void setPosOnReferenceDirection(Node& entity, double pos) { entity.pos = QPointF(entity.pos.x(), pos); }
    static inline void setPosOnOrthoDirection(Node& entity, double pos) { entity.pos = QPointF(pos, entity.pos.y()); }
    static inline double rectSize(const Node& entity) { return entity.rect.height(); }
    static inline double rectOrthoSize(const Node& entity) { return entity.rect.width(); }
};

template<>
struct LayoutTypeStrategy<BasicLayoutPluginConfig::LevelizationLayoutType::Horizontal>
{
    static inline double getPosOnReferenceDirection(const Node& entity) { return entity.pos.x(); }
    static inline double getPosOnOrthoDirection(const Node& entity) { return entity.pos.y(); }
    static inline void setPosOnReferenceDirection(Node& entity, double pos) { entity.pos = QPointF(pos, entity.pos.y()); }
    static inline void setPosOnOrthoDirection(Node& entity, double pos) { entity.pos = QPointF(entity.pos.x(), pos); }
    static inline double rectSize(const Node& entity) { return entity.rect.width(); }
    static inline double rectOrthoSize(const Node& entity) { return entity.rect.height(); }
};

template<BasicLayoutPluginConfig::LevelizationLayoutType LT>
void limitNumberOfEntitiesPerLevel(std::vector<Node>& nodes,
                                   BasicLayoutPluginConfig const& config)
{
    using LTS = LayoutTypeStrategy<LT>;

    auto entitiesFromLevel = std::map<int, std::vector<Node*>>{};
    for (auto& node : nodes) {
        entitiesFromLevel[node.lakosianLevel].push_back(&node);
    }

    for (auto& [level, entities] : entitiesFromLevel) {
        std::sort(entities.begin(), entities.end(), [](auto &e0, auto &e1) {
            return LTS::getPosOnOrthoDirection(*e0) < LTS::getPosOnOrthoDirection(*e1);
        });
    }

    auto globalOffset = 0.;
    for (auto& [level, entities] : entitiesFromLevel) {
        for (auto& e : entities) {
            auto currentPos = LTS::getPosOnReferenceDirection(*e);
            LTS::setPosOnReferenceDirection(*e, currentPos + config.direction * globalOffset);
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

            auto currentReferencePos = LTS::getPosOnReferenceDirection(*e);
            LTS::setPosOnReferenceDirection(*e, currentReferencePos + config.direction * localOffset);
            maxSizeOnCurrentLevel = std::max(maxSizeOnCurrentLevel, LTS::rectSize(*e));
            localNumberOfEntities += 1;
        }
        globalOffset += localOffset;
    }
}

template<BasicLayoutPluginConfig::LevelizationLayoutType LT>
void centralizeLayout(std::vector<Node>& nodes,
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
        LTS::setPosOnOrthoDirection(e, currentPos + (maxSize - lineToLineTotalWidth[lineRepr]) / 2.);
        lineCurrentPos[lineRepr] += LTS::rectOrthoSize(e) + config.spaceBetweenEntities;
    }
}

template<BasicLayoutPluginConfig::LevelizationLayoutType LT>
void prepareEntityPositionForEachLevel(std::vector<Node>& nodes,
                                       BasicLayoutPluginConfig const& config)
{
    using LTS = LayoutTypeStrategy<LT>;

    auto sizeForLvl = std::map<int, double>{};
    for (auto const& node : nodes) {
        sizeForLvl[node.lakosianLevel] = std::max(sizeForLvl[node.lakosianLevel], LTS::rectSize(node));
    }

    auto posPerLevel = std::map<int, double>{};
    for (auto const& [level, _] : sizeForLvl) {
        // clang-format off
        posPerLevel[level] = (
            level == 0 ? 0.0 : posPerLevel[level - 1] + config.direction * (sizeForLvl[level - 1] + config.spaceBetweenLevels)
        );
        // clang-format on
    }

    for (auto node : nodes) {
        LTS::setPosOnOrthoDirection(node, 0.0);
        LTS::setPosOnReferenceDirection(node, posPerLevel[node.lakosianLevel]);
    }
}

void runLevelizationLayout(std::vector<Node>& nodes, BasicLayoutPluginConfig const& config)
{
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

void levelizationLayout(Node& n,
                        BasicLayoutPluginConfig::LevelizationLayoutType type,
                        int direction,
                        std::optional<QPointF> moveToPosition)
{
    runLevelizationLayout(n.children,
                          {type,
                           direction,
                           config.spaceBetweenLevels,
                           config.spaceBetweenSublevels,
                           config.spaceBetweenEntities,
                           config.maxEntitiesPerLevel});
}

void BasicLayoutPlugin::executeLayout(const QString& algorithmName, Graph& g)
{
    std::ignore = algorithmName;
    auto direction = (algorithmName == tr("Top to Bottom") || algorithmName == tr("Left to Right")) ? +1 : -1;

    for (auto& node : g.topLevelNodes) {
        recursiveLevelLayout(node, direction);
    }

    runLevelizationLayout(g.topLevelNodes,
                          {BasicLayoutPluginConfig::LevelizationLayoutType::Vertical,
                           direction,
                           config.spaceBetweenLevels,
                           config.spaceBetweenSublevels,
                           config.spaceBetweenEntities,
                           config.maxEntitiesPerLevel});
}

K_PLUGIN_CLASS_WITH_JSON(BasicLayoutPlugin, "codevis_basiclayoutplugin.json")

#include "BasicLayoutPlugin.moc"
