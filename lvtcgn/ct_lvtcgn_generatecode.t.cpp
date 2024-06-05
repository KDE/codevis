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

#include <QCoreApplication>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <fstream>
#include <string>

using namespace std::string_literals;
static const std::string TMPDIR_NAME = "tmp_ct_lvtcgn_generatecode";

using namespace Codethink::lvtcgn::mdl;

TEST_CASE("Basic code generation")
{
    int argc = 0;
    char **argv = nullptr;
    QCoreApplication app(argc, argv);

    auto tmp_dir = TmpDir{TMPDIR_NAME};

    QString SCRIPT_CONTENTS = R"(
var result_string = "";
var entities_processed = 0;

export function beforeProcessEntities(output_dir) {
        result_string = 'BEFORE process entities called.\n';
        entities_processed = 0;
        console.log("Run Before Process Entities\n")
}

export function buildPhysicalEntity(entity, output_dir) {
    result_string += '(' + entity.name() + ", " + entity.type() + ");";
    entities_processed += 1;
    console.log("Run Build Physical Entity\n")
}

export function afterProcessEntities(output_dir) {
    entities_processed += 1;
    result_string += "\nAFTER process entities called. " + entities_processed;

    var file = new FileIO();
    file.saveFile(output_dir + "/output.txt", result_string);
    console.log("Run After Process Entities")
}

)";

    QTemporaryFile scriptFile;
    scriptFile.open();
    QTextStream s(&scriptFile);
    s << SCRIPT_CONTENTS;
    scriptFile.close();
    auto outputDir = tmp_dir.createDir("out");
    QString scriptPath = scriptFile.fileName();

    auto contentProvider = FakeContentProvider{};
    auto result = CodeGeneration::generateCodeFromjS(scriptPath, QString::fromStdString(outputDir), contentProvider);
    if (result.has_error()) {
        qDebug() << ("Error message: " + result.error().message);
    } else {
        qDebug() << "Code generation worked.";
    }

    auto resultStream = std::ifstream(outputDir / "output.txt");
    auto output = std::string((std::istreambuf_iterator<char>(resultStream)), (std::istreambuf_iterator<char>()));
    REQUIRE(output == R"(BEFORE process entities called.
(somepkg_a, Package);(somepkg_c, Package);(component_b, Component);
AFTER process entities called. 4)");

    qDebug() << "Finished test, all passed";
}

#if 0
TEST_CASE("Code generation errors")
{
    import
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
#endif
