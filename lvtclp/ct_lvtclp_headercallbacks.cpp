// lvtclp_headercallbacks.cpp                                         -*-C++-*-

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

#include <ct_lvtclp_headercallbacks.h>

#include <ct_lvtclp_clputil.h>

#include <ct_lvtmdb_componentobject.h>
#include <ct_lvtmdb_fileobject.h>
#include <ct_lvtmdb_objectstore.h>
#include <ct_lvtmdb_packageobject.h>

#include <clang/Basic/FileManager.h>
#include <clang/Basic/Version.h>

#include <filesystem>
#include <utility>

namespace Codethink::lvtclp {

HeaderCallbacks::HeaderCallbacks(clang::SourceManager *sm,
                                 lvtmdb::ObjectStore& memDb,
                                 std::filesystem::path prefix,
                                 std::vector<std::filesystem::path> nonLakosians,
                                 std::vector<std::pair<std::string, std::string>> thirdPartyDirs,
                                 std::vector<llvm::GlobPattern> ignoreGlobs,
                                 std::optional<HeaderLocationCallback_f> headerLocationCallback):
    sourceManager(*sm),
    d_memDb(memDb),
    d_prefix(std::filesystem::weakly_canonical(prefix)),
    d_nonLakosianDirs(std::move(nonLakosians)),
    d_thirdPartyDirs(std::move(thirdPartyDirs)),
    d_ignoreGlobs(std::move(ignoreGlobs)),
    d_headerLocationCallback(std::move(headerLocationCallback))
{
}

void HeaderCallbacks::InclusionDirective(clang::SourceLocation HashLoc,
                                         const clang::Token& IncludeTok,
                                         clang::StringRef FileName,
                                         bool IsAngled,
                                         clang::CharSourceRange FilenameRange,
#if CLANG_VERSION_MAJOR >= 16
                                         clang::OptionalFileEntryRef File,
#elif CLANG_VERSION_MAJOR >= 15
                                         clang::Optional<clang::FileEntryRef> File,
#else
                                         const clang::FileEntry *File,
#endif
                                         clang::StringRef SearchPath,
                                         clang::StringRef RelativePath,
                                         const clang::Module *Imported,
                                         clang::SrcMgr::CharacteristicKind FileType)
{
#if CLANG_VERSION_MAJOR >= 16
    if (!File.has_value()) {
        return;
    }
#elif CLANG_VERSION_MAJOR >= 15
    if (!File.hasValue()) {
        return;
    }
#else
    if (File == nullptr) {
        return;
    }
#endif

    auto realPathStr =
#if CLANG_VERSION_MAJOR >= 15
        File.value().getFileEntry().tryGetRealPathName().str()
#else
        File->tryGetRealPathName().str()
#endif
        ;

    if (d_sourceFile_p == nullptr) {
        return;
    }

    if (ClpUtil::isFileIgnored(std::filesystem::path{realPathStr}.filename().string(), d_ignoreGlobs)) {
        return;
    }

    lvtmdb::FileObject *filePtr =
        ClpUtil::writeSourceFile(realPathStr, true, d_memDb, d_prefix, d_nonLakosianDirs, d_thirdPartyDirs);

    if (filePtr == d_sourceFile_p) {
        return;
    }

    if (d_headerLocationCallback) {
        auto sourceFile = std::string{};
        d_sourceFile_p->withROLock([&]() {
            sourceFile = d_sourceFile_p->name();
        });
        auto includedFile = FileName.str();
        auto lineNo = sourceManager.getExpansionLineNumber(HashLoc);
        (*d_headerLocationCallback)(sourceFile, includedFile, lineNo);
    }

    // add include relationship d_sourceFile_p -> filePtr
    assert(filePtr);
    assert(d_sourceFile_p);
    lvtmdb::FileObject::addIncludeRelation(d_sourceFile_p, filePtr);

    lvtmdb::ComponentObject *sourceComp = nullptr;
    lvtmdb::PackageObject *sourcePkg = nullptr;
    d_sourceFile_p->withROLock([&sourceComp, &sourcePkg, this]() {
        sourceComp = d_sourceFile_p->component();
        sourcePkg = d_sourceFile_p->package();
    });

    lvtmdb::ComponentObject *targetComp = nullptr;
    lvtmdb::PackageObject *targetPkg = nullptr;
    filePtr->withROLock([&targetComp, &targetPkg, &filePtr]() {
        targetComp = filePtr->component();
        targetPkg = filePtr->package();
    });

    if (sourceComp && targetComp && sourceComp != targetComp) {
        lvtmdb::ComponentObject::addDependency(sourceComp, targetComp);
    }

    if (sourcePkg && targetPkg && sourcePkg != targetPkg) {
        lvtmdb::PackageObject::addDependency(sourcePkg, targetPkg);

        lvtmdb::PackageObject *sourceParent = nullptr;
        lvtmdb::PackageObject *targetParent = nullptr;
        sourcePkg->withROLock([&sourcePkg, &sourceParent]() {
            sourceParent = sourcePkg->parent();
        });
        targetPkg->withROLock([&targetPkg, &targetParent]() {
            targetParent = targetPkg->parent();
        });

        if (sourceParent == nullptr) {
            // Source package is a standalone package. It is necessary to register the dependency to the package group
            if (targetParent != nullptr) {
                lvtmdb::PackageObject::addDependency(sourcePkg, targetParent);
            }
        }

        if (targetParent == nullptr) {
            // Target package is a standalone package. It is necessary to register the dependency from the package group
            if (sourceParent != nullptr) {
                lvtmdb::PackageObject::addDependency(sourceParent, targetPkg);
            }
        }
    }
}

void HeaderCallbacks::FileChanged(clang::SourceLocation sourceLocation,
                                  FileChangeReason reason,
                                  clang::SrcMgr::CharacteristicKind fileType,
                                  clang::FileID prevFID)
{
    const std::string realPath = ClpUtil::getRealPath(sourceLocation, sourceManager);

    if (ClpUtil::isFileIgnored(std::filesystem::path{realPath}.filename().string(), d_ignoreGlobs)) {
        d_sourceFile_p = nullptr;
        return;
    }

    const FileType type = ClpUtil::categorisePath(realPath);
    bool isHeader = false;
    if (type == FileType::e_Header) {
        isHeader = true;
    }

    d_sourceFile_p =
        ClpUtil::writeSourceFile(realPath, isHeader, d_memDb, d_prefix, d_nonLakosianDirs, d_thirdPartyDirs);
}

} // end namespace Codethink::lvtclp
