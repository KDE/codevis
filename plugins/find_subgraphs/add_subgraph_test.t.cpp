#include <catch2-local-includes.h>

#include "ct_lvtplg_basicpluginhandlers.h"
#include "ct_lvtplg_basicpluginhooks.h"
#include "ct_lvtplg_plugindatatypes.h"

#define PLUGIN_TEST_CODE 1
#include "plugin.cpp"

#include <memory>

using Entity = Codethink::lvtplg::Entity;

struct FakeNode {
    std::string name;
    std::vector<std::shared_ptr<Entity>> connections;
};

std::shared_ptr<Codethink::lvtplg::Entity> createEntityFromFake(FakeNode& f)
{
    auto e = std::make_shared<Entity>();
    auto getName = [&f] {
        return f.name;
    };

    auto getParent = []() -> std::shared_ptr<Entity> {
        return nullptr;
    };

    auto getDependencies = [&f]() -> std::vector<std::shared_ptr<Entity>> {
        return f.connections;
    };

    e->getName = getName;
    e->getParent = getParent;
    e->getDependencies = getDependencies;

    return e;
}

TEST_CASE("Find Subgraphs")
{
    std::array<FakeNode, 10> nodes;

    // Graph has three distinct entities:
    // 0 -> 1 -> 2
    // 0 -> 2
    // 3 -> 4 -> 5
    // 3-> 5
    // 6 -> 7 -> 9
    // 8 -> 9
    // 9 -> 6

    nodes[0].name = "0";
    nodes[1].name = "1";
    nodes[2].name = "2";
    nodes[3].name = "3";
    nodes[4].name = "4";
    nodes[5].name = "5";
    nodes[6].name = "6";
    nodes[7].name = "7";
    nodes[8].name = "8";
    nodes[9].name = "9";

    std::vector<std::shared_ptr<Codethink::lvtplg::Entity>> values{createEntityFromFake(nodes[0]),
                                                                   createEntityFromFake(nodes[1]),
                                                                   createEntityFromFake(nodes[2]),
                                                                   createEntityFromFake(nodes[3]),
                                                                   createEntityFromFake(nodes[4]),
                                                                   createEntityFromFake(nodes[5]),
                                                                   createEntityFromFake(nodes[6]),
                                                                   createEntityFromFake(nodes[7]),
                                                                   createEntityFromFake(nodes[8]),
                                                                   createEntityFromFake(nodes[9])};

    // First Subgraph
    nodes[0].connections.push_back(values[1]);
    nodes[1].connections.push_back(values[2]);
    nodes[0].connections.push_back(values[2]);

    // Second Subgraph
    nodes[3].connections.push_back(values[4]);
    nodes[4].connections.push_back(values[5]);
    nodes[3].connections.push_back(values[5]);

    // Last subgraph
    nodes[6].connections.push_back(values[7]);
    nodes[7].connections.push_back(values[9]);
    nodes[8].connections.push_back(values[9]);
    nodes[9].connections.push_back(values[6]);

    const auto getAllEntitiesInCurrentView = [values]() -> std::vector<std::shared_ptr<Codethink::lvtplg::Entity>> {
        return values;
    };

    void *pluginData = nullptr;
    const auto registerPluginData = [&pluginData](const std::string& id, void *data) {
        std::ignore = id;
        pluginData = data;
    };

    std::string testSceneName{"test-name"};
    PluginActiveSceneChangedHandler activeSceneHandler{
        [&pluginData](std::string const& id) -> void * {
            return pluginData;
        },
        [&testSceneName]() -> std::string {
            return testSceneName;
        },
        [](std::string const& id) -> PluginTreeWidgetHandler {
            return PluginTreeWidgetHandler{};
        },
    };

    PluginSetupHandler setupHandler{registerPluginData,
                                    [pluginData](const std::string& id) {
                                        return pluginData;
                                    },
                                    [](const std::string& id) {},
                                    []() {
                                        return PluginPythonInterpHandler{};
                                    }};

    PluginContextMenuActionHandler handler{
        [&pluginData](const std::string& id) -> void * {
            return pluginData;
        },
        getAllEntitiesInCurrentView,
        [](const std::string& qualName) -> std::shared_ptr<Codethink::lvtplg::Entity> {
            return {};
        }, //  getEntityByQualifiedName;
        [](const std::string& id) -> PluginTreeWidgetHandler {
            return {};
        }, // getTree;
        [](const std::string& id) -> PluginDockWidgetHandler {
            return {};
        }, // getDock;
        [](const std::string&, const std::string&) -> std::optional<Codethink::lvtplg::Edge> {
            return {};
        }, // getEdgeByQualifiedName;
        [](const std::string&) -> void {}, // loadEntityByQualifiedName;
        [](const std::string&, const std::string&) -> std::optional<Codethink::lvtplg::Edge> {
            return {};
        }, // addEdgeByQualifiedName;
        [](const std::string&, const std::string&) -> void {}, // removeEdgeByQualifiedName;
        [](const std::string&, const std::string&) -> bool {
            return false;
        }, // hasEdgeByQualifiedName;
        [](const std::string&) -> Codethink::lvtplg::RawDBRows {
            return {};
        } // runQueryOnDatabase;
    };

    hookSetupPlugin(&setupHandler);
    hookActiveSceneChanged(&activeSceneHandler);
    findSubgraphsToplevel(&handler);

    auto pluginData2 = getPluginData(&handler);
    std::cout << "Our Subgraphs" << pluginData2->ourGraphs[testSceneName].size() << "\n";
    REQUIRE(pluginData2->ourGraphs[testSceneName].size() == 3);
}
