// ct_lvtclp_physicaldepscanner.cpp                                  -*-C++-*-

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

#include "ct_lvtclp_cpp_tool_constants.h"
#include "ct_lvtclp_headercallbacks.h"
#include <ct_lvtclp_clputil.h>
#include <ct_lvtclp_physicaldepscanner.h>

#include <ct_lvtclp_diagnostic_consumer.h>

#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendActions.h>

namespace {

using namespace Codethink;
using namespace Codethink::lvtclp;

class FrontendAction : public clang::PreprocessOnlyAction {
    // Per-thread processing using only the clang preprocessor

  private:
    lvtmdb::ObjectStore& d_memDb;
    const CppToolConstants& d_constants;
    std::function<void(const std::string&)> d_filenameCallback;
    std::optional<HeaderCallbacks::HeaderLocationCallback_f> d_headerLocationCallback;
    ThreadStringMap& d_pathToCanonical;

  public:
    FrontendAction(lvtmdb::ObjectStore& memDb,
                   const CppToolConstants& constants,
                   std::function<void(const std::string&)> filenameCallback,
                   std::optional<HeaderCallbacks::HeaderLocationCallback_f> headerLocationCallback,
                   ThreadStringMap& pathToCanonical):
        d_memDb(memDb),
        d_constants(constants),
        d_filenameCallback(std::move(filenameCallback)),
        d_headerLocationCallback(std::move(headerLocationCallback)),
        d_pathToCanonical(pathToCanonical)
    {
    }

    ~FrontendAction() noexcept override = default;

    void ExecuteAction() override
    {
        auto realPathStr = getCurrentFile().str();
        if (ClpUtil::isFileIgnored(realPathStr, d_constants.ignoreGlobs)) {
            return;
        }

        clang::CompilerInstance& compiler = getCompilerInstance();
        clang::Preprocessor& pp = compiler.getPreprocessor();

        pp.addPPCallbacks(std::make_unique<lvtclp::HeaderCallbacks>(&compiler.getSourceManager(),
                                                                    d_memDb,
                                                                    d_constants,
                                                                    d_pathToCanonical,
                                                                    d_headerLocationCallback));

        // try our best not to bail on compilation errors
        clang::FrontendOptions& fOpts = compiler.getFrontendOpts();
        fOpts.FixWhatYouCan = true;
        fOpts.FixAndRecompile = true;
        fOpts.FixToTemporaries = true;
        // skip parsing function bodies
        fOpts.SkipFunctionBodies = true;

        // Attempt to obliterate any -Werror options
        clang::DiagnosticOptions& dOpts = compiler.getDiagnosticOpts();
        dOpts.Warnings = {"-Wno-everything"};

        PreprocessOnlyAction::ExecuteAction();
    }

    bool BeginSourceFileAction(clang::CompilerInstance& ci) override
    {
        d_filenameCallback(getCurrentFile().str());

        auto realPathStr = getCurrentFile().str();
        if (ClpUtil::isFileIgnored(realPathStr, d_constants.ignoreGlobs)) {
            return false;
        }

        return PreprocessOnlyAction::BeginSourceFileAction(ci);
    }
};

} // namespace

namespace Codethink::lvtclp {

struct DepScanActionFactory::Private {
    lvtmdb::ObjectStore& memDb;
    std::function<void(const std::string&)> filenameCallback;
    ThreadStringMap& pathToCanonical;
    std::optional<HeaderCallbacks::HeaderLocationCallback_f> headerLocationCallback;
    const CppToolConstants constants;

    Private(lvtmdb::ObjectStore& memDb,
            const CppToolConstants& constants,
            std::function<void(const std::string&)> filenameCallback,
            ThreadStringMap& pathToCanonical,
            std::optional<HeaderCallbacks::HeaderLocationCallback_f> headerLocationCallback):
        memDb(memDb),
        filenameCallback(std::move(filenameCallback)),
        pathToCanonical(pathToCanonical),
        headerLocationCallback(std::move(headerLocationCallback)),
        constants(constants)
    {
    }
};

DepScanActionFactory::DepScanActionFactory(
    lvtmdb::ObjectStore& memDb,
    const CppToolConstants& constants,
    std::function<void(const std::string&)> filenameCallback,
    ThreadStringMap& pathToCanonical,
    std::optional<HeaderCallbacks::HeaderLocationCallback_f> headerLocationCallback):
    d(std::make_unique<DepScanActionFactory::Private>(
        memDb, constants, std::move(filenameCallback), pathToCanonical, std::move(headerLocationCallback)))
{
}

DepScanActionFactory::~DepScanActionFactory() noexcept = default;

std::unique_ptr<clang::FrontendAction> DepScanActionFactory::create()
{
    return std::make_unique<FrontendAction>(d->memDb,
                                            d->constants,
                                            d->filenameCallback,
                                            d->headerLocationCallback,
                                            d->pathToCanonical);
}

} // namespace Codethink::lvtclp
