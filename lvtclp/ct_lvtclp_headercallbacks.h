// lvtclp_headercallbacks.h                                           -*-C++-*-

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

#ifndef INCLUDED_CT_LVTCLP_HEADERCALLBACKS
#define INCLUDED_CT_LVTCLP_HEADERCALLBACKS

//@PURPOSE: Add files to the database as they are touched by clang
//
//@CLASSES: lvtclp::HeaderCallbacks Implements clang::PPCallbacks
//
//@SEE_ALSO: clang::PPCallbacks

#include <ct_lvtmdb_fileobject.h>
#include <ct_lvtmdb_objectstore.h>

// std
#include <filesystem>
#include <functional>
#include <memory>
#include <vector>

// llvm
#include <llvm/Support/CommandLine.h>

// clang
#include <clang/Basic/Version.h>
#include <clang/Lex/PPCallbacks.h>

// Qt
#include <QString>

namespace Codethink {

// FORWARD DECLARATIONS

namespace lvtclp {

// =====================
// class HeaderCallbacks
// =====================

class HeaderCallbacks : public clang::PPCallbacks {
    // Implements clang::PPCallbacks. These callbacks make sure that new files
    // are added to the database as they are processed by clang
    // Not for use outside lvtclp

    // DATA
    lvtmdb::FileObject *d_sourceFile_p = nullptr;
    // Stores the source file currently processed by clang so that we know
    // where to add includes

    clang::SourceManager& sourceManager;
    // Clang context providing the filename

    lvtmdb::ObjectStore& d_memDb;
    // The active in-memory database session

    std::filesystem::path d_prefix;

    std::vector<std::filesystem::path> d_nonLakosianDirs;
    std::vector<std::pair<std::string, std::string>> d_thirdPartyDirs;
    std::vector<std::string> d_ignoreGlobs;

  public:
    // CREATORS
    HeaderCallbacks(clang::SourceManager *sm,
                    lvtmdb::ObjectStore& memDb,
                    std::filesystem::path prefix,
                    std::vector<std::filesystem::path> nonLakosians,
                    std::vector<std::pair<std::string, std::string>> thirdPartyDirs,
                    std::vector<std::string> ignoreGlobs);

    // MANIPULATORS
    void InclusionDirective(clang::SourceLocation HashLoc,
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
                            clang::SrcMgr::CharacteristicKind FileType) override;
    // Invoked when clang processes an #include directive

    void FileChanged(clang::SourceLocation sourceLocation,
                     FileChangeReason reason,
                     clang::SrcMgr::CharacteristicKind fileType,
                     clang::FileID prevFID) override;
    // Invoked whenever a source file is entered or exited by clang
};

} // namespace lvtclp

} // end namespace Codethink

#endif
