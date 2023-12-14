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

#ifndef CODEVIS_FORTRANCINTEROPUTILS_H
#define CODEVIS_FORTRANCINTEROPUTILS_H

#include <ct_lvtmdb_objectstore.h>
#include <lvtclp_export.h>

namespace Codethink::lvtclp::fortran {

LVTCLP_EXPORT void solveFortranToCInteropDeps(Codethink::lvtmdb::ObjectStore& sharedMemDb);

}

#endif // CODEVIS_FORTRANCINTEROPUTILS_H
