// ct_lvtqtc_lakosentitypluginutils.h                                -*-C++-*-

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

#ifndef DIAGRAM_SERVER_CT_LVTQTC_LAKOSENTITYPLUGINUTILS_H
#define DIAGRAM_SERVER_CT_LVTQTC_LAKOSENTITYPLUGINUTILS_H

#include <ct_lvtplg_plugindatatypes.h>
#include <ct_lvtqtc_lakosentity.h>

namespace Codethink::lvtqtc {

LVTQTC_EXPORT Entity createWrappedEntityFromLakosEntity(LakosEntity *e);
LVTQTC_EXPORT Edge createWrappedEdgeFromLakosEntity(LakosEntity *from, LakosEntity *to);
}

#endif // DIAGRAM_SERVER_CT_LVTQTC_LAKOSENTITYPLUGINUTILS_H
