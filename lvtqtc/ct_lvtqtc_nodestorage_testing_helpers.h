// ct_lvtqtc_nodestorage_testing_helpers.h                            -*-C++-*-

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

#ifndef INCLUDED_CT_LVTQTC_NS_TESTING_HELPERS_H
#define INCLUDED_CT_LVTQTC_NS_TESTING_HELPERS_H

#include <ct_lvtldr_nodestorage.h>
#include <ct_lvtqtc_componententity.h>
#include <ct_lvtqtc_packagedependency.h>
#include <ct_lvtqtc_packageentity.h>
#include <ct_lvtshr_loaderinfo.h>

#include <catch2-local-includes.h>

#include <optional>
#include <string>

struct ScopedPackageEntity {
    ScopedPackageEntity(Codethink::lvtldr::NodeStorage& ns,
                        std::string const& name,
                        std::optional<std::reference_wrapper<Codethink::lvtqtc::PackageEntity>> parent = std::nullopt):
        d_ns(ns)
    {
        auto result = ns.addPackage(name, name, parent ? parent.value().get().internalNode() : nullptr);
        if (result.has_error()) {
            FAIL("Error adding package: code " + std::to_string(static_cast<int>(result.error().kind)));
        }
        auto *pkg = result.value();
        d_value = std::make_unique<Codethink::lvtqtc::PackageEntity>(pkg, Codethink::lvtshr::LoaderInfo{});
    }

    ~ScopedPackageEntity()
    {
        auto *internalNode = d_value->internalNode();
        d_value.reset();
        auto result = d_ns.removePackage(internalNode);
        if (result.has_error()) {
            FAIL("Error removing package: code " + std::to_string(static_cast<int>(result.error().kind)));
        }
    }

    inline Codethink::lvtqtc::PackageEntity& value()
    {
        return *d_value;
    }

  private:
    Codethink::lvtldr::NodeStorage& d_ns;
    std::unique_ptr<Codethink::lvtqtc::PackageEntity> d_value;
};

struct ScopedComponentEntity {
    ScopedComponentEntity(Codethink::lvtldr::NodeStorage& ns,
                          std::string const& name,
                          Codethink::lvtqtc::PackageEntity& parent):
        d_ns(ns)
    {
        auto result = ns.addComponent(name, name, parent.internalNode());
        if (result.has_error()) {
            FAIL("Error adding component: code " + std::to_string(static_cast<int>(result.error().kind)));
        }
        auto *component = result.value();
        d_value = std::make_unique<Codethink::lvtqtc::ComponentEntity>(component, Codethink::lvtshr::LoaderInfo{});
    }

    ~ScopedComponentEntity()
    {
        auto *internalNode = d_value->internalNode();
        d_value.reset();
        auto result = d_ns.removeComponent(internalNode);
        if (result.has_error()) {
            FAIL("Error removing component: code " + std::to_string(static_cast<int>(result.error().kind)));
        }
    }

    inline Codethink::lvtqtc::ComponentEntity& value()
    {
        return *d_value;
    }

  private:
    Codethink::lvtldr::NodeStorage& d_ns;
    std::unique_ptr<Codethink::lvtqtc::ComponentEntity> d_value;
};

struct ScopedPackageDependency {
    ScopedPackageDependency(Codethink::lvtldr::NodeStorage& ns,
                            Codethink::lvtqtc::LakosEntity& source,
                            Codethink::lvtqtc::LakosEntity& target):
        d_ns(ns)
    {
        auto result = ns.addPhysicalDependency(source.internalNode(), target.internalNode());
        if (result.has_error()) {
            FAIL("Error adding dependency: code " + std::to_string(static_cast<int>(result.error().kind)));
        }
        d_value = std::make_unique<Codethink::lvtqtc::PackageDependency>(&source, &target);
    }

    ~ScopedPackageDependency()
    {
        d_ns.removePhysicalDependency(d_value->from()->internalNode(), d_value->to()->internalNode())
            .expect("Unexpected error removing physical dependency");
    }

    inline Codethink::lvtqtc::PackageDependency& value()
    {
        return *d_value;
    }

  private:
    Codethink::lvtldr::NodeStorage& d_ns;
    std::unique_ptr<Codethink::lvtqtc::PackageDependency> d_value;
};

#endif
