// ct_lvtqtc_util.h                                                    -*-C++-*-

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

#ifndef DEFINED_CT_LVTQTC_UTIL
#define DEFINED_CT_LVTQTC_UTIL

#include <lvtqtc_export.h>

#include <QGraphicsItem>

namespace Codethink::lvtqtc {

struct LVTQTC_EXPORT QtcUtil {
    constexpr static bool isDebugMode()
    {
#ifndef NDEBUG
        return true;
#else
        return false;
#endif
    }

    enum {
        LAKOSENTITY_TYPE = QGraphicsItem::UserType + 1,
        LAKOSRELATION_TYPE,
    };

    enum {
        e_INFORMATION = 100,
        e_NODE_HOVER_LAYER = 40,
        e_NODE_SELECTED_LAYER = 30,
        e_EDGE_HIGHLIGHED_LAYER = 20,
        e_EDGE_LAYER = 10,
        e_NODE_LAYER = 1,
    };

    enum class CreateUndoAction : short {
        e_No,
        e_Yes,
    };

    enum class VisibilityMode : short {
        e_Visible,
        e_Hidden,
    };

    enum class UndoActionType : short { e_Add, e_Remove };
};

} // namespace Codethink::lvtqtc

#endif // DEFINED_CT_LVTQTC_UTIL
