// ct_lvtclp_toolexecutor.h                                           -*-C++-*-

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

// much of this component is taken straight from clang source
// that source is licenced as Apache-2.0 with llvm-exception
// See https://llvm.org/LICENSE.txt
//     clang/include/clang/Tooling/AllTUsExecution.h

#ifndef INCLUDED_CT_LVTCLP_TOOLEXECUTOR
#define INCLUDED_CT_LVTCLP_TOOLEXECUTOR

//@PURPOSE: Re-implement clang::tooling::AllTUsToolExecutor using QThreadPool
//          This component borrows generously from the clang implementation
//
//@CLASSES:
//  lvtclp::ToolExecutor

#include <lvtclp_export.h>

#include <clang/Tooling/Execution.h>
#include <clang/Tooling/Tooling.h>

#include <functional>
#include <memory>

namespace Codethink::lvtmdb {
class ObjectStore;
}

namespace Codethink::lvtclp {

// =======================
// class ToolExecutor
// =======================

class LVTCLP_EXPORT ToolExecutor : public clang::tooling::ToolExecutor {
  private:
    // TYPES
    struct Private;
    class RunnableThread;

    // DATA
    std::unique_ptr<Private> d;

  public:
    // CREATORS
    ToolExecutor(const clang::tooling::CompilationDatabase& compDb,
                 unsigned threadCount,
                 std::function<void(std::string, long)> messageCallback,
                 lvtmdb::ObjectStore& memDb);

    ~ToolExecutor() noexcept override;

    // MANIPULATORS
    using clang::tooling::ToolExecutor::execute;

    llvm::Error execute(llvm::ArrayRef<std::pair<std::unique_ptr<clang::tooling::FrontendActionFactory>,
                                                 clang::tooling::ArgumentsAdjuster>> actions) override;

    clang::tooling::ExecutionContext *getExecutionContext() override;

    clang::tooling::ToolResults *getToolResults() override;

    void mapVirtualFile(llvm::StringRef filePath, llvm::StringRef content) override;

    void cancelRun();
    // Cancel mid-execute

    // ACCESSORS
    [[nodiscard]] llvm::StringRef getExecutorName() const override;
};

} // namespace Codethink::lvtclp

#endif // INCLUDED_CT_LVTCLP_TOOLEXECUTOR
