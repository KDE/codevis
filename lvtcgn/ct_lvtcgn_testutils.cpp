#include <ct_lvtcgn_testutils.h>

FakeContentProvider::FakeContentProvider()
{
    auto somepkg_a = std::make_unique<FakeEntity>("somepkg_a", "Package", true);
    auto somepkg_b = std::make_unique<FakeEntity>("somepkg_b", "Package", false);
    auto somepkg_c = std::make_unique<FakeEntity>("somepkg_c", "Package", true);
    auto component_a = std::make_unique<FakeEntity>("component_a", "Component", false);
    auto component_b = std::make_unique<FakeEntity>("component_b", "Component", true);

    component_b->addFwdDep(component_a.get());
    somepkg_c->addChild(component_a.get());
    somepkg_c->addChild(component_b.get());

    _infos[somepkg_a.get()] = std::make_unique<FakeEntityInfo>(somepkg_a.get(), *this);
    _infos[somepkg_b.get()] = std::make_unique<FakeEntityInfo>(somepkg_b.get(), *this);
    _infos[somepkg_c.get()] = std::make_unique<FakeEntityInfo>(somepkg_c.get(), *this);
    _infos[component_a.get()] = std::make_unique<FakeEntityInfo>(component_a.get(), *this);
    _infos[component_b.get()] = std::make_unique<FakeEntityInfo>(component_b.get(), *this);

    _rootEntities.emplace_back(_infos[somepkg_a.get()].get());
    _rootEntities.emplace_back(_infos[somepkg_b.get()].get());
    _rootEntities.emplace_back(_infos[somepkg_c.get()].get());

    _data.emplace_back(std::move(somepkg_a));
    _data.emplace_back(std::move(somepkg_b));
    _data.emplace_back(std::move(somepkg_c));
    _data.emplace_back(std::move(component_a));
    _data.emplace_back(std::move(component_b));
}
