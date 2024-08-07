/*
// Copyright 2024Codethink Ltd <codethink@codethink.co.uk>
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

#include <ct_lvtclp_parse_error_handler.h>

// Auto generated by CMake / see main CMakeLists file, and
// the CMakeLists on this folder.
#include <catch2-local-includes.h>
#include <test-project-paths.h>

#include <QDebug>
#include <QTemporaryDir>
#include <QTemporaryFile>

using namespace Codethink::lvtclp;

TEST_CASE("Test Calling ParseErrorHandler")
{
    QTemporaryDir dir;
    QTemporaryFile compileCommandsTemp;
    compileCommandsTemp.open();
    compileCommandsTemp.write("pseudo-compile-commands");
    compileCommandsTemp.close();

    Codethink::lvtclp::ParseErrorHandler handler;

    // Simulate a small number of messages on two threads.
    handler.receivedMessage("Test Message", 1);
    handler.receivedMessage("Test Message 2", 2);

    Codethink::lvtclp::ParseErrorHandler::SaveOutputInputArgs args{
        .compileCommands = compileCommandsTemp.fileName().toStdString(),
        .outputPath = (dir.path() + "/test.zip").toStdString(),
        .ignorePattern = "",
    };

    auto res = handler.saveOutput(args);
    if (!res) {
        qDebug() << res.error();
    }

    REQUIRE_FALSE(res.has_error());
}