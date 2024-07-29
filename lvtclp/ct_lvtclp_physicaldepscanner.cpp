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
    std::filesystem::path d_prefix;
    std::filesystem::path d_buildFolder;
    const std::vector<std::filesystem::path>& d_nonLakosianDirs;
    const std::vector<std::pair<std::string, std::string>>& d_thirdPartyDirs;
    const std::vector<llvm::GlobPattern>& d_ignoreGlobs;

    std::function<void(const std::string&)> d_filenameCallback;
    std::optional<HeaderCallbacks::HeaderLocationCallback_f> d_headerLocationCallback;
    ThreadStringMap& d_pathToCanonical;
    bool d_enableLakosianRules;

  public:
    FrontendAction(lvtmdb::ObjectStore& memDb,
                   std::filesystem::path prefix,
                   std::filesystem::path buildFolder,
                   const std::vector<std::filesystem::path>& nonLakosians,
                   const std::vector<std::pair<std::string, std::string>>& thirdPartyDirs,
                   std::function<void(const std::string&)> filenameCallback,
                   const std::vector<llvm::GlobPattern>& ignoreGlobs,
                   std::optional<HeaderCallbacks::HeaderLocationCallback_f> headerLocationCallback,
                   ThreadStringMap& pathToCanonical,
                   bool enableLakosianRules):
        d_memDb(memDb),
        d_prefix(std::move(prefix)),
        d_buildFolder(buildFolder),
        d_nonLakosianDirs(nonLakosians),
        d_thirdPartyDirs(thirdPartyDirs),
        d_ignoreGlobs(ignoreGlobs),
        d_filenameCallback(std::move(filenameCallback)),
        d_headerLocationCallback(std::move(headerLocationCallback)),
        d_pathToCanonical(pathToCanonical),
        d_enableLakosianRules(enableLakosianRules)
    {
    }

    ~FrontendAction() noexcept override = default;

    void ExecuteAction() override
    {
        auto realPathStr = getCurrentFile().str();
        if (ClpUtil::isFileIgnored(realPathStr, d_ignoreGlobs)) {
            return;
        }

        clang::CompilerInstance& compiler = getCompilerInstance();
        clang::Preprocessor& pp = compiler.getPreprocessor();

        pp.addPPCallbacks(std::make_unique<lvtclp::HeaderCallbacks>(&compiler.getSourceManager(),
                                                                    d_memDb,
                                                                    d_prefix,
                                                                    d_buildFolder,
                                                                    d_nonLakosianDirs,
                                                                    d_thirdPartyDirs,
                                                                    d_ignoreGlobs,
                                                                    d_pathToCanonical,
                                                                    d_enableLakosianRules,
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
        if (ClpUtil::isFileIgnored(realPathStr, d_ignoreGlobs)) {
            return false;
        }

        return PreprocessOnlyAction::BeginSourceFileAction(ci);
    }
};

} // namespace

namespace Codethink::lvtclp {

struct DepScanActionFactory::Private {
    lvtmdb::ObjectStore& memDb;
    std::filesystem::path prefix;
    std::filesystem::path buildFolder;
    std::vector<std::filesystem::path> nonLakosianDirs;
    std::vector<std::pair<std::string, std::string>> thirdPartyDirs;
    std::function<void(const std::string&)> filenameCallback;
    std::vector<llvm::GlobPattern> ignoreGlobs;
    ThreadStringMap& pathToCanonical;
    std::optional<HeaderCallbacks::HeaderLocationCallback_f> headerLocationCallback;
    bool enableLakosianRules;

    Private(lvtmdb::ObjectStore& memDb,
            std::filesystem::path prefix,
            std::filesystem::path buildFolder,
            std::vector<std::filesystem::path> nonLakosians,
            std::vector<std::pair<std::string, std::string>> thirdPartyDirs,
            std::function<void(const std::string&)> filenameCallback,
            std::vector<llvm::GlobPattern> ignoreGlobs,
            ThreadStringMap& pathToCanonical,
            std::optional<HeaderCallbacks::HeaderLocationCallback_f> headerLocationCallback,
            bool enableLakosianRules):
        memDb(memDb),
        prefix(std::move(prefix)),
        buildFolder(buildFolder),
        nonLakosianDirs(std::move(nonLakosians)),
        thirdPartyDirs(std::move(thirdPartyDirs)),
        filenameCallback(std::move(filenameCallback)),
        ignoreGlobs(std::move(ignoreGlobs)),
        pathToCanonical(pathToCanonical),
        headerLocationCallback(std::move(headerLocationCallback)),
        enableLakosianRules(enableLakosianRules)
    {
    }
};

DepScanActionFactory::DepScanActionFactory(
    lvtmdb::ObjectStore& memDb,
    const std::filesystem::path& prefix,
    const std::filesystem::path& buildFolder,
    const std::vector<std::filesystem::path>& nonLakosians,
    const std::vector<std::pair<std::string, std::string>>& thirdPartyDirs,
    std::function<void(const std::string&)> filenameCallback,
    std::vector<llvm::GlobPattern> ignoreGlobs,
    ThreadStringMap& pathToCanonical,
    bool enableLakosianRules,
    std::optional<HeaderCallbacks::HeaderLocationCallback_f> headerLocationCallback):
    d(std::make_unique<DepScanActionFactory::Private>(memDb,
                                                      prefix,
                                                      buildFolder,
                                                      nonLakosians,
                                                      thirdPartyDirs,
                                                      std::move(filenameCallback),
                                                      std::move(ignoreGlobs),
                                                      pathToCanonical,
                                                      std::move(headerLocationCallback),
                                                      enableLakosianRules))
{
}

DepScanActionFactory::~DepScanActionFactory() noexcept = default;

std::unique_ptr<clang::FrontendAction> DepScanActionFactory::create()
{
    return std::make_unique<FrontendAction>(d->memDb,
                                            d->prefix,
                                            d->buildFolder,
                                            d->nonLakosianDirs,
                                            d->thirdPartyDirs,
                                            d->filenameCallback,
                                            d->ignoreGlobs,
                                            d->headerLocationCallback,
                                            d->pathToCanonical,
                                            d->enableLakosianRules);
}

} // namespace Codethink::lvtclp
