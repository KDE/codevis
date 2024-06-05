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

#include <QDebug>
#include <QVector>
#include <memory>
#include <unordered_map>

#include <ct_lvtcgn_generatecode.h>

namespace Codethink::lvtcgn::mdl {
class IPhysicalEntityInfo;
class ICodeGenerationDataProvider;
} // namespace Codethink::lvtcgn::mdl

using namespace Codethink::lvtcgn::mdl;

struct FakeEntity {
    std::string name;
    std::string type;
    bool selected;
    QVector<FakeEntity *> children;
    QVector<FakeEntity *> fwdDeps;
    FakeEntity *parent;

    FakeEntity(std::string name, std::string type, bool selected):
        name(name), type(type), selected(selected), parent(nullptr)
    {
    }

    void addChild(FakeEntity *child)
    {
        child->parent = this;
        children.emplace_back(child);
    }

    void addFwdDep(FakeEntity *other)
    {
        fwdDeps.emplace_back(other);
    }
};

class FakeContentProvider : public ICodeGenerationDataProvider {
  public:
    FakeContentProvider();
    virtual ~FakeContentProvider() = default;

    [[nodiscard]] QVector<IPhysicalEntityInfo *> topLevelEntities() override
    {
        return _rootEntities;
    }

    [[nodiscard]] int numberOfPhysicalEntities() const override
    {
        return _data.size();
    }

    [[nodiscard]] IPhysicalEntityInfo *getInfoFor(FakeEntity *entity)
    {
        return _infos[entity].get();
    }

  private:
    QVector<IPhysicalEntityInfo *> _rootEntities;
    QVector<std::shared_ptr<FakeEntity>> _data;
    std::unordered_map<FakeEntity *, std::unique_ptr<IPhysicalEntityInfo>> _infos;
};

class FakeEntityInfo : public IPhysicalEntityInfo {
  public:
    explicit FakeEntityInfo(FakeEntity *entity, FakeContentProvider& provider): d_entity(entity), d_provider(provider)
    {
    }

    ~FakeEntityInfo() override
    {
        qDebug() << "Destroying" << this;
    };

    [[nodiscard]] QString name() const override
    {
        return QString::fromStdString(d_entity->name);
    }

    [[nodiscard]] QString type() const override
    {
        return QString::fromStdString(d_entity->type);
    }

    [[nodiscard]] IPhysicalEntityInfo *parent() const override
    {
        if (!d_entity->parent) {
            return nullptr;
        }

        return d_provider.getInfoFor(d_entity->parent);
    }

    [[nodiscard]] QVector<IPhysicalEntityInfo *> children() const override
    {
        QVector<IPhysicalEntityInfo *> children;
        for (auto const& e : d_entity->children) {
            children.emplace_back(d_provider.getInfoFor(e));
        }
        return children;
    }

    [[nodiscard]] QVector<IPhysicalEntityInfo *> fwdDependencies() const override
    {
        QVector<IPhysicalEntityInfo *> fwdDeps;
        for (auto const& e : d_entity->fwdDeps) {
            fwdDeps.emplace_back(d_provider.getInfoFor(e));
        }
        return fwdDeps;
    }

    [[nodiscard]] bool selectedForCodeGeneration() const override
    {
        return d_entity->selected;
    }

    void setSelectedForCodeGeneration(bool value) override
    {
        d_entity->selected = value;
    }

  private:
    FakeEntity *d_entity;
    FakeContentProvider& d_provider;
};

#endif
