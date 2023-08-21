// ct_lvtclp_testphysicalandtemplates.t.cpp                           -*-C++-*-

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

#include <ct_lvtmdb_componentobject.h>
#include <ct_lvtmdb_fileobject.h>
#include <ct_lvtmdb_objectstore.h>
#include <ct_lvtmdb_packageobject.h>
#include <ct_lvtmdb_typeobject.h>

#include <ct_lvtclp_testutil.h>
#include <ct_lvtclp_tool.h>

#include <filesystem>

#include <catch2-local-includes.h>
#include <clang/Basic/Version.h>

using namespace Codethink::lvtmdb;
using namespace Codethink::lvtclp;

const PyDefaultGilReleasedContext defaultGilContextForTesting;

void createTestEnv(const std::filesystem::path& topLevel)
{
    if (std::filesystem::exists(topLevel)) {
        REQUIRE(std::filesystem::remove_all(topLevel));
    }

    REQUIRE(std::filesystem::create_directories(topLevel / "groups/bsl/bslma"));

    REQUIRE(Test_Util::createFile(topLevel / "groups/bsl/bslma/bslma_allocatortraits.h", R"(
// bslma_allocatortraits.h

namespace bslma {

template <class ALLOCATOR_TYPE>
struct AllocatorTraits_SomeTrait {
    static const bool value = false;
};

})"));
    REQUIRE(Test_Util::createFile(topLevel / "groups/bsl/bslma/bslma_allocatortraits.cpp", R"(
// bslma_allocatortraits.cpp
#include <bslma_allocatortraits.h>)"));

    REQUIRE(std::filesystem::create_directories(topLevel / "groups/foo/foobar"));

    REQUIRE(Test_Util::createFile(topLevel / "groups/foo/foobar/foobar_someclass.h", R"(
// foobar_someclass.h

namespace foobar {

class SomeClass {
};

})"));
    REQUIRE(Test_Util::createFile(topLevel / "groups/foo/foobar/foobar_someclass.cpp", R"(
// foobar_someclass.cpp
#include <foobar_someclass.h>)"));

    REQUIRE(Test_Util::createFile(topLevel / "groups/foo/foobar/foobar_vector.h", R"(
// foobar_vector.h

namespace foobar {

template <class TYPE>
class Vector {
    // ...
};

}
)"));
    REQUIRE(Test_Util::createFile(topLevel / "groups/foo/foobar/foobar_vector.cpp", R"(
// foobar_vector.cpp
#include <foobar_vector.h>)"));

    REQUIRE(Test_Util::createFile(topLevel / "groups/foo/foobar/foobar_somecomponent.h", R"(
// foobar_somecomponent.h

#include <bslma_allocatortraits.h>

namespace foobar {

class C {
    void method();
};

}

namespace bslma {

template<>
struct AllocatorTraits_SomeTrait<foobar::C> {
    static const bool value = true;
};

})"));
    REQUIRE(Test_Util::createFile(topLevel / "groups/foo/foobar/foobar_somecomponent.cpp", R"(
// foobar_somecomponent.cpp
#include <foobar_somecomponent.h>

#include <foobar_someclass.h>
#include <foobar_vector.h>

namespace foobar {

void C::method()
{
    Vector<SomeClass> v;
}

})"));
}

void testFilesExist(ObjectStore& session)
{
    auto files = {"groups/bsl/bslma/bslma_allocatortraits.h",
                  "groups/bsl/bslma/bslma_allocatortraits.cpp",
                  "groups/foo/foobar/foobar_someclass.h",
                  "groups/foo/foobar/foobar_someclass.cpp",
                  "groups/foo/foobar/foobar_vector.h",
                  "groups/foo/foobar/foobar_vector.cpp",
                  "groups/foo/foobar/foobar_somecomponent.h",
                  "groups/foo/foobar/foobar_somecomponent.cpp"};

    session.withROLock([&] {
        for (auto const& file : files) {
            REQUIRE(session.getFile(file));
        }
    });
}

void testPackagesExist(ObjectStore& session)
{
    auto packages = {"groups/bsl", "groups/bsl/bslma", "groups/foo", "groups/foo/foobar"};
    session.withROLock([&] {
        for (auto const& pkg : packages) {
            REQUIRE(session.getPackage(pkg));
        }
    });
}

bool pkgGrpHasOneChild(PackageObject *grp, PackageObject *const child)
{
    bool ret = false;
    grp->withROLock([&] {
        const auto& children = grp->children();
        if (children.size() != 1) {
            ret = false;
        } else if (children.front() != child) {
            ret = false;
        } else {
            ret = true;
        }
    });
    return ret;
}

struct ModelComponent {
    std::string name;
    std::string qualifiedName;
    std::string parent;
    std::string fileBase;
    std::vector<std::string> classes;
    std::vector<std::string> dependencies;
};

void checkComponent(ComponentObject *real, const ModelComponent& model)
{
    std::vector<TypeObject *> childClasses;
    std::vector<ComponentObject *> deps;
    std::vector<FileObject *> files;

    real->withROLock([&] {
        childClasses = real->types();
        deps = real->forwardDependencies();
        files = real->files();

        CHECK(real->name() == model.name);
        CHECK(real->qualifiedName() == model.qualifiedName);
        CHECK(real->types().size() == model.classes.size());
        auto lock = real->package()->readOnlyLock();
        CHECK(real->package()->qualifiedName() == model.parent);
    });

    // we should have a file called model.fileBase + ".h" and another called
    // model.fileBase + ".cpp" and no others
    CHECK(files.size() == 2);
    auto it = std::find_if(files.begin(), files.end(), [&](FileObject *file) {
        auto lock = file->readOnlyLock();
        return file->qualifiedName() == (model.fileBase + ".h");
    });
    CHECK(it != files.end());
    it = std::find_if(files.begin(), files.end(), [&](FileObject *file) {
        auto lock = file->readOnlyLock();
        return file->qualifiedName() == (model.fileBase + ".cpp");
    });
    CHECK(it != files.end());

    CHECK(std::equal(childClasses.begin(),
                     childClasses.end(),
                     model.classes.begin(),
                     [](TypeObject *ptr, const std::string& qname) {
                         auto lock = ptr->readOnlyLock();
                         return ptr->qualifiedName() == qname;
                     }));

    // test forward dependencies
    CHECK(deps.size() == model.dependencies.size());
    CHECK(std::equal(deps.begin(),
                     deps.end(),
                     model.dependencies.begin(),
                     [](ComponentObject *rel, const std::string& qname) {
                         auto lock = rel->readOnlyLock();
                         return rel->qualifiedName() == qname;
                     }));
}

void checkPkgComponents(PackageObject *pkg, std::initializer_list<ModelComponent> expected)
{
    std::vector<ComponentObject *> children;
    pkg->withROLock([&] {
        children = pkg->components();
    });
    REQUIRE(children.size() == expected.size());

    for (const ModelComponent& model : expected) {
        // find matching component by qualifiedName
        auto it = std::find_if(children.begin(), children.end(), [&model](ComponentObject *c) {
            auto lock = c->readOnlyLock();
            return model.qualifiedName == c->qualifiedName();
        });
        CHECK(it != children.end());
        checkComponent(*it, model);
    }
}

void checkUdtInPkg(const std::string& udtName, PackageObject *pkg, ObjectStore& session)
{
    TypeObject *udt = nullptr;
    session.withROLock([&] {
        udt = session.getType(udtName);
    });

    REQUIRE(udt);
    udt->withROLock([&] {
        REQUIRE(udt->package() == pkg);
    });

    pkg->withROLock([&] {
        std::vector<TypeObject *> udts = pkg->types();
        auto it = std::find(udts.begin(), udts.end(), udt);
        REQUIRE(it != udts.end());
    });
}

struct PhysicalAndTemplatesFixture {
    PhysicalAndTemplatesFixture():
        topLevel(std::filesystem::temp_directory_path() / "ct_lvtclp_testphysicalandtemplates_test")
    {
        createTestEnv(topLevel);
    }

    ~PhysicalAndTemplatesFixture()
    {
        REQUIRE(std::filesystem::remove_all(topLevel));
    }

    std::filesystem::path topLevel;
};

TEST_CASE_METHOD(PhysicalAndTemplatesFixture, "Physical and Templates")
{
    StaticCompilationDatabase cmds({{"groups/bsl/bslma/bslma_allocatortraits.cpp", "build/bslma_allocatortriats.o"},
                                    {"groups/foo/foobar/foobar_someclass.cpp", "build/foobar_someclass.o"},
                                    {"groups/foo/foobar/foobar_vector.cpp", "build/foobar_vector.o"},
                                    {"groups/foo/foobar/foobar_somecomponent.cpp", "build/foobar_somecomponent.o"}},
                                   "placeholder-g++",
                                   {"-Igroups/bsl/bslma", "-Igroups/foo/foobar"},
                                   topLevel);

    Tool tool(topLevel, cmds, ":memory:");
    REQUIRE(tool.runFull());
    ObjectStore& session = tool.getObjectStore();
    testFilesExist(session);
    testPackagesExist(session);

    // we test that files have things in them accross the other unit tests
    // lets focus on components and packages here

    PackageObject *bsl = nullptr;
    PackageObject *bslma = nullptr;
    PackageObject *foo = nullptr;
    PackageObject *foobar = nullptr;

    session.withROLock([&] {
        bsl = session.getPackage("groups/bsl");
        bslma = session.getPackage("groups/bsl/bslma");
        foo = session.getPackage("groups/foo");
        foobar = session.getPackage("groups/foo/foobar");
    });

    REQUIRE(bsl);
    REQUIRE(bslma);
    REQUIRE(foo);
    REQUIRE(foobar);

    // bslma is in bsl
    CHECK(pkgGrpHasOneChild(bsl, bslma));
    // foobar is in foo
    CHECK(pkgGrpHasOneChild(foo, foobar));
    // bslma has no deps
    bslma->withROLock([&] {
        CHECK(bslma->forwardDependencies().empty());

        // bslma has reverse dep on foobar
        const auto bslmaRevDeps = bslma->reverseDependencies();
        REQUIRE(bslmaRevDeps.size() == 1);
        CHECK(bslmaRevDeps.front() == foobar);
    });

    foobar->withROLock([&] {
        // foobar depends on bslma
        const auto foobarDeps = foobar->forwardDependencies();
        REQUIRE(foobarDeps.size() == 1);
        CHECK(foobarDeps.front() == bslma);
        // foobar has no reverse dependencies
        CHECK(foobar->reverseDependencies().empty());
    });

    // check the packages have classes inside of them
    checkUdtInPkg("bslma::AllocatorTraits_SomeTrait", bslma, session);
    checkUdtInPkg("foobar::SomeClass", foobar, session);
    checkUdtInPkg("foobar::Vector", foobar, session);
    checkUdtInPkg("foobar::C", foobar, session);

    // check bslma compnoents
    checkPkgComponents(bslma,
                       {{"bslma_allocatortraits",
                         "groups/bsl/bslma/bslma_allocatortraits",
                         "groups/bsl/bslma",
                         "groups/bsl/bslma/bslma_allocatortraits",
                         {"bslma::AllocatorTraits_SomeTrait"},
                         {}}});

    // check foobar components
    checkPkgComponents(foobar,
                       {{"foobar_someclass",
                         "groups/foo/foobar/foobar_someclass",
                         "groups/foo/foobar",
                         "groups/foo/foobar/foobar_someclass",
                         {"foobar::SomeClass"},
                         {}},
                        {"foobar_vector",
                         "groups/foo/foobar/foobar_vector",
                         "groups/foo/foobar",
                         "groups/foo/foobar/foobar_vector",
                         {"foobar::Vector"},
                         {}},
                        {
                            "foobar_somecomponent",
                            "groups/foo/foobar/foobar_somecomponent",
                            "groups/foo/foobar",
                            "groups/foo/foobar/foobar_somecomponent",
                            // TODO - this is not supposed to add - bslma::AllocatorTraits_SomeTrait<foobar::C>, so we
                            // need to check logicaldepvisitor. Perhaps we need to add a new rule that inner namespaces
                            // that have names that matches packages are out of scope.
                            {"foobar::C", "bslma::AllocatorTraits_SomeTrait"},
                            {"groups/bsl/bslma/bslma_allocatortraits",
                             "groups/foo/foobar/foobar_someclass",
                             "groups/foo/foobar/foobar_vector"},
                        }});

    // C usesInImpl on Vector and SomeClass,
    // no relationship between AllocatorTraits_SomeTrait and C
    REQUIRE(Test_Util::usesInTheImplementationExists("foobar::C", "foobar::Vector", session));
    REQUIRE(Test_Util::usesInTheImplementationExists("foobar::C", "foobar::SomeClass", session));
    REQUIRE(!Test_Util::usesInTheImplementationExists("foobar::C", "bslma::AllocatorTraits_SomeTrait", session));
    REQUIRE(!Test_Util::usesInTheImplementationExists("bslma::AllocatorTraits_SomeTrait", "foobar::C", session));
    REQUIRE(!Test_Util::usesInTheInterfaceExists("foobar::C", "bslma::AllocatorTraits_SomeTrait", session));
    REQUIRE(!Test_Util::usesInTheInterfaceExists("bslma::AllocatorTraits_SomeTrait", "foobar::C", session));
}

class NonLakosianFixture {
  public:
    // public data for Catch2 magic
    std::filesystem::path d_topLevel;
    std::filesystem::path d_pcre2;
    std::filesystem::path d_pcre2Source;
    std::filesystem::path d_pcre2Header;
    std::filesystem::path d_sljit;
    std::filesystem::path d_sljitSource;
    std::filesystem::path d_sljitHeader;
    std::filesystem::path d_bdlpcre;
    std::filesystem::path d_bdlpcreRegex;

    void createTestEnv() const
    {
        if (std::filesystem::exists(d_topLevel)) {
            REQUIRE(std::filesystem::remove_all(d_topLevel));
        }

        REQUIRE(std::filesystem::create_directories(d_sljit));
        REQUIRE(Test_Util::createFile(d_sljitSource));
        REQUIRE(Test_Util::createFile(d_sljitHeader));
        REQUIRE(Test_Util::createFile(d_pcre2Source, "#include \"sljit/sljit.h\""));
        REQUIRE(Test_Util::createFile(d_pcre2Header));

        REQUIRE(std::filesystem::create_directories(d_bdlpcre));
        REQUIRE(Test_Util::createFile(d_bdlpcreRegex, "#include <pcre2/pcre2.h>"));
    }

    NonLakosianFixture():
        d_topLevel(std::filesystem::temp_directory_path() / "ct_lvtclp_testphysicalandtemplates_nonlakosian"),
        d_pcre2(d_topLevel / "thirdparty" / "pcre2"),
        d_pcre2Source(d_pcre2 / "pcre2.c"),
        d_pcre2Header(d_pcre2 / "pcre2.h"),
        d_sljit(d_pcre2 / "sljit"),
        d_sljitSource(d_sljit / "sljit.c"),
        d_sljitHeader(d_sljit / "sljit.h"),
        d_bdlpcre(d_topLevel / "groups" / "bdl" / "bdlpcre"),
        d_bdlpcreRegex(d_bdlpcre / "bdlpcre_regex.cpp")
    {
        createTestEnv();
    }

    ~NonLakosianFixture()
    {
        REQUIRE(std::filesystem::remove_all(d_topLevel));
    }
};

TEST_CASE_METHOD(NonLakosianFixture, "Non-lakosian extra levels of hierarchy")
{
    StaticCompilationDatabase cmds(
        {{d_sljitSource.string(), ""}, {d_pcre2Source.string(), ""}, {d_bdlpcreRegex.string(), ""}},
        "placeholder-g++",
        {"-Ithirdparty"},
        d_topLevel);

    Tool tool(d_topLevel, cmds, ":memory:");
    REQUIRE(tool.runFull());
    ObjectStore& session = tool.getObjectStore();

    FileObject *sljitSource = nullptr;
    PackageObject *sljit = nullptr;
    PackageObject *pcre2 = nullptr;
    PackageObject *bdlpcre = nullptr;
    PackageObject *bdl = nullptr;
    PackageObject *nonlakosiangroup = nullptr;

    session.withROLock([&] {
        sljitSource = session.getFile("thirdparty/pcre2/sljit/sljit.c");
        sljit = session.getPackage("thirdparty/pcre2/sljit");
        pcre2 = session.getPackage("thirdparty/pcre2");
        bdlpcre = session.getPackage("groups/bdl/bdlpcre");
        bdl = session.getPackage("groups/bdl");
        nonlakosiangroup = session.getPackage("non-lakosian group");
    });

    REQUIRE(sljitSource);
    REQUIRE(sljit);
    REQUIRE(pcre2);
    REQUIRE(bdlpcre);
    REQUIRE(bdl);
    REQUIRE(nonlakosiangroup);

    sljitSource->withROLock([&] {
        CHECK(sljitSource->package() == sljit); // file/component
    });

    // TODO: Check if we need to verify the package grouop dependencies here.
    // I believe this is only needed on lvtldr.
    sljit->withROLock([&] {
        CHECK(sljit->parent() == nonlakosiangroup); // package

        //        CHECK(sljit->packageGroupDependencies().empty());
        //        CHECK(sljit->packageGroupRevDependencies().size() == 1);
    });

    bdlpcre->withROLock([&] {
        CHECK(bdlpcre->parent() == bdl);
        //        CHECK(bdlpcre->packageGroupDependencies().size() == 1);
        //        CHECK(bdlpcre->packageGroupRevDependencies().empty());
    });
    bdl->withROLock([&] {
        CHECK(!bdl->parent());
        //        CHECK(bdl->packageGroupDependencies().size() == 1);
        //        CHECK(bdl->packageGroupRevDependencies().empty());
    });

    pcre2->withROLock([&] {
        //        CHECK(pcre2->packageGroupDependencies().size() == 1);
        //        CHECK(pcre2->packageGroupRevDependencies().size() == 1);
    });

    // check that all packageGroupDependencies() are correct
    // this is a regression test for an old assertion error
}
