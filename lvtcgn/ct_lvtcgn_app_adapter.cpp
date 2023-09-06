// ct_lvtcgn_app_adapter.cpp                              -*-C++-*-

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

#include <ct_lvtcgn_app_adapter.h>
#include <ct_lvtcgn_codegendialog.h>
#include <ct_lvtcgn_generatecode.h>

#include <ct_lvtldr_lakosiannode.h>
#include <ct_lvtldr_nodestorage.h>

#include <preferences.h>

using namespace Codethink::lvtcgn::mdl;
using namespace Codethink::lvtcgn::gui;
using namespace Codethink::lvtldr;
using namespace Codethink::lvtshr;

namespace {
bool shouldDefaultSelectForCodegen(LakosianNode& node)
{
    // External dependencies (Within non-lakosian group) are disabled by default
    if (node.name() == "non-lakosian group") {
        return false;
    }
    auto *parentNode = &node;
    while (parentNode->parent()) {
        parentNode = parentNode->parent();
        if (parentNode->name() == "non-lakosian group") {
            return false;
        }
    }

    return true;
}
} // namespace

namespace Codethink::lvtcgn::app {

WrappedLakosianNode::WrappedLakosianNode(NodeStorageDataProvider& dataProvider,
                                         LakosianNode *node,
                                         bool isSelectedForCodegen):
    dataProvider(dataProvider), node(node), isSelectedForCodegen(isSelectedForCodegen)
{
}

WrappedLakosianNode::~WrappedLakosianNode() = default;

std::string WrappedLakosianNode::name() const
{
    return node->name();
}

std::string WrappedLakosianNode::type() const
{
    std::string type = "Unknown";
    if (node->type() == DiagramType::ComponentType) {
        type = "Component";
    }
    if (node->type() == DiagramType::PackageType && node->parent() != nullptr) {
        type = "Package";
    }
    if (node->type() == DiagramType::PackageType && node->parent() == nullptr) {
        // If there's no parent, the node may be a package or a package group:
        auto const& children = node->children();
        if (children.empty()) {
            // (1) If there are no children, assume package.
            type = "Package";
        } else if (children.at(0)->type() == DiagramType::PackageType) {
            // (2) If any child is a package, assume package groups
            type = "PackageGroup";
        } else if (children.at(0)->type() == DiagramType::ComponentType) {
            // (3) If any child is a component, assume package
            type = "Package";
        }
    }
    return type;
}

std::optional<std::reference_wrapper<IPhysicalEntityInfo>> WrappedLakosianNode::parent() const
{
    if (!node->parent()) {
        return std::nullopt;
    }
    return dataProvider.getWrappedNode(node->parent());
}

std::vector<std::reference_wrapper<IPhysicalEntityInfo>> WrappedLakosianNode::children() const
{
    auto wrappedVec = std::vector<std::reference_wrapper<IPhysicalEntityInfo>>{};
    for (auto *e : node->children()) {
        if (e->type() == DiagramType::ComponentType || e->type() == DiagramType::PackageType) {
            wrappedVec.push_back(dataProvider.getWrappedNode(e));
        }
    }
    return wrappedVec;
}

std::vector<std::reference_wrapper<IPhysicalEntityInfo>> WrappedLakosianNode::fwdDependencies() const
{
    auto wrappedVec = std::vector<std::reference_wrapper<IPhysicalEntityInfo>>{};
    for (auto const& edge : node->providers()) {
        auto *e = edge.other();
        if (e->type() == DiagramType::ComponentType || e->type() == DiagramType::PackageType) {
            wrappedVec.push_back(dataProvider.getWrappedNode(e));
        }
    }
    return wrappedVec;
}

bool WrappedLakosianNode::selectedForCodeGeneration() const
{
    return isSelectedForCodegen;
}

void WrappedLakosianNode::setSelectedForCodeGeneration(bool value)
{
    isSelectedForCodegen = value;
}

NodeStorageDataProvider::NodeStorageDataProvider(Codethink::lvtldr::NodeStorage& sharedNodeStorage):
    ns(sharedNodeStorage)
{
}

NodeStorageDataProvider::~NodeStorageDataProvider() = default;

std::reference_wrapper<IPhysicalEntityInfo> NodeStorageDataProvider::getWrappedNode(LakosianNode *node)
{
    if (nodeToInfo.find(node) == nodeToInfo.end()) {
        nodeToInfo.insert(
            {node, std::make_unique<WrappedLakosianNode>(*this, node, shouldDefaultSelectForCodegen(*node))});
    }
    return *nodeToInfo.at(node);
}

std::vector<std::reference_wrapper<IPhysicalEntityInfo>> NodeStorageDataProvider::topLevelEntities()
{
    std::vector<std::reference_wrapper<IPhysicalEntityInfo>> refVec;
    for (auto *topLevelNode : ns.getTopLevelPackages()) {
        refVec.emplace_back(getWrappedNode(topLevelNode).get());
    }
    return refVec;
}

int NodeStorageDataProvider::numberOfPhysicalEntities() const
{
    // Count package groups
    auto topLvlPkgs = ns.getTopLevelPackages();
    auto counter = topLvlPkgs.size();
    for (auto *pkgGrp : topLvlPkgs) {
        // Count packages
        counter += pkgGrp->children().size();
        for (auto *pkg : pkgGrp->children()) {
            // Count components
            counter += pkg->children().size();
        }
    }
    return int(counter);
}

void CodegenAppAdapter::run(QWidget *parent, Codethink::lvtldr::NodeStorage& sharedNodeStorage)
{
    auto dataProvider = NodeStorageDataProvider{sharedNodeStorage};
    auto dialog = CodeGenerationDialog{dataProvider, nullptr, parent};

    dialog.setOutputDir(Preferences::self()->lastOutputDir());

    dialog.exec();

    Preferences::self()->setLastOutputDir(dialog.outputDir());
}

} // namespace Codethink::lvtcgn::app
