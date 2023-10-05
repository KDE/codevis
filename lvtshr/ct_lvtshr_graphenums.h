// ct_lvtshr_graphenums.h                                         -*-C++-*-

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
#ifndef INCLUDED_CT_LVTSHR_GRAPHENUMS
#define INCLUDED_CT_LVTSHR_GRAPHENUMS

#include <lvtshr_export.h>

namespace Codethink::lvtshr {

// Defines how the load algorithm will handle the classes
enum ClassView { TraverseByRelation, TraverseAllPaths, ClassHierarchyGraph, ClassHierarchyTree };

/*! Type has a value for each type of LakosRelation */
enum LakosRelationType {
    IsA = 0x1, /*!< The IsA relation */
    PackageDependency = 0x2, /*!< The Package-Dependency relation */
    UsesInNameOnly = 0x4, /*!< The Uses-In-Name-Only relation */
    UsesInTheImplementation = 0x8, /*!< The Uses-In-The-Implementation relation */
    UsesInTheInterface = 0x10, /*!< The Uses-In-The-Interface relation */
    None = 0,
};

/*! ClassScope specifies the scope of the classes to be loaded */
enum class ClassScope {
    All, /*!< Load all classes  */
    PackageOnly, /*!< Load only classes in same package  */
    NamespaceOnly /*!< Load only classes in same namespace  */
};

enum class DiagramType {
    ClassType = 1,
    ComponentType = 10,
    PackageType = 100,
    RepositoryType = 1000,
    NoneType = 1001,
};

enum class AccessSpecifier {
    // AccessSpecifier A C++ access specifier (public, private, protected), plus the
    //  special value "none" which means different things in different contexts.

    e_public = 0, // public access
    e_protected = 1, // protected access
    e_private = 2, // private access
    e_none = 3, // undefined access
};

enum class UDTKind { Class, Enum, Struct, TypeAlias, Union, Unknown };

/*! Defines how searches on the Diagram should be handled. */
enum class SearchMode { CaseInsensitive, CaseSensitive, RegularExpressions };

} // end namespace Codethink::lvtshr

#endif
