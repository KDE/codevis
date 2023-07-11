// ct_lvtclp_diagnostic_consumer.h                                      -*-C++-*-

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

#ifndef INCLUDED_CT_LVTCLP_DIAGNOSTICS_CONSUMER
#define INCLUDED_CT_LVTCLP_DIAGNOSTICS_CONSUMER

//@PURPOSE: Get the errros from clang and processes them.
//
//@CLASSES: DiagnosticConsumer: Tells clang how it should emit error messages.
//
//@SEE_ALSO: clang::DiagnosticConsumer

#include <lvtclp_export.h>

#include <clang/Basic/Diagnostic.h>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>

namespace Codethink::lvtmdb {
class ObjectStore;
}

namespace Codethink::lvtclp {

// =============================
// class DiagnosticConsumer
// =============================

class LVTCLP_EXPORT DiagnosticConsumer : public clang::DiagnosticConsumer {
    // Class that tells clang how it should emit error messages.
  public:
    explicit DiagnosticConsumer(lvtmdb::ObjectStore& memDb, std::function<void(std::string, long)> messageCallback);
    ~DiagnosticConsumer() override;
    void HandleDiagnostic(clang::DiagnosticsEngine::Level DiagLevel, const clang::Diagnostic& Info) override;

  private:
    struct Private;
    std::unique_ptr<Private> d;
};

} // namespace Codethink::lvtclp

#endif
