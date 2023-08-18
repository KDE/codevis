// ct_lvtclp_testvar.t.cpp                                             -*-C++-*-

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

// This file is for tests for CodebaseDbVisitor::VisitVarDecl
// (except for static class member variables, which are in testfield)

#include <ct_lvtmdb_namespaceobject.h>
#include <ct_lvtmdb_objectstore.h>
#include <ct_lvtmdb_typeobject.h>
#include <ct_lvtmdb_variableobject.h>

#include <ct_lvtclp_testutil.h>

#include <catch2-local-includes.h>

using namespace Codethink::lvtclp;
using namespace Codethink::lvtmdb;

const PyDefaultGilReleasedContext defaultGilContextForTesting;

TEST_CASE("Local variable declaration")
{
    const static char *source = R"(
class C {};

class D {
    void method()
    {
        C c;
    }
};
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testLocalVarDecl.cpp"));

    REQUIRE(Test_Util::usesInTheImplementationExists("D", "C", session));
}

TEST_CASE("Local variable anon parent")
{
    const static char *source = R"(
class C {};

class {
    void method()
    {
        C c;
    }
} d;
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testLocalVarAnonParent.cpp"));

    // check there are no usesInTheImpl relationships in the database
    session.withROLock([&] {
        for (const auto& [_, klass] : session.types()) {
            auto *k = klass.get();
            klass->withROLock([k] {
                REQUIRE(k->usesInTheImplementation().empty());
            });
        }
    });
}

TEST_CASE("Local variable anon type")
{
    const static char *source = R"(
class C {
    void method()
    {
        class {} c;
    }
};
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testLocalVarAnonType.cpp"));

    // check there are no usesInTheImpl relationships in the database
    session.withROLock([&] {
        for (const auto& [_, klass] : session.types()) {
            auto *k = klass.get();
            klass->withROLock([k] {
                REQUIRE(k->usesInTheImplementation().empty());
            });
        }
    });
}

TEST_CASE("Local variable template method")
{
    const static char *source = R"(
class T {};

class C {};

class D {};

class E {
  public:
    template <typename T>
    void method()
    {
        C c;
    }
};

void function()
{
    E e;
    e.method<D>();
}
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testLocalVarTemplateMethod.cpp"));

    REQUIRE(Test_Util::usesInTheImplementationExists("E", "C", session));
    REQUIRE(!Test_Util::usesInTheImplementationExists("E", "T", session));
    REQUIRE(!Test_Util::usesInTheImplementationExists("E", "D", session));
}

TEST_CASE("Local variable template method 2")
{
    const static char *source = R"(
class T {};

class D {};

class E {
  public:
    template <typename T>
    void method()
    {
        T t;
    }
};

void function()
{
    E e;
    e.method<D>();
}
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testLocalVarTemplateMethod2.cpp"));

    REQUIRE(!Test_Util::usesInTheImplementationExists("E", "T", session));
    REQUIRE(!Test_Util::usesInTheImplementationExists("E", "D", session));
}

TEST_CASE("Local variable template class")
{
    const static char *source = R"(
class T {};
class C {};
class D {};

template <typename T>
class E {
  public:
    void method()
    {
        C c;
    }
};

void function()
{
    E<D> ed;
    ed.method();
}
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testLocalVarTemplateClass.cpp"));

    REQUIRE(Test_Util::usesInTheImplementationExists("E", "C", session));
    REQUIRE(!Test_Util::usesInTheImplementationExists("E", "T", session));
    REQUIRE(!Test_Util::usesInTheImplementationExists("E", "D", session));
}

TEST_CASE("Local variable template class 2")
{
    const static char *source = R"(
class T {};
class D {};

template <typename T>
class E {
  public:
    void method()
    {
        T t;
    }
};

void function()
{
    E<D> ed;
    ed.method();
}
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testLocalVarTemplateClass2.cpp"));

    REQUIRE(!Test_Util::usesInTheImplementationExists("E", "T", session));
    REQUIRE(!Test_Util::usesInTheImplementationExists("E", "D", session));
}

TEST_CASE("Local variable lambda in method")
{
    const static char *source = R"(
class C {};

class D {
    void method()
    {
        auto lambda = []() {
            C c;
        };
    }
};
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testLocalVarLambdaInMethod.cpp"));

    REQUIRE(Test_Util::usesInTheImplementationExists("D", "C", session));
}

TEST_CASE("Local variable lambda in lambda in method")
{
    const static char *source = R"(
class C {};

class D {
    void method()
    {
        auto lambda = []() {
            auto lambda2 = []() {
                C c;
            };
        };
    }
};
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testLocalVarLambdaInLambdaInMethod.cpp"));

    REQUIRE(Test_Util::usesInTheImplementationExists("D", "C", session));
}

TEST_CASE("Local variable lambda in field")
{
    const static char *source = R"(
class C {};

class D {
    // auto not allowed in non-static class member
    // auto variables must be initialized at declaration
    // only constexpr static members can be initialized at declaration
    static constexpr auto lambda = []() {
        C c;
    };
};
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testLocalVarLambdaInField.cpp"));

    REQUIRE(Test_Util::usesInTheImplementationExists("D", "C", session));
}

TEST_CASE("Local variable in block")
{
    const static char *source = R"(
class C {};

class D {
    void method()
    {
        {
            C c;
        }
    }
};
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testLocalVarInBlock.cpp"));

    REQUIRE(Test_Util::usesInTheImplementationExists("D", "C", session));
}

TEST_CASE("Method arg relation")
{
    const static char *source = R"(
class Pub {};
class Prot {};
class Priv {};

class C {
  public:
    void pubMethod(Pub p) {}

  protected:
    void protMethod(Prot p) {}

  private:
    void privMethod(Priv p) {}
};
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testMethodArgRelation.cpp"));

    REQUIRE(Test_Util::usesInTheInterfaceExists("C", "Pub", session));
    REQUIRE(!Test_Util::usesInTheImplementationExists("C", "Pub", session));
    REQUIRE(Test_Util::usesInTheInterfaceExists("C", "Prot", session));
    REQUIRE(!Test_Util::usesInTheImplementationExists("C", "Prot", session));
    REQUIRE(!Test_Util::usesInTheInterfaceExists("C", "Priv", session));
    REQUIRE(Test_Util::usesInTheImplementationExists("C", "Priv", session));
}

TEST_CASE("Method template arg")
{
    const static char *source = R"(
template <typename T>
class C {};

template <typename T>
class D {};

class E {};

class F {};

// see if we can trip up our parsing
class T {};

class G {
  public:
    void method(C<D<E>> cde) {}

    template <typename T>
    void templateMethod(C<T> ct) {}

  private:
    void privMethod(C<F> cf) {}
};
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testMethodTemplateArg.cpp"));

    REQUIRE(Test_Util::usesInTheInterfaceExists("G", "C", session));
    REQUIRE(Test_Util::usesInTheInterfaceExists("G", "D", session));
    REQUIRE(Test_Util::usesInTheInterfaceExists("G", "E", session));
    REQUIRE(!Test_Util::usesInTheInterfaceExists("G", "T", session));
    REQUIRE(!Test_Util::usesInTheInterfaceExists("G", "F", session));
    REQUIRE(Test_Util::usesInTheImplementationExists("G", "F", session));
}

TEST_CASE("Lambda args in method")
{
    const static char *source = R"(
class C {};

class D {
  public:
    void method()
    {
        auto foo = [](C& c){};
    }
};
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testLambdaArgsInMethod.cpp"));

    // the method is public but the lambda is an implementation detail inside
    // the method so C should be usesInTheImplementation, despite C being used
    // as a method parameter
    REQUIRE(Test_Util::usesInTheImplementationExists("D", "C", session));
    REQUIRE(!Test_Util::usesInTheInterfaceExists("D", "C", session));
}

TEST_CASE("Lambda args in field")
{
    const static char *source = R"(
class Pub {};
class Prot {};
class Priv {};

class C {
  public:
    static constexpr auto pub = [](Pub p) {};

  protected:
    static constexpr auto prot = [](Prot& p) {};

  private:
    static constexpr auto priv = [](Priv *p) {};
};
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testLocalVarLambdaInField.cpp"));

    REQUIRE(Test_Util::usesInTheInterfaceExists("C", "Pub", session));
    REQUIRE(!Test_Util::usesInTheImplementationExists("C", "Pub", session));
    REQUIRE(Test_Util::usesInTheInterfaceExists("C", "Prot", session));
    REQUIRE(!Test_Util::usesInTheImplementationExists("C", "Prot", session));
    REQUIRE(!Test_Util::usesInTheInterfaceExists("C", "Priv", session));
    REQUIRE(Test_Util::usesInTheImplementationExists("C", "Priv", session));
}

TEST_CASE("Lambda args in lambda in field")
{
    const static char *source = R"(
class C {};

class D {
  public:
    static constexpr auto pub = []() {
            auto innerLambda = [](C c) {};
        };
};
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testLocalVarLambdaInLambdaInField.cpp"));

    REQUIRE(!Test_Util::usesInTheInterfaceExists("D", "C", session));
    REQUIRE(Test_Util::usesInTheImplementationExists("D", "C", session));
}

TEST_CASE("Generic lambda args")
{
    const static char *source = R"(
class C {};

class D {
  public:
    static constexpr auto lambda = [](auto foo) {};
};

class E {
    void method()
    {
        D::lambda(C());
    }
};
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testGenericLambdaArgs.cpp"));

    REQUIRE(Test_Util::usesInTheImplementationExists("E", "D", session));
    REQUIRE(Test_Util::usesInTheImplementationExists("E", "C", session));
    REQUIRE(!Test_Util::usesInTheImplementationExists("D", "C", session));
    REQUIRE(!Test_Util::usesInTheInterfaceExists("D", "C", session));
}

TEST_CASE("Namespace var decl")
{
    static const char *source = R"(
class C;

namespace foo {

C *c;

}
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testNamespaceVarDecl.cpp"));

    NamespaceObject *foo = nullptr;
    VariableObject *c = nullptr;

    session.withROLock([&] {
        foo = session.getNamespace("foo");
        c = session.getVariable("foo::c");
    });
    REQUIRE(foo);
    REQUIRE(c);

    c->withROLock([&] {
        REQUIRE(c->name() == "c");
        REQUIRE(c->parent() == foo);
        REQUIRE(!c->isGlobal());
#if CLANG_VERSION_MAJOR >= 16
        REQUIRE(c->signature() == "C *");
#else
        REQUIRE(c->signature() == "class C *");
#endif
    });
}

TEST_CASE("Global var decl")
{
    static const char *source = R"(
class C;

C *c;
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testGlobalVarDecl.cpp"));

    VariableObject *c = nullptr;

    session.withROLock([&] {
        c = session.getVariable("c");
    });
    REQUIRE(c);

    c->withROLock([&] {
        REQUIRE(c->name() == "c");
        REQUIRE(!c->parent());
        REQUIRE(c->isGlobal());
#if CLANG_VERSION_MAJOR >= 16
        REQUIRE(c->signature() == "C *");
#else
        REQUIRE(c->signature() == "class C *");
#endif

    });
}

TEST_CASE("Constexpr global var")
{
    static const char *source = R"(
namespace foo {

constexpr double pi = 3.141592654;

}
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testConstexprGlobalVar.cpp"));

    NamespaceObject *foo = nullptr;
    VariableObject *pi = nullptr;

    session.withROLock([&] {
        foo = session.getNamespace("foo");
        pi = session.getVariable("foo::pi");
    });
    REQUIRE(foo);
    REQUIRE(pi);

    pi->withROLock([&] {
        REQUIRE(pi->name() == "pi");
        REQUIRE(pi->parent() == foo);
        REQUIRE(!pi->isGlobal());
        REQUIRE(pi->signature() == "const double");
    });
}
