// ct_lvtclp_testfunction.t.cpp                                        -*-C++-*-

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

// This file is for tests for VisitFunctionDecl and VisitCXXMethodDecl

#include <ct_lvtmdb_functionobject.h>
#include <ct_lvtmdb_methodobject.h>
#include <ct_lvtmdb_namespaceobject.h>
#include <ct_lvtmdb_objectstore.h>
#include <ct_lvtmdb_typeobject.h>

#include <ct_lvtshr_graphenums.h>

#include <ct_lvtclp_testutil.h>

#include <initializer_list>
#include <iostream>

#include <catch2-local-includes.h>
#include <clang/Basic/Version.h>

using namespace Codethink::lvtclp;
using namespace Codethink::lvtmdb;
using namespace Codethink;

const PyDefaultGilReleasedContext defaultGilContextForTesting;

TEST_CASE("Function declaration")
{
    const static char *source = R"(
namespace foo {

int function(int arg0, const int& arg1)
{
    if (arg0)
        return arg1;
    return -arg1;
}

}
)";

    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testFunctionDecl.cpp"));

    NamespaceObject *foo;
    FunctionObject *fn = nullptr;

    session.withROLock([&] {
        foo = session.getNamespace("foo");
        fn = session.getFunction("foo::function", "function(int arg0, const int & arg1)", std::string{}, "int");
    });

    REQUIRE(foo);
    REQUIRE(fn);

    fn->withROLock([&] {
        REQUIRE(fn->name() == "function");
        REQUIRE(fn->signature() == "function(int arg0, const int & arg1)");
        REQUIRE(fn->returnType() == "int");
        REQUIRE(fn->templateParameters().empty());
        REQUIRE(fn->parent() == foo);
    });
}

TEST_CASE("Static function")
{
    const static char *source = R"(
static void function()
{
}
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testStaticFn.cpp"));

    session.withROLock([&] {
        REQUIRE(session.functions().size() == 1);
    });
}

TEST_CASE("Function overload")
{
    const static char *source = R"(
int foo()
{
    return 2;
}

int foo(int arg)
{
    return arg;
}
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testFuncOverload.cpp"));
    FunctionObject *foo0 = nullptr;
    FunctionObject *foo1 = nullptr;

    session.withROLock([&] {
        foo0 = session.getFunction("foo", "foo()", std::string{}, "int");
        foo1 = session.getFunction("foo", "foo(int arg)", std::string{}, "int");
    });

    REQUIRE(foo0);
    REQUIRE(foo1);

    for (const auto& fn : {foo0, foo1}) {
        auto lock = fn->readOnlyLock();
        REQUIRE(fn->qualifiedName() == "foo");
        REQUIRE(fn->returnType() == "int");
        REQUIRE(fn->templateParameters().empty());
        REQUIRE(!fn->parent());
    }
}

TEST_CASE("Function forward declaration")
{
    const static char *source = R"(
void function();

void function()
{
}
)";

    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testFnForwardDecl.cpp"));

    session.withROLock([&] {
        // REQUIRE(session.getFunction("function"));
        // REQUIRE(func);

        // check we didn't add any kind of second function for the forward decl
        REQUIRE(session.functions().size() == 1);
    });
}

TEST_CASE("Function template")
{
    const static char *source = R"(
template <typename T>
T clone(const T& t)
{
    return t;
}

void instantiateSpecialization()
{
    int i = 2;
    int j = clone(i);
    int k = clone<int>(i);
}
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testFnTemplate.cpp"));

    FunctionObject *clone = nullptr;
    session.withROLock([&] {
        clone = session.getFunction("clone", "clone(const T & t)", "template <typename T>", "type-parameter-0-0");

        // check we didn't separately add clone<int>
        REQUIRE(session.functions().size() == 2);
    });

    REQUIRE(clone);

    clone->withROLock([&] {
        REQUIRE(clone->templateParameters() == "template <typename T>");
    });
}

TEST_CASE("Function default parameters")
{
    const static char *source = R"(
namespace foo {

int function(int &arg1, const int arg0 = 0)
{
    if (arg0)
        return arg1;
    return -arg1;
}

}
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testFnDefaultParams.cpp"));

    NamespaceObject *foo = nullptr;
    FunctionObject *fn = nullptr;
    session.withROLock([&] {
        foo = session.getNamespace("foo");
        fn = session.getFunction("foo::function", "function(int & arg1, const int arg0 = 0)", std::string{}, "int");
    });

    REQUIRE(foo);
    REQUIRE(fn);

    fn->withROLock([&] {
        REQUIRE(fn);
        REQUIRE(fn->name() == "function");
        REQUIRE(fn->signature() == "function(int & arg1, const int arg0 = 0)");
        REQUIRE(fn->returnType() == "int");
        REQUIRE(fn->templateParameters().empty());
        REQUIRE(fn->parent() == foo);
    });
}

TEST_CASE("Method declaration")
{
    const static char *source = R"(
class C {
    void method();
};

void C::method()
{
}
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testMethodDeclaration.cpp"));

    TypeObject *C = nullptr;
    MethodObject *method = nullptr;

    session.withROLock([&] {
        C = session.getType("C");
        method = session.getMethod("C::method", "method()", std::string{}, "void");
    });
    REQUIRE(method);

    method->withROLock([&] {
        REQUIRE(method->name() == "method");
        REQUIRE(method->signature() == "method()");
        REQUIRE(method->returnType() == "void");
        REQUIRE(method->templateParameters().empty());
        REQUIRE(!method->isVirtual());
        REQUIRE(!method->isPure());
        REQUIRE(!method->isStatic());
        REQUIRE(!method->isConst());
        REQUIRE(method->parent() == C);
        REQUIRE(method->argumentTypes().empty());
    });
}

TEST_CASE("Method access")
{
    const static char *source = R"(
class C {
  public:
    void pub() {}

  protected:
    void prot() {}

  private:
    void priv() {}
};
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testMethodAccess.cpp"));

    MethodObject *pub = nullptr;
    MethodObject *prot = nullptr;
    MethodObject *priv = nullptr;

    session.withROLock([&] {
        pub = session.getMethod("C::pub", "pub()", std::string{}, "void");
        prot = session.getMethod("C::prot", "prot()", std::string{}, "void");
        priv = session.getMethod("C::priv", "priv()", std::string{}, "void");
    });
    REQUIRE(pub);
    REQUIRE(prot);
    REQUIRE(priv);

    pub->withROLock([&] {
        REQUIRE(pub->access() == lvtshr::AccessSpecifier::e_public);
    });
    prot->withROLock([&] {
        REQUIRE(prot->access() == lvtshr::AccessSpecifier::e_protected);
    });
    priv->withROLock([&] {
        REQUIRE(priv->access() == lvtshr::AccessSpecifier::e_private);
    });
}

TEST_CASE("Method specifiers")
{
    const static char *source = R"(
class C {
    virtual void virtMethod() {}
    virtual void pureMethod() = 0;
    static void staticMethod() {}
    void constMethod() const {}
};
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testMethodSpecifiers.cpp"));

    MethodObject *virtMethod = nullptr;
    MethodObject *pureMethod = nullptr;
    MethodObject *staticMethod = nullptr;
    MethodObject *constMethod = nullptr;

    session.withROLock([&] {
        virtMethod = session.getMethod("C::virtMethod", "virtMethod()", std::string{}, "void");
        pureMethod = session.getMethod("C::pureMethod", "pureMethod()", std::string{}, "void");
        staticMethod = session.getMethod("C::staticMethod", "staticMethod()", std::string{}, "void");
        constMethod = session.getMethod("C::constMethod", "constMethod()", std::string{}, "void");
    });

    REQUIRE(virtMethod);
    REQUIRE(pureMethod);
    REQUIRE(staticMethod);
    REQUIRE(constMethod);

    virtMethod->withROLock([&] {
        REQUIRE(virtMethod->isVirtual());
        REQUIRE(!virtMethod->isPure());
        REQUIRE(!virtMethod->isStatic());
        REQUIRE(!virtMethod->isConst());
    });

    pureMethod->withRWLock([&] {
        REQUIRE(pureMethod->isVirtual());
        REQUIRE(pureMethod->isPure());
        REQUIRE(!pureMethod->isStatic());
        REQUIRE(!pureMethod->isConst());
    });

    staticMethod->withROLock([&] {
        REQUIRE(!staticMethod->isVirtual());
        REQUIRE(!staticMethod->isPure());
        REQUIRE(staticMethod->isStatic());
        REQUIRE(!staticMethod->isConst());
    });

    constMethod->withROLock([&] {
        REQUIRE(!constMethod->isVirtual());
        REQUIRE(!constMethod->isPure());
        REQUIRE(!constMethod->isStatic());
        REQUIRE(constMethod->isConst());
    });
}

TEST_CASE("Method arguments")
{
    const static char *source = R"(
class C {};
class D {
    void method(int i, const C& c) {}
};
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testMethodArguments.cpp"));

    MethodObject *method = nullptr;
    session.withROLock([&] {
#if CLANG_VERSION_MAJOR >= 16
        method = session.getMethod("D::method", "method(int i, const C & c)", std::string{}, "void");
#else
        method = session.getMethod("D::method", "method(int i, const class C & c)", std::string{}, "void");
#endif
    });
    REQUIRE(method);

    method->withROLock([&] {
        const auto argumentClasses = method->argumentTypes();
        REQUIRE(argumentClasses.size() == 1);
        auto *C = *argumentClasses.begin();

        C->withROLock([&] {
            REQUIRE(C->qualifiedName() == "C");
        });
    });
}

TEST_CASE("Method overload")
{
    const static char *source = R"(
class C {};

class D {
    int foo ()
    {
        return 2;
    }

    int foo(C arg)
    {
        return 2;
    }
};
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testMethodOverload.cpp"));

    TypeObject *C = nullptr;
    TypeObject *D = nullptr;
    MethodObject *foo0 = nullptr; // method without parameters
    MethodObject *foo1 = nullptr; // method with parameters
    session.withROLock([&] {
        C = session.getType("C");
        D = session.getType("D");
        foo0 = session.getMethod("D::foo", "foo()", std::string{}, "int");
#if CLANG_VERSION_MAJOR >= 16
        foo1 = session.getMethod("D::foo", "foo(C arg)", std::string{}, "int");
#else
        foo1 = session.getMethod("D::foo", "foo(class C arg)", std::string{}, "int");
#endif
    });

    REQUIRE(C);
    REQUIRE(D);
    REQUIRE(foo0);
    REQUIRE(foo1);

    for (const auto& foo : {foo0, foo1}) {
        foo->withROLock([&] {
            REQUIRE(foo->qualifiedName() == "D::foo");
            REQUIRE(foo->returnType() == "int");
            REQUIRE(foo->templateParameters().empty());
            REQUIRE(foo->access() == lvtshr::AccessSpecifier::e_private);
            REQUIRE(!foo->isVirtual());
            REQUIRE(!foo->isPure());
            REQUIRE(!foo->isStatic());
            REQUIRE(!foo->isConst());
            REQUIRE(foo->parent() == D);
        });
    }

    foo0->withROLock([&] {
        REQUIRE(foo0->argumentTypes().empty());
    });

    foo1->withROLock([&] {
        const auto foo1Args = foo1->argumentTypes();
        REQUIRE(foo1Args.size() == 1);
        REQUIRE(*foo1Args.begin() == C);
    });
}

TEST_CASE("Method template")
{
    const static char *source = R"(
class C {
  public:
    template <typename T>
    T clone(const T& t)
    {
        return t;
    }
};

void instantiateSpecialization()
{
    C c;
    int i = 2;
    int j = c.clone(i);
}
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testMethodTemplate.cpp"));

    MethodObject *clone = nullptr;
    session.withROLock([&] {
        for (const auto& [_, fn] : session.methods()) {
            std::cout << "Key = --- \n" << _ << "\n ---\n";
        }

        clone = session.getMethod("C::clone", "clone(const T & t)", "template <typename T>", "type-parameter-0-0");

        // check we didn't separately add C::clone<int>
        REQUIRE(session.functions().size() == 1);
    });
    REQUIRE(clone);

    clone->withROLock([&] {
        REQUIRE(clone->templateParameters() == "template <typename T>");
    });
}

TEST_CASE("Method with default param")
{
    const static char *source = R"(
class C {
    void method(int i = 0);
};

void C::method(int i)
{
}
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testMethodDefaultParam.cpp"));

    TypeObject *C = nullptr;
    MethodObject *method = nullptr;

    session.withROLock([&] {
        C = session.getType("C");
        method = session.getMethod("C::method", "method(int i = 0)", std::string{}, "void");
    });
    REQUIRE(C);
    REQUIRE(method);

    method->withROLock([&] {
        REQUIRE(method->name() == "method");
        REQUIRE(method->signature() == "method(int i = 0)");
        REQUIRE(method->returnType() == "void");
        REQUIRE(method->templateParameters().empty());
        REQUIRE(!method->isVirtual());
        REQUIRE(!method->isPure());
        REQUIRE(!method->isStatic());
        REQUIRE(!method->isConst());
        REQUIRE(method->parent() == C);
        REQUIRE(method->argumentTypes().empty());
    });
}

TEST_CASE("Dependency via free function")
{
    // We need both C and CFactory because variable declarations and static
    // function calls are tracked in different places in CodebaseDbVisitor
    const static char *source = R"(
class C {
  public:
    void method(char c);
};

class CFactory {
  public:
    static C makeC();
};

static void freeFunction(char ch)
{
    C c = CFactory::makeC();
    c.method(ch);
}

class D {
    void method()
    {
        freeFunction('D');
    }
};

class E {
    void method()
    {
        freeFunction('E');
    }
};
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testDepViaFreeFunction.cpp"));

    REQUIRE(Test_Util::usesInTheImplementationExists("D", "CFactory", session));
    REQUIRE(Test_Util::usesInTheImplementationExists("D", "C", session));
    REQUIRE(Test_Util::usesInTheImplementationExists("E", "CFactory", session));
    REQUIRE(Test_Util::usesInTheImplementationExists("E", "C", session));
}

TEST_CASE("Dependency via function")
{
    const static char *source = R"(
class C {
  public:
    void method();
};

static void func1()
{
    C c;
    c.method();
}

static void func2()
{
    func1();
}

class D {
    void method()
    {
        func2();
    }
};
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testDepViaFunctionViaFunction.cpp"));

    REQUIRE(Test_Util::usesInTheImplementationExists("D", "C", session));
}

TEST_CASE("Circular function dependency")
{
    const static char *source = R"(
class C {};
class D {};

static void funcC(bool b);
static void funcD(bool b);

static void funcC(bool b)
{
    C c;
    if (b) {
        funcD(false);
    }
}

static void funcD(bool b)
{
    D d;
    if (b) {
        funcC(false);
    }
}

class E {
    void method()
    {
        funcC(true);
    };
};
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testDepViaCircularFunctionDep.cpp"));

    REQUIRE(Test_Util::usesInTheImplementationExists("E", "C", session));
    REQUIRE(Test_Util::usesInTheImplementationExists("E", "D", session));
}

TEST_CASE("Tricky function expansion")
{
    // This is a bad test. It relies on the unordered_multimap in staticfnhandler
    // traversing x then y then z. On clang+gnu(?)+linux this seems to work by
    // the order functions appear in the file
    static const char *source = R"(
class C {};

static void x();
static void y();
static void z();

static void x()
{
    C c;
}

static void y()
{
    x();
}

static void z()
{
    y();
}

class D {
    void method()
    {
        z();
    }
};
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testTrickyFUnctionExpansion.cpp"));

    REQUIRE(Test_Util::usesInTheImplementationExists("D", "C", session));
}

TEST_CASE("Function dependency overload")
{
    // check that the code for detecting uses-in-impl relationships via static
    // free functions isn't confused by overloaded functions
    static const char *source = R"(
class C {};
class D {};

static void freeFunction(int arg)
{
    C c;
}

static void freeFunction(bool arg)
{
    D d;
}

class UsesC {
    void method()
    {
        freeFunction(42);
    }
};

class UsesD {
    void method()
    {
        freeFunction(true);
    }
};
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testFunctionDepOverload.cpp"));

    REQUIRE(Test_Util::usesInTheImplementationExists("UsesC", "C", session));
    REQUIRE(Test_Util::usesInTheImplementationExists("UsesD", "D", session));
    REQUIRE(!Test_Util::usesInTheImplementationExists("UsesC", "D", session));
    REQUIRE(!Test_Util::usesInTheImplementationExists("UsesD", "C", session));
}

// TODO: Review this. Should an indirect call to a static class member through a free function infer a direct
// dependency? I think no. But the original writer of this test believes yes. Perhaps because at the time, we didn't
// have the free functions as entities, so it kind of make sense, but now that we do have them, I think the indirect
// dependency should be enough. The same applies for the test above, which is _not_ failing, but I believe it should
// fail. All that must be revisited.
//
// clang-format off
//TEST_CASE("Function dependency with template")
//{
//    // check that the code for detecting uses-in-impl relationships via static
//    // free functions isn't confused by template specializations
//    static const char *source = R"(
//class C {};
//class D { public: static void method(); };
//
//template <class TYPE>
//static void freeFunction();
//
//template <>
//void freeFunction<int>()
//{
//    // test CodebaseDbVisitor::visitLocalVarDeclOrParam path
//    C c;
//}
//
//template <>
//void freeFunction<bool>()
//{
//    // test CodebaseDbVisitor::searchClangStmtAST path
//    D::method();
//}
//
//class UsesC {
//    void method()
//    {
//        freeFunction<int>();
//    }
//};
//
//class UsesD {
//    void method()
//    {
//        freeFunction<bool>();
//    }
//};
//)";
//    ObjectStore session;
//    REQUIRE(Test_Util::runOnCode(session, source, "testFunctionDepTemplate.cpp"));
//
//    REQUIRE(Test_Util::usesInTheImplementationExists("UsesC", "C", session));
//    REQUIRE(Test_Util::usesInTheImplementationExists("UsesD", "D", session));
//    REQUIRE(!Test_Util::usesInTheImplementationExists("UsesC", "D", session));
//    REQUIRE(!Test_Util::usesInTheImplementationExists("UsesD", "C", session));
//}
// clang-format on
