// ct_lvtcgn_testutils.h                                              -*-C++-*-

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

#ifndef CT_LVTCGN_TESTUTILS_H_INCLUDED
#define CT_LVTCGN_TESTUTILS_H_INCLUDED

#include <ct_lvtcgn_generatecode.h>
#include <memory>
#include <unordered_map>
#include <utility>

using namespace Codethink::lvtcgn::mdl;

struct FakeEntity {
    std::string name;
    std::string type;
    bool selected;
    std::vector<std::reference_wrapper<FakeEntity>> children;
    std::vector<std::reference_wrapper<FakeEntity>> fwdDeps;
    std::optional<std::reference_wrapper<FakeEntity>> parent;

    FakeEntity(std::string name, std::string type, bool selected):
        name(std::move(name)), type(std::move(type)), selected(selected), parent(std::nullopt)
    {
    }

    void addChild(FakeEntity& child)
    {
        child.parent = *this;
        children.emplace_back(child);
    }

    void addFwdDep(FakeEntity& other)
    {
        fwdDeps.emplace_back(other);
    }
};

class FakeContentProvider : public ICodeGenerationDataProvider {
  public:
    FakeContentProvider();
    virtual ~FakeContentProvider() = default;

    [[nodiscard]] std::vector<std::reference_wrapper<IPhysicalEntityInfo>> topLevelEntities() override
    {
        return rootEntities;
    }

    [[nodiscard]] int numberOfPhysicalEntities() const override
    {
        return data.size();
    }

    [[nodiscard]] IPhysicalEntityInfo& getInfoFor(FakeEntity *entity)
    {
        return *infos[entity];
    }

  private:
    std::vector<std::reference_wrapper<IPhysicalEntityInfo>> rootEntities;
    std::vector<std::unique_ptr<FakeEntity>> data;
    std::unordered_map<FakeEntity *, std::unique_ptr<IPhysicalEntityInfo>> infos;
};

class FakeEntityInfo : public IPhysicalEntityInfo {
  public:
    explicit FakeEntityInfo(FakeEntity& entity, FakeContentProvider& provider): d_entity(entity), d_provider(provider)
    {
    }

    ~FakeEntityInfo() override = default;

    [[nodiscard]] std::string name() const override
    {
        return d_entity.name;
    }

    [[nodiscard]] std::string type() const override
    {
        return d_entity.type;
    }

    [[nodiscard]] std::optional<std::reference_wrapper<IPhysicalEntityInfo>> parent() const override
    {
        if (!d_entity.parent.has_value()) {
            return std::nullopt;
        }
        return d_provider.getInfoFor(&d_entity.parent.value().get());
    }

    [[nodiscard]] std::vector<std::reference_wrapper<IPhysicalEntityInfo>> children() const override
    {
        std::vector<std::reference_wrapper<IPhysicalEntityInfo>> children;
        for (auto const& e : d_entity.children) {
            children.emplace_back(d_provider.getInfoFor(&(e.get())));
        }
        return children;
    }

    [[nodiscard]] std::vector<std::reference_wrapper<IPhysicalEntityInfo>> fwdDependencies() const override
    {
        std::vector<std::reference_wrapper<IPhysicalEntityInfo>> fwdDeps;
        for (auto const& e : d_entity.fwdDeps) {
            fwdDeps.emplace_back(d_provider.getInfoFor(&(e.get())));
        }
        return fwdDeps;
    }

    [[nodiscard]] bool selectedForCodeGeneration() const override
    {
        return d_entity.selected;
    }

    void setSelectedForCodeGeneration(bool value) override
    {
        d_entity.selected = value;
    }

  private:
    FakeEntity& d_entity;
    FakeContentProvider& d_provider;
};

FakeContentProvider::FakeContentProvider()
{
    auto somepkg_a = std::make_unique<FakeEntity>("somepkg_a", "Package", true);
    auto somepkg_b = std::make_unique<FakeEntity>("somepkg_b", "Package", false);
    auto somepkg_c = std::make_unique<FakeEntity>("somepkg_c", "Package", true);
    auto component_a = std::make_unique<FakeEntity>("component_a", "Component", false);
    auto component_b = std::make_unique<FakeEntity>("component_b", "Component", true);
    component_b->addFwdDep(*component_a);
    somepkg_c->addChild(*component_a);
    somepkg_c->addChild(*component_b);

    infos[somepkg_a.get()] = std::make_unique<FakeEntityInfo>(*somepkg_a, *this);
    infos[somepkg_b.get()] = std::make_unique<FakeEntityInfo>(*somepkg_b, *this);
    infos[somepkg_c.get()] = std::make_unique<FakeEntityInfo>(*somepkg_c, *this);
    infos[component_a.get()] = std::make_unique<FakeEntityInfo>(*component_a, *this);
    infos[component_b.get()] = std::make_unique<FakeEntityInfo>(*component_b, *this);
    rootEntities.emplace_back(*infos[somepkg_a.get()]);
    rootEntities.emplace_back(*infos[somepkg_b.get()]);
    rootEntities.emplace_back(*infos[somepkg_c.get()]);
    data.emplace_back(std::move(somepkg_a));
    data.emplace_back(std::move(somepkg_b));
    data.emplace_back(std::move(somepkg_c));
    data.emplace_back(std::move(component_a));
    data.emplace_back(std::move(component_b));
}

#endif
