#include <catch2-local-includes.h>

#include "ct_lvtplg_plugindatatypes.h"
#include "plugin.cpp"
#include <memory>

struct FakeNode {
    std::string name;
    std::vector<FakeNode *> connections;
    std::vector<FakeNode *> children;
};

std::shared_ptr<Codethink::lvtplg::Entity> createEntityFromFake(FakeNode& f)
{
    auto e = std::make_shared<Codethink::lvtplg::Entity>();
    auto getName = [&f] {
        return f.name;
    };

    e->getName = getName;
    return e;
}

TEST_CASE("Find Subgraphs")
{
    std::array<FakeNode, 10> nodes;

    auto ptr0 = createEntityFromFake(nodes[0]);
    auto ptr1 = createEntityFromFake(nodes[1]);
    auto ptr2 = createEntityFromFake(nodes[2]);
    auto ptr3 = createEntityFromFake(nodes[3]);
    auto ptr4 = createEntityFromFake(nodes[4]);
    auto ptr5 = createEntityFromFake(nodes[5]);
    auto ptr6 = createEntityFromFake(nodes[6]);
    auto ptr7 = createEntityFromFake(nodes[7]);
    auto ptr8 = createEntityFromFake(nodes[8]);
    auto ptr9 = createEntityFromFake(nodes[9]);

    const auto getAllEntitiesInCurrentView = []() -> std::vector<std::shared_ptr<Codethink::lvtplg::Entity>> {
        return {};
    };

    PluginContextMenuActionHandler handler{
        [](const std::string& id) -> void * {
            return nullptr;
        }, // getPluginData,
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

    findSubgraphsToplevel(&handler);
}
