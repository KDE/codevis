// ct_lvtclp_diagnostic_consumer.cpp                                  -*-C++-*-

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
#include <ct_lvtmdb_objectstore.h>
#include <ct_lvtmdb_util.h>

#include <iostream>
#include <memory>
#include <thread>

#include <llvm/ADT/SmallString.h>
#include <llvm/Support/raw_ostream.h>

#include <clang/Basic/LangOptions.h>
#include <clang/Frontend/TextDiagnostic.h>

using namespace Codethink::lvtclp;

struct DiagnosticConsumer::Private {
    lvtmdb::ObjectStore& memDb;
    std::function<void(std::string, long)> messageCallback;

    clang::LangOptions langOpts;

    explicit Private(lvtmdb::ObjectStore& memDb, std::function<void(std::string, long)> messageCallback):
        memDb(memDb), messageCallback(std::move(messageCallback))
    {
    }
};

DiagnosticConsumer::DiagnosticConsumer(lvtmdb::ObjectStore& memDb,
                                       std::function<void(std::string, long)> messageCallback):
    d(std::make_unique<Private>(memDb, std::move(messageCallback)))
{
}

DiagnosticConsumer::~DiagnosticConsumer() = default;

void DiagnosticConsumer::HandleDiagnostic(clang::DiagnosticsEngine::Level DiagLevel, const clang::Diagnostic& Info)
{
    // maintain error and warning counts
    clang::DiagnosticConsumer::HandleDiagnostic(DiagLevel, Info);

    using Level = clang::DiagnosticsEngine::Level;

    // don't print warnings - the output is already too busy
    if (DiagLevel != Level::Error && DiagLevel != Level::Fatal) {
        return;
    }

    llvm::SmallString<1024> message;
    Info.FormatDiagnostic(message);

    long threadId = static_cast<long>(std::hash<std::thread::id>{}(std::this_thread::get_id()));

    assert(Info.hasSourceManager());

    std::string out;
    llvm::raw_string_ostream ss(out);
    clang::TextDiagnostic textDiag(ss, d->langOpts, new clang::DiagnosticOptions());

    textDiag.emitDiagnostic(clang::FullSourceLoc(Info.getLocation(), Info.getSourceManager()),
                            DiagLevel,
                            message,
                            Info.getRanges(),
                            Info.getFixItHints());

    d->messageCallback(ss.str(), threadId);

    d->memDb.withRWLock([&] {
        d->memDb.getOrAddError(
            lvtmdb::MdbUtil::ErrorKind::CompilerError,
            "", // There's no need to store the qualified name for a compiler error. we don't know the qualified name.
            ss.str(),
            Info.getLocation().printToString(Info.getSourceManager()));
    });
}
