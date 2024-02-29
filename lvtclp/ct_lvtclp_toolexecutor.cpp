// ct_lvtclp_toolexecutor.cpp                                         -*-C++-*-

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

#include <ct_lvtclp_diagnostic_consumer.h>
#include <ct_lvtclp_toolexecutor.h>

#include <QRunnable>
#include <QThreadPool>

#include <llvm/Support/VirtualFileSystem.h>

#include <climits>
#include <iostream>
#include <mutex>
#include <utility>
#include <vector>

// much of this component is taken straight from clang source
// that source is licenced as Apache-2.0 with llvm-exception
// See https://llvm.org/LICENSE.txt
//     clang/lib/Tooling/AllTUsExecution.cpp

namespace {

int unsignedToInt(unsigned i)
{
    if (i > (unsigned) INT_MAX) {
        return INT_MAX;
    }
    return static_cast<int>(i);
}

llvm::Error makeError(const llvm::Twine& msg)
{
    return llvm::make_error<llvm::StringError>(msg, llvm::inconvertibleErrorCode());
}

class ThreadSafeToolResults : public clang::tooling::ToolResults {
  private:
    // DATA
    clang::tooling::InMemoryToolResults d_results;
    std::mutex d_mutex;

  public:
    // MODIFIERS
    void addResult(llvm::StringRef key, llvm::StringRef value) override
    {
        std::unique_lock<std::mutex> guard(d_mutex);
        d_results.addResult(key, value);
    }

    std::vector<std::pair<llvm::StringRef, llvm::StringRef>> AllKVResults() override
    {
        return d_results.AllKVResults();
    }

    void forEachResult(llvm::function_ref<void(llvm::StringRef key, llvm::StringRef value)> callback) override
    {
        d_results.forEachResult(callback);
    }
};

} // unnamed namespace

namespace Codethink::lvtclp {

struct ToolExecutor::Private {
    const clang::tooling::CompilationDatabase& compilations;

    ThreadSafeToolResults results;
    clang::tooling::ExecutionContext context;

    llvm::StringMap<std::string> overlayFiles;

    unsigned threadCount;

    std::function<void(std::string, long)> messageCallback;
    lvtmdb::ObjectStore& memDb;

    QThreadPool pool;

    Private(const clang::tooling::CompilationDatabase& compDb,
            unsigned threadCount,
            std::function<void(std::string, long)> messageCallback,
            lvtmdb::ObjectStore& memDb):
        compilations(compDb),
        context(&results),
        threadCount(threadCount),
        messageCallback(std::move(messageCallback)),
        memDb(memDb)
    {
    }
};

class ToolExecutor::RunnableThread : public QRunnable {
    // The ancient version of Qt in appimage (5.11) doesn't support creating a
    // QRunnable from a lambda so we have to write a lambda by hand here :(

  public:
    // PUBLIC TYPES
    using Action = std::pair<std::unique_ptr<clang::tooling::FrontendActionFactory>, clang::tooling::ArgumentsAdjuster>;

  private:
    // PRIVATE DATA
    const std::string d_file;
    ToolExecutor *d_executor;
    const Action& d_action;
    std::function<void(llvm::Twine)> d_appendError;
    std::function<void(std::string, long)> d_messageCallback;
    lvtmdb::ObjectStore& d_memDb;

  public:
    // CREATORS
    RunnableThread(std::string file,
                   ToolExecutor *executor,
                   const Action& action,
                   std::function<void(llvm::Twine)> appendError,
                   std::function<void(std::string, long)> messageCallback,
                   lvtmdb::ObjectStore& memDb):
        d_file(std::move(file)),
        d_executor(executor),
        d_action(action),
        d_appendError(std::move(appendError)),
        d_messageCallback(std::move(messageCallback)),
        d_memDb(memDb)
    {
        setAutoDelete(true);
    }

    ~RunnableThread() override = default;

    // MUTATORS
    void run() override
    {
        clang::tooling::ClangTool tool(d_executor->d->compilations,
                                       {d_file},
                                       std::make_shared<clang::PCHContainerOperations>(),
                                       llvm::vfs::createPhysicalFileSystem());
        auto consumer = lvtclp::DiagnosticConsumer(d_memDb, d_messageCallback);
        tool.setDiagnosticConsumer(&consumer);
        for (const auto& fileAndContent : d_executor->d->overlayFiles) {
            tool.mapVirtualFile(fileAndContent.first(), fileAndContent.second);
        }

        // 0 on success;
        // 1 if any error occurred;
        // 2 if there is no error but some files are skipped due to missing compile commands.
        try {
            auto ret = tool.run(d_action.first.get());

            if (ret == 1) {
                d_appendError(llvm::Twine("Failed to run action on ") + d_file + "\n");
            }
        }
        // on Windows, LLVM failed while parsing the owncloud desktop client
        // throwing an instance of std::out_of_range. The specific code is
        // inside of llvm and out of our control, so just ignore it.
        catch (std::out_of_range& e) {
            std::cout << "ERROR - Out of Range Access" << e.what() << "\n";
        }
    }
};

ToolExecutor::ToolExecutor(const clang::tooling::CompilationDatabase& compDb,
                           unsigned threadCount,
                           std::function<void(std::string, long)> messageCallback,
                           lvtmdb::ObjectStore& memDb):
    d(std::make_unique<Private>(compDb, threadCount, std::move(messageCallback), memDb))
{
}

ToolExecutor::~ToolExecutor() noexcept = default;

llvm::Error ToolExecutor::execute(
    llvm::ArrayRef<std::pair<std::unique_ptr<clang::tooling::FrontendActionFactory>, clang::tooling::ArgumentsAdjuster>>
        actions)
{
    if (actions.empty()) {
        return makeError("No action to execute.");
    }

    if (actions.size() != 1) {
        return makeError("Only support executing 1 action");
    }

    std::string errorMsg;
    std::mutex logMutex;
    auto appendError = [&errorMsg, &logMutex](llvm::Twine err) {
        std::unique_lock<std::mutex> lock(logMutex);
        errorMsg += err.str();
    };

    const auto& action = actions.front();

    {
        d->pool.setMaxThreadCount(unsignedToInt(d->threadCount));

        for (const std::string& file : d->compilations.getAllFiles()) {
            // deleted automatically by QThreadPool
            // see QRunnable::setAutoDelete
            QRunnable *runnable = new RunnableThread(file, this, action, appendError, d->messageCallback, d->memDb);
            d->pool.start(runnable);
        }

        // make sure all tasks have finished
        d->pool.waitForDone();
    }

    if (!errorMsg.empty()) {
        return makeError(errorMsg);
    }

    return llvm::Error::success();
}

clang::tooling::ExecutionContext *ToolExecutor::getExecutionContext()
{
    return &d->context;
}

clang::tooling::ToolResults *ToolExecutor::getToolResults()
{
    return &d->results;
}

void ToolExecutor::mapVirtualFile(llvm::StringRef filePath, llvm::StringRef content)
{
    d->overlayFiles[filePath] = std::string(content);
}

void ToolExecutor::cancelRun()
{
    d->pool.clear();
}

llvm::StringRef ToolExecutor::getExecutorName() const
{
    return "Codethink::lvtclp::ToolExecutor";
}

} // namespace Codethink::lvtclp
