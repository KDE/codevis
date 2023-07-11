// ct_lvtmdb_errorobject.h                                            -*-C++-*-

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

#ifndef INCLUDED_CT_LVTMDB_ERROROBJECT
#define INCLUDED_CT_LVTMDB_ERROROBJECT

#include <lvtmdb_export.h>

#include <ct_lvtmdb_databaseobject.h>
#include <ct_lvtmdb_util.h>

#include <string>

namespace Codethink::lvtmdb {

// ===================
// class ErrorObject
// ===================

class LVTMDB_EXPORT ErrorObject : public DatabaseObject {
    // See locking discipline in superclass. That applies here.

  private:
    MdbUtil::ErrorKind d_errorKind;
    // the type of error this refeers to.

    std::string d_errorMessage;
    // The stored error message

    std::string d_fileName;
    // The file containing the error

  public:
    // CREATORS
    ErrorObject(MdbUtil::ErrorKind errorKind,
                std::string qualifiedName,
                std::string errorMessage,
                std::string fileName);

    ~ErrorObject() noexcept override;

    ErrorObject(ErrorObject&& other) noexcept;

    ErrorObject& operator=(ErrorObject&& other) noexcept;

    // ACCESSORS
    [[nodiscard]] MdbUtil::ErrorKind errorKind() const;

    [[nodiscard]] const std::string& errorMessage() const;

    [[nodiscard]] const std::string& fileName() const;

    [[nodiscard]] std::string storageKey() const;
    // unique identification of this error message

    // CLASS METHODS
    static std::string
    getStorageKey(const std::string& qualifiedName, const std::string& errorMessage, const std::string& fileName);
};

} // namespace Codethink::lvtmdb

#endif // INCLUDED_CT_LVTMDB_ERROROBJECT
