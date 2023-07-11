// ct_lvtshr_functional.h                                         -*-C++-*-

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
#ifndef INCLUDED_CT_LVTSHR_FUNCTIONAL
#define INCLUDED_CT_LVTSHR_FUNCTIONAL

#include <lvtshr_export.h>

#include <functional>

namespace Codethink::lvtshr {

struct LVTSHR_EXPORT ScopeExit {
    /* Calls the lambda on exit, simplifying management of code
     * that needs to be called whenever we quit a function, but that
     * we can't RAII
     */

    explicit ScopeExit(std::function<void(void)> f): f_(std::move(f))
    {
    }
    ~ScopeExit()
    {
        f_();
    }

  private:
    std::function<void(void)> f_;
};

} // namespace Codethink::lvtshr

#endif
