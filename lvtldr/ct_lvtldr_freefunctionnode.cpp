// ct_lvtldr_freefunctionnode.cpp                                            -*-C++-*-

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

#include <ct_lvtldr_freefunctionnode.h>
#include <ct_lvtldr_nodestorage.h>

namespace Codethink::lvtldr {

using namespace lvtshr;

// ==========================
// class TypeNode
// ==========================

FreeFunctionNode::FreeFunctionNode(NodeStorage& store): LakosianNode(store, std::nullopt)
{
    // Only to be used on tests
}

FreeFunctionNode::FreeFunctionNode(NodeStorage& store,
                                   std::optional<std::reference_wrapper<DatabaseHandler>> dbHandler,
                                   std::optional<FreeFunctionNodeFields> fields):
    LakosianNode(store, dbHandler), d_dbHandler(dbHandler), d_fields(*fields)
{
    setName(d_fields.name);
    d_qualifiedNameParts = NamingUtils::buildQualifiedNamePrefixParts(d_fields.qualifiedName, "::");
}

FreeFunctionNode::~FreeFunctionNode() noexcept = default;

lvtshr::DiagramType FreeFunctionNode::type() const
{
    return lvtshr::DiagramType::FreeFunctionType;
}

void FreeFunctionNode::loadParent()
{
    d->parentLoaded = true;
    d->parent = d->store.findById({DiagramType::ComponentType, *d_fields.componentId});
}

std::string FreeFunctionNode::qualifiedName() const
{
    return NamingUtils::buildQualifiedName(d_qualifiedNameParts, name(), "::");
}

long long FreeFunctionNode::id() const
{
    return d_fields.id;
}

lvtshr::UniqueId FreeFunctionNode::uid() const
{
    return {lvtshr::DiagramType::FreeFunctionType, id()};
}

std::string FreeFunctionNode::parentName()
{
    // TODO: Get parent name (minor)
    return "";
}

LakosianNode::IsLakosianResult FreeFunctionNode::isLakosian()
{
    return LakosianNode::IsLakosianResult::IsLakosian;
}

} // namespace Codethink::lvtldr
