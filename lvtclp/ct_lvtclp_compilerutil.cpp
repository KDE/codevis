// ct_lvtclp_compilerutil.cpp                                         -*-C++-*-

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

#ifdef Q_OS_LINUX
#include <bits/types/struct_timeval.h>
#endif

#include <ct_lvtclp_compilerutil.h>
#include <ct_lvtshr_stringhelpers.h>

#include <QDebug>
#include <filesystem>
#include <initializer_list>
#include <iostream>
#include <optional>

#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/Tooling.h>

namespace {

std::string trimLeadingSpaces(const std::string& str)
// remove leading spaces from str
{
    std::size_t loc = str.find_first_not_of(' ');
    return str.substr(loc);
}

std::optional<std::vector<std::string>> tryCompiler(const std::string& compiler)
{
    std::cout << "Testing with compiler " << compiler << std::endl;
    std::optional<std::string> compilerOut = Codethink::lvtclp::CompilerUtil::runCompiler(compiler);
    if (!compilerOut) {
        return {};
    }

    bool inIncludeSearchBlock = false;
    const char *blockStart = "#include <...> search starts here:";
    const char *blockEnd = "End of search list.";

    std::vector<std::string> includes;

    // now collect each line between blockStart and blockEnd
    std::stringstream ss(*compilerOut);
    std::string line;
    while (std::getline(ss, line)) {
        if (line == blockEnd) {
            break;
        }

        if (inIncludeSearchBlock) {
            includes.push_back(trimLeadingSpaces(line));
        }

        if (line == blockStart) {
            inIncludeSearchBlock = true;
        }
    }

    return includes;
}

std::vector<std::string> findLinuxIncludes(const std::string& compileCommandCompiler)
// run tryCompiler on clang++ or g++ with various version suffixes,
// returning the first successful result
{
    // First try the compiler specified by the compile commands.
    std::optional<std::vector<std::string>> res = tryCompiler(compileCommandCompiler);
    if (res) {
        return std::move(*res);
    }

    // lower versions first because hopefully things are backwards compatible
    std::initializer_list<std::string> suffixes = {
        "-9",
        "-10",
        "-11",
        "-12",
        "-13",
        "-14",
        "-15",
        "-16",
        "-17",
        "-18",
        "-19",
        "-20",
        "",
    };

    for (const char *compiler : {"clang++", "g++"}) {
        for (const std::string& suffix : suffixes) {
            const std::string fullName = compiler + suffix;
            std::optional<std::vector<std::string>> res = tryCompiler(fullName);
            if (res) {
                return std::move(*res);
            }
        }
    }

    return {};
}

} // unnamed namespace

namespace Codethink::lvtclp {

std::vector<std::string> CompilerUtil::findSystemIncludes(const std::string& compileCommandCompiler)
{
#ifndef Q_OS_WINDOWS
    return findLinuxIncludes(compileCommandCompiler);
#else
    std::ignore = compileCommandCompiler;
    return {};
#endif
}

std::optional<std::string> CompilerUtil::runCompiler(const std::string& compiler)
// attempt to run something like
//      compiler -E -v - </dev/null 1>/dev/null 2>out
// if compiler isn't in path, return {}
// if compiler doesn't return EXIT_SUCCESS, return {}
// otherwise return the contents of stderr
{
#ifdef Q_OS_WINDOWS
    return {};
#else
    // The popen() function shall execute the command specified by the string
    // command. It shall create a pipe between
    // the calling program and the executed command, and shall return a pointer
    // to a stream that can be used to either
    // read from or write to the pipe.
    // https://pubs.opengroup.org/onlinepubs/009696699/functions/popen.html
    const auto compile_call = compiler + " -E -v -x c++ - </dev/null 2>&1";
    FILE *fp = popen(compile_call.c_str(), "r");
    if (fp == nullptr) {
        return {};
    }
    auto constexpr READ_SIZE = 1024;
    char buffer[READ_SIZE];
    timeval timeout;

    std::string result;

    int fd = fileno(fp);
    bool eof = false;
    fd_set fdset;

    while (!eof) {
        timeout.tv_sec = 10;
        timeout.tv_usec = 0;
        FD_ZERO(&fdset);
        FD_SET(fd, &fdset);
        int rc = select(fd + 1, &fdset, 0, 0, &timeout);
        switch (rc) {
        // Error
        case -1: {
            std::cout << "Error reading file descriptor.\n";
            pclose(fp);
            return {};
        }
        // Timeout reading the file, we waited for 10s.
        // try again.
        case 0: {
            continue;
        }
        // Success
        default: {
            int read_bytes = read(fd, buffer, sizeof(buffer));
            if (read_bytes > 0) {
                result += buffer;
            } else if (read_bytes < 0) {
                std::cout << "Error while reading the file descriptor:errno = " << errno << "\n";
                eof = true;
            } else if (read_bytes == 0) {
                eof = true;
            }
        }
        }
    }

    /*
    while (fgets(output, READ_SIZE, fp) != nullptr) {
        result += output;
    }
    */

    // The pclose() function shall close a stream that was opened by popen(), wait for the command to terminate, and
    // return the termination status of the process that was running the command language interpreter. However, if a
    // call caused the termination status to be unavailable to pclose(), then pclose() shall return -1 with errno set
    // to [ECHILD] to report this situation.
    // https://pubs.opengroup.org/onlinepubs/009696699/functions/pclose.html
    auto status = pclose(fp);
    if (status != 0) {
        return {};
    }

    return result;
#endif
}

std::optional<std::string> CompilerUtil::findBundledHeaders(bool silent)
{
#ifdef __APPLE__
    // the bundled headers are broken on MacOS. Fortunately we have working
    // system includes
    return {};
#endif

#ifdef CT_CLANG_HEADERS_RELATIVE_DIR
    // CT_LVT_BINDIR should contain the directory in which argv[0] is located.
    // CT_CLANG_HEADERS_RELATIVE_DIR should contain a relative path from that
    // directory to the installed copy of the clang tooling headers
    const char *env_p = std::getenv("CT_LVT_BINDIR");
    if (env_p) {
        auto env = std::string{env_p};
        const std::filesystem::path binDir(env);
        std::filesystem::path headersDir = binDir / CT_CLANG_HEADERS_RELATIVE_DIR;
        if (std::filesystem::is_regular_file(headersDir / "stddef.h")) {
            if (!silent) {
                qDebug() << "Found clang header dir: " << headersDir.string();
            }
            return std::filesystem::canonical(headersDir).string();
        }

        // try appimage path
        headersDir = binDir / "lib" / "clang" / "include";
        if (std::filesystem::is_regular_file(headersDir / "stddef.h")) {
            if (!silent) {
                qDebug() << "Found clang header dir: " << headersDir.string();
            }
            return std::filesystem::canonical(headersDir).string();
        }
        if (!silent) {
            qDebug() << "WARNING: broken installation: cannot find clang headers at " << headersDir.string();
        }
    }
    if (!silent) {
        qDebug() << "Finding headers with heuristics - expect compilation failures";
    }
#endif // CT_CLANG_HEADERS_RELATIVE_DIR

    return {};
}

bool CompilerUtil::weNeedSystemHeaders()
// Figure out if we can already find system headers without modifying anything
{
    std::initializer_list<const char *> sources = {
        // stddef.h is the header mentioned in clang::tooling documentation
        "#include <stddef.h>",
        // weirdly MacOS finds stddef.h but then fails on stdarg.h so check
        // that too
        "#include <stdarg.h>",
    };

    for (const char *source : sources) {
        auto action = std::make_unique<clang::PreprocessOnlyAction>();

        // the tool will fail if it can't find the include
        bool ret = clang::tooling::runToolOnCode(std::move(action), source, "dummy.cpp");
        if (!ret) {
            return true;
        }
    }

    return false;
}

} // namespace Codethink::lvtclp
