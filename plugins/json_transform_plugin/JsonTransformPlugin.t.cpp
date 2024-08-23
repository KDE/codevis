
TEST_CASE_METHOD(QTApplicationFixture, "Json Bulk Edit")
{
    auto tmpDir = TmpDir{"relayout_single_entity"};
    auto dbPath = tmpDir.path() / "codedb.db";
    auto nodeStorage = NodeStorageTestUtils::createEmptyNodeStorage(dbPath);

    auto gv = GraphicsViewWrapperForTesting{nodeStorage};
    auto *gs = qobject_cast<GraphicsScene *>(gv.scene());
    gv.setColorManagement(std::make_shared<ColorManagement>(false));
    gv.show();

    auto *pkg = nodeStorage.addPackage("pkg", "pkg", nullptr, gv.scene()).value();
    std::ignore = nodeStorage.addComponent("pkg_comp1", "pkg_comp1", pkg);
    std::ignore = nodeStorage.addComponent("pkg_comp2", "pkg_comp2", pkg);
    auto *pkg2 = nodeStorage.addPackage("pkg2", "pkg2", nullptr, gv.scene()).value();
    std::ignore = nodeStorage.addComponent("pkg_comp3", "pkg_comp3", pkg2);
    std::ignore = nodeStorage.addComponent("pkg_comp4", "pkg_comp4", pkg2);

    auto *pkg1 = gs->entityByQualifiedName("pkg");
    std::ignore = gs->entityByQualifiedName("pkg2");
    std::ignore = gs->entityByQualifiedName("pkg_comp1");
    std::ignore = gs->entityByQualifiedName("pkg_comp2");

    std::ignore = gs->entityByQualifiedName("pkg_comp3");
    gs->reLayout();

    QStringList wrongJsonsDontCrash{
        "", // Empty Json
        "{}", // Single Object
        R"( {"root": []} )", // Wrong initial object
        R"({"elements":[{}]})", // Has Elements but inner items are wrong.
        R"({"elements":{}})", // Has Elements but should be array instead of object
        R"({"elements":[{ "name" }])", // Inner Missing Object
        R"({"elements":[{ "naame": "test" }])", // Missing "name" key on inner object
        R"({"elements":[{ "naame": "test" }])", // Missing "name" key on inner object
    };

    for (const auto& wrongJson : wrongJsonsDontCrash) {
        gs->loadJsonWithDocumentChanges(wrongJson);
    }

    QString correctJson = R"(
    {
        "elements" : [
            {
                "name" : "pkg",
                "color" : "#003300"
            }
        ]
    }
    )";

    REQUIRE(pkg1->color() != QColor("#003300"));
    gs->loadJsonWithDocumentChanges(correctJson);
    REQUIRE(pkg1->color() == QColor("#003300"));
}
