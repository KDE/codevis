// ct_lvtldr_lakosiannode.cpp                                         -*-C++-*-

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

#include <ct_lvtldr_lakosiannode.h>

#include <ct_lvtldr_nodestorage.h>

#include <boost/algorithm/string.hpp>

#include <algorithm>
#include <cassert>
#include <filesystem>
#include <unordered_map>

using namespace Codethink::lvtshr;

namespace Codethink::lvtldr {

std::vector<std::string> NamingUtils::buildQualifiedNamePrefixParts(const std::string& qname,
                                                                    const std::string& separator)
{
    std::vector<std::string> qnameParts;
    boost::iter_split(qnameParts, qname, boost::first_finder(separator));
    qnameParts.pop_back();
    return qnameParts;
}

std::string NamingUtils::buildQualifiedName(const std::vector<std::string>& parts,
                                            const std::string& name,
                                            const std::string& separator)
{
    auto qname = std::string{};
    for (auto const& namePart : parts) {
        qname += namePart + separator;
    }
    qname += name;
    return qname;
}

// ==========================
// class LakosianNode
// ==========================

LakosianNode::LakosianNode(NodeStorage& store, std::optional<std::reference_wrapper<DatabaseHandler>> dbHandler):
    d(std::make_unique<Private>(store, dbHandler))
{
}

LakosianNode::~LakosianNode() noexcept = default;

LakosianNode::LakosianNode(LakosianNode&&) noexcept = default;

void LakosianNode::registerOnNameChanged(void *receiver, const std::function<void(LakosianNode *)>& callback)
{
#ifndef CT_SCANBUILD // scanbuild doesn't like boost
    auto connection = d->onNameChanged.connect(callback);
    d->connections[receiver].push_back(connection);
#endif
}

void LakosianNode::registerOnNotesChanged(void *receiver, const std::function<void(std::string)>& callback)
{
#ifndef CT_SCANBUILD // scanbuild doesn't like boost
    auto connection = d->onNotesChanged.connect(callback);
    d->connections[receiver].push_back(connection);
#endif
}

void LakosianNode::registerChildCountChanged(void *receiver, const std::function<void(size_t)>& callback)
{
#ifndef CT_SCANBUILD // scanbuild doesn't like boost
    auto connection = d->onChildCountChanged.connect(callback);
    d->connections[receiver].push_back(connection);
#endif
}

void LakosianNode::unregisterAllCallbacksTo(void *receiver)
{
    if (d->connections.count(receiver) == 0) {
        return;
    }

    for (const auto& c : d->connections.at(receiver)) {
        c.disconnect();
    }
    auto it = std::find_if(d->connections.begin(), d->connections.end(), [&receiver](const auto& e) {
        return e.first == receiver;
    });
    d->connections.erase(it);
}

bool LakosianNode::isPackageGroup()
{
    // default implementation
    return false;
}

std::string LakosianNode::notes() const
{
    if (d->dbHandler->get().hasNotes(uid())) {
        return d->dbHandler->get().getNotesFromId(uid());
    }
    return "";
}

LakosianNode *LakosianNode::parent()
{
    loadParent();
    assert(d->parentLoaded);
    return d->parent;
}

std::string LakosianNode::name() const
{
    return d->name;
}

void LakosianNode::setNotes(const std::string& notes)
{
    if (!d->dbHandler->get().hasNotes(uid())) {
        d->dbHandler->get().addNotes(uid(), notes);
    } else {
        d->dbHandler->get().setNotes(uid(), notes);
    }

    d->onNotesChanged(notes);
}

void LakosianNode::setName(const std::string& newName)
{
    d->name = newName;
    d->onNameChanged(this);
}

const std::vector<LakosianNode *>& LakosianNode::children()
{
    const auto currChildren = d->children.size();
    auto childrenWasLoaded = d->childrenLoaded;
    loadChildren();

    assert(d->childrenLoaded);
    if (currChildren != d->children.size() || !childrenWasLoaded) {
        d->onChildCountChanged(d->children.size());
    }

    return d->children;
}

const std::vector<LakosianEdge>& LakosianNode::providers()
{
    loadProviders();
    assert(d->providersLoaded);
    return d->providers;
}

void LakosianNode::invalidateProviders()
{
    // TODO [#455]: It is possible to update providers, instead of fetching all over again from database.
    d->providersLoaded = false;
    d->providers.clear();
}

void LakosianNode::invalidateChildren()
{
    d->childrenLoaded = false;
    d->children.clear();
}

void LakosianNode::invalidateParent()
{
    d->parentLoaded = false;
    d->parent = nullptr;
}

const std::vector<LakosianEdge>& LakosianNode::clients()
{
    loadClients();
    assert(d->clientsLoaded);
    return d->clients;
}

void LakosianNode::invalidateClients()
{
    // TODO [#455]: It is possible to update clients, instead of fetching all over again from database.
    d->clientsLoaded = false;
    d->clients.clear();
}

bool LakosianNode::hasProvider(LakosianNode *other)
{
    const auto& deps = providers();
    return std::find_if(std::cbegin(deps),
                        std::cend(deps),
                        [&](const auto& dep) {
                            return dep.other() == other;
                        })
        != std::cend(deps);
}

bool operator==(const LakosianNode& lhs, const LakosianNode& rhs)
{
    return lhs.type() == rhs.type() && lhs.id() == rhs.id();
}

std::vector<LakosianNode *> LakosianNode::parentHierarchy()
{
    // parent-hierarchy of the physical node.
    auto *tmp = this;
    std::vector<LakosianNode *> hierarchy;
    while (tmp) {
        hierarchy.push_back(tmp);
        tmp = tmp->parent();
    }
    std::reverse(std::begin(hierarchy), std::end(hierarchy));
    return hierarchy;
}

LakosianNode *LakosianNode::topLevelParent()
{
    LakosianNode *parentPtr = this;
    while (parentPtr) {
        if (!parentPtr->parent()) {
            break;
        }
        parentPtr = parentPtr->parent();
    }

    return parentPtr;
}

} // namespace Codethink::lvtldr
