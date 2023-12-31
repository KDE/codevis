// ct_lvtcgn_app_adapter.h                              -*-C++-*-

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

#ifndef _CT_LVTCGN_APPADAPTER_H
#define _CT_LVTCGN_APPADAPTER_H

#include <QWidget>
#include <ct_lvtcgn_generatecode.h>
#include <ct_lvtldr_lakosiannode.h>
#include <functional>
#include <lvtcgn_adapter_export.h>
#include <map>
#include <memory>
#include <optional>
#include <vector>

namespace Codethink::lvtcgn::app {

class NodeStorageDataProvider;

class LVTCGN_ADAPTER_EXPORT WrappedLakosianNode : public Codethink::lvtcgn::mdl::IPhysicalEntityInfo {
  public:
    WrappedLakosianNode(NodeStorageDataProvider& dataProvider, lvtldr::LakosianNode *node, bool isSelectedForCodegen);
    ~WrappedLakosianNode() override;
    std::string name() const override;
    std::string type() const override;
    std::optional<std::reference_wrapper<IPhysicalEntityInfo>> parent() const override;
    std::vector<std::reference_wrapper<IPhysicalEntityInfo>> children() const override;
    std::vector<std::reference_wrapper<IPhysicalEntityInfo>> fwdDependencies() const override;
    bool selectedForCodeGeneration() const override;
    void setSelectedForCodeGeneration(bool value) override;

  private:
    NodeStorageDataProvider& dataProvider;
    Codethink::lvtldr::LakosianNode *node;
    bool isSelectedForCodegen;
};

class LVTCGN_ADAPTER_EXPORT NodeStorageDataProvider : public Codethink::lvtcgn::mdl::ICodeGenerationDataProvider {
  public:
    explicit NodeStorageDataProvider(lvtldr::NodeStorage& sharedNodeStorage);
    NodeStorageDataProvider(NodeStorageDataProvider const& other) = delete;
    NodeStorageDataProvider(NodeStorageDataProvider&& other) = delete;
    ~NodeStorageDataProvider() override;
    [[nodiscard]] std::vector<std::reference_wrapper<mdl::IPhysicalEntityInfo>> topLevelEntities() override;
    [[nodiscard]] int numberOfPhysicalEntities() const override;

    friend class WrappedLakosianNode;

  private:
    [[nodiscard]] std::reference_wrapper<mdl::IPhysicalEntityInfo> getWrappedNode(lvtldr::LakosianNode *node);

    lvtldr::NodeStorage& ns;
    std::map<lvtldr::LakosianNode *, std::unique_ptr<mdl::IPhysicalEntityInfo>> nodeToInfo;
};

class LVTCGN_ADAPTER_EXPORT CodegenAppAdapter {
  public:
    static void run(QWidget *parent, Codethink::lvtldr::NodeStorage& sharedNodeStorage);
};

} // namespace Codethink::lvtcgn::app

#endif
