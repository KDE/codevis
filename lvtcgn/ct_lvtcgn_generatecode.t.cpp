// ct_lvtcgn_generatecode.t.cpp                                       -*-C++-*-

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

#include <ct_lvtcgn_generatecode.h>
#include <ct_lvtcgn_testutils.h>

#include <catch2-local-includes.h>
#include <ct_lvttst_tmpdir.h>

#pragma push_macro("slots")
#undef slots
#include <pybind11/embed.h>
#include <pybind11/pybind11.h>
#pragma pop_macro("slots")

#include <fstream>
#include <string>

using namespace std::string_literals;
static const std::string TMPDIR_NAME = "tmp_ct_lvtcgn_generatecode";

using namespace Codethink::lvtcgn::mdl;

namespace py = pybind11;
struct PyDefaultGilReleasedContext {
    py::scoped_interpreter pyInterp;
    py::gil_scoped_release pyGilDefaultReleased;
};

TEST_CASE("Basic code generation")
{
    PyDefaultGilReleasedContext _pyDefaultGilReleasedContext;

    auto tmp_dir = TmpDir{TMPDIR_NAME};

    const std::string SCRIPT_CONTENTS = R"(
    let entities_processed = 0;

    function beforeProcessEntities(output_dir) {
        with open(output_dir + '/output.txt', 'a+') as f:
            f.write(f'BEFORE process entities called.\n')
        user_ctx['callcount'] = 0
    }

    function buildPhysicalEntity(entity, output_dir) {
        with open(output_dir + '/output.txt', 'a+') as f:
            f.write(f'({entity.name()}, {entity.type()});')
        user_ctx['callcount'] += 1
    }

    def afterProcessEntities(output_dir):
        user_ctx['callcount'] += 1
        with open(output_dir + '/output.txt', 'a+') as f:
            f.write(f'\nAFTER process entities called. {user_ctx["callcount"]}')
    )";

    auto scriptPath = tmp_dir.createTextFile("some_script.py", SCRIPT_CONTENTS);
    auto outputDir = tmp_dir.createDir("out");

    auto contentProvider = FakeContentProvider{};
    auto result = CodeGeneration::generateCodeFromjS(QString::fromStdString(scriptPath.string()),
                                                     QString::fromStdString(outputDir.string()),
                                                     contentProvider);
    if (result.has_error()) {
        INFO("Error message: " + result.error().message);
    }
    REQUIRE(!result.has_error());

    auto resultStream = std::ifstream(outputDir / "output.txt");
    auto output = std::string((std::istreambuf_iterator<char>(resultStream)), (std::istreambuf_iterator<char>()));
    REQUIRE(output == R"(BEFORE process entities called.
(somepkg_a, DiagramType.Package);(somepkg_c, DiagramType.Package);(component_b, DiagramType.Component);
AFTER process entities called. 4)");
}

TEST_CASE("Code generation errors")
{
    auto tmpDir = TmpDir{TMPDIR_NAME};
    auto contentProvider = FakeContentProvider{};

    // Provide a bad file
    {
        PyDefaultGilReleasedContext _pyDefaultGilReleasedContext;
        auto result = CodeGeneration::generateCodeFromjS("badfile.py", ".", contentProvider);
        REQUIRE(result.has_error());
        REQUIRE(result.error().message == "ModuleNotFoundError: No module named 'badfile'");
    }

    // Provide a good file, but no required function
    {
        PyDefaultGilReleasedContext _pyDefaultGilReleasedContext;
        const std::string SCRIPT_CONTENTS = R"(
def f(x):
    pass
)";
        auto scriptPath = tmpDir.createTextFile("some_script.py", SCRIPT_CONTENTS);

        auto result =
            CodeGeneration::generateCodeFromjS(QString::fromStdString(scriptPath.string()), ".", contentProvider);
        REQUIRE(result.has_error());
        REQUIRE(result.error().message == "Expected function named buildPhysicalEntity");
    }

    // Python code invalid syntax
    {
        PyDefaultGilReleasedContext _pyDefaultGilReleasedContext;
        const std::string SCRIPT_CONTENTS = R"(
def f(x):
    nonsense code
)";
        auto scriptPath = tmpDir.createTextFile("some_script.py", SCRIPT_CONTENTS);

        auto result =
            CodeGeneration::generateCodeFromjS(QString::fromStdString(scriptPath.string()), ".", contentProvider);
        REQUIRE(result.has_error());
        REQUIRE(result.error().message == "SyntaxError: invalid syntax (some_script.py, line 3)");
    }
}

TEST_CASE("Code generation python API")
{
    PyDefaultGilReleasedContext _pyDefaultGilReleasedContext;

    auto tmpDir = TmpDir{TMPDIR_NAME};

    const std::string SCRIPT_CONTENTS = R"(
def buildPhysicalEntity(cgn, entity, output_dir, user_ctx):
    with open(output_dir + '/output.txt', 'a+') as f:
        f.write(f'(')

        # TEST name()
        f.write(f'{entity.name()}, ')

        # TEST type()
        f.write(f'{entity.type()}, ')

        # TEST parent()
        if entity.parent():
            f.write(f'{entity.parent().name()}, ')
        else:
            f.write(f'<no parent>, ')

        # TEST forwardDependencies()
        f.write(f'deps = [')
        for dep in entity.forwardDependencies():
            f.write(f'{dep.name()}, ')
        f.write(f']')

        f.write(f')\n')
)";
    auto scriptPath = tmpDir.createTextFile("some_script.py", SCRIPT_CONTENTS);
    auto outputDir = tmpDir.createDir("out");

    auto contentProvider = FakeContentProvider{};
    auto result = CodeGeneration::generateCodeFromjS(QString::fromStdString(scriptPath.string()),
                                                     QString::fromStdString(outputDir.string()),
                                                     contentProvider);
    if (result.has_error()) {
        INFO("Error message: " + result.error().message);
    }
    REQUIRE(!result.has_error());

    auto resultStream = std::ifstream(outputDir / "output.txt");
    auto output = std::string((std::istreambuf_iterator<char>(resultStream)), (std::istreambuf_iterator<char>()));
    REQUIRE(output == R"((somepkg_a, DiagramType.Package, <no parent>, deps = [])
(somepkg_c, DiagramType.Package, <no parent>, deps = [])
(component_b, DiagramType.Component, somepkg_c, deps = [component_a, ])
)");
}
