// ct_lvtclp_componentutil.h                                          -*-C++-*-

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

#ifndef INCLUDED_CT_LVTCLP_COMPONENTUTIL
#define INCLUDED_CT_LVTCLP_COMPONENTUTIL

//@PURPOSE: Derive component information from files in a database

#include <lvtclp_export.h>

#include <filesystem>

namespace Codethink::lvtmdb {
class ComponentObject;
}
namespace Codethink::lvtmdb {
class ObjectStore;
}
namespace Codethink::lvtmdb {
class PackageObject;
}

namespace Codethink::lvtclp {

struct LVTCLP_EXPORT ComponentUtil {
  public:
    // CLASS METHODS
    static lvtmdb::ComponentObject *
    addComponent(const std::filesystem::path& filePath, lvtmdb::PackageObject *parent, lvtmdb::ObjectStore& memDb);
    // If one is not already present, add a component for the file
    // Assumes that memDb is already locked for writing
    // NOTE: this does not look for other files which might be in the
    //       component
};

} // namespace Codethink::lvtclp

#endif // INCLUDED_CT_LVTCLP_COMPONENTUTIL
