// ct_lvtclp_testrelationships.t.cpp                                   -*-C++-*-

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

// This file is home to general tests of codebasedbvisitor

#include <ct_lvtmdb_objectstore.h>

#include <ct_lvtclp_testutil.h>

#include <catch2/catch.hpp>

using namespace Codethink::lvtclp;
using namespace Codethink::lvtmdb;

PyDefaultGilReleasedContext _;

TEST_CASE("Private field")
{
    const static char *source =
        R"(class C {};
   class D {
     private:
       C c;
   };
)";

    ObjectStore memDb;
    REQUIRE(Test_Util::runOnCode(memDb, source, "testPrivateField.cpp"));
    REQUIRE(Test_Util::usesInTheImplementationExists("D", "C", memDb));
}

TEST_CASE("Public field")
{
    const static char *source =
        R"(class C {};
   class D {
     public:
       C c;
   };
)";

    ObjectStore memDb;
    REQUIRE(Test_Util::runOnCode(memDb, source, "testPrivateField.cpp"));
    REQUIRE(Test_Util::usesInTheInterfaceExists("D", "C", memDb));
}

TEST_CASE("Method in interface")
{
    const static char *source =
        R"(class C;
   class D;
   class E;

   class Foo {
     public:
       void pub(C& c)
       {
       }

     protected:
       void prot(D& c)
       {
       }

     private:
       void priv(E& e)
       {
       }
   };
)";

    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testMethodInterface.cpp"));
    REQUIRE(Test_Util::usesInTheInterfaceExists("Foo", "C", session));
    REQUIRE(!Test_Util::usesInTheImplementationExists("Foo", "C", session));
    REQUIRE(Test_Util::usesInTheInterfaceExists("Foo", "D", session));
    REQUIRE(!Test_Util::usesInTheImplementationExists("Foo", "D", session));
    REQUIRE(!Test_Util::usesInTheInterfaceExists("Foo", "E", session));
    REQUIRE(Test_Util::usesInTheImplementationExists("Foo", "E", session));
}

TEST_CASE("Method body")
{
    {
        const static char *source =
            R"(class C {};
        class D {
            void method() {
                C c;
            }
        };
        )";

        ObjectStore session;
        REQUIRE(Test_Util::runOnCode(session, source, "testMethodBody.cpp"));
        REQUIRE(Test_Util::usesInTheImplementationExists("D", "C", session));
    }

    {
        const static char *source =
            R"(class C {};
        class D {
            void method();
        };
        void D::method()
        {
            C c;
        }
        )";

        ObjectStore session;
        REQUIRE(Test_Util::runOnCode(session, source, "testMethodBody2.cpp"));
        REQUIRE(Test_Util::usesInTheImplementationExists("D", "C", session));
    }
}

TEST_CASE("Temporary object")
{
    const static char *source =
        R"(class C {};
   void freeFunction(C c) {}
   class D {
       void method() {
           freeFunction(C());
       }
   };
)";

    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testTemporaryObject.cpp"));
    REQUIRE(Test_Util::usesInTheImplementationExists("D", "C", session));
}

TEST_CASE("Static method")
{
    const static char *source =
        R"(class C {
     public:
       static void method() {}
   };

   class D {
       void method()
       {
           C::method();
       }
   };
)";

    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testStaticMethod.cpp"));
    REQUIRE(Test_Util::usesInTheImplementationExists("D", "C", session));
}

TEST_CASE("Typedef")
{
    const static char *source =
        R"(class C {};
   typedef C CEE;
   class D {
     private:
       CEE c;
   };
)";

    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testTypedef.cpp"));
    REQUIRE(Test_Util::usesInTheImplementationExists("CEE", "C", session));
    REQUIRE(Test_Util::usesInTheImplementationExists("D", "CEE", session));
}

TEST_CASE("Using alias")
{
    const static char *source =
        R"(class C {};
   using CEE = C;
   class D {
     private:
       CEE c;
   };
)";

    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testUsingAlias.cpp"));
    REQUIRE(Test_Util::usesInTheImplementationExists("CEE", "C", session));
    REQUIRE(Test_Util::usesInTheImplementationExists("D", "CEE", session));
}

TEST_CASE("Template class")
{
    const static char *source =
        R"(class C {};

   template<typename T>
   class D {
       T t;
   };

   class E {
       D<C> dc;
   };
)";

    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testTemplateClass.cpp"));
    REQUIRE(Test_Util::usesInTheImplementationExists("E", "C", session));
    REQUIRE(Test_Util::usesInTheImplementationExists("E", "D", session));
    REQUIRE(!Test_Util::usesInTheImplementationExists("D", "C", session));
    REQUIRE(!Test_Util::usesInTheImplementationExists("D", "T", session));
}

TEST_CASE("Template method")
{
    const static char *source =
        R"(class C {};

   class D {
     public:
       template <typename T>
       static void method(T t)
       {
           C c;
       }
   };

   class E {
     public:
       void method()
       {
           D::method(1);
       }
   };
)";

    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testTemplateMethod.cpp"));
    REQUIRE(Test_Util::usesInTheImplementationExists("D", "C", session));
    REQUIRE(Test_Util::usesInTheImplementationExists("E", "D", session));
}

TEST_CASE("Member type")
{
    const static char *source =
        R"(class Pub1 {};
   class Pub2 {};
   class Prot1 {};
   class Prot2 {};
   class Priv1 {};
   class Priv2 {};

   class C {
     public:
       using pub1 = Pub1;
       typedef Pub2 pub2;

     protected:
       using prot1 = Prot1;
       typedef Prot2 prot2;

     private:
       using priv1 = Priv1;
       typedef Priv2 priv2;
   };
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testMemberType.cpp"));
    REQUIRE(Test_Util::usesInTheInterfaceExists("C", "C::pub1", session));
    REQUIRE(Test_Util::usesInTheInterfaceExists("C", "C::pub2", session));
    REQUIRE(Test_Util::usesInTheInterfaceExists("C", "C::prot1", session));
    REQUIRE(Test_Util::usesInTheInterfaceExists("C", "C::prot2", session));
    REQUIRE(Test_Util::usesInTheImplementationExists("C", "C::priv1", session));
    REQUIRE(Test_Util::usesInTheImplementationExists("C", "C::priv2", session));
    REQUIRE(Test_Util::usesInTheImplementationExists("C::pub1", "Pub1", session));
    REQUIRE(Test_Util::usesInTheImplementationExists("C::pub2", "Pub2", session));
    REQUIRE(Test_Util::usesInTheImplementationExists("C::prot1", "Prot1", session));
    REQUIRE(Test_Util::usesInTheImplementationExists("C::prot2", "Prot2", session));
    REQUIRE(Test_Util::usesInTheImplementationExists("C::priv1", "Priv1", session));
    REQUIRE(Test_Util::usesInTheImplementationExists("C::priv2", "Priv2", session));
}

TEST_CASE("Template member variable")
{
    const static char *source =
        R"(class C {};

   template <typename T>
   class D {
     public:
       void method()
       {
           T var;
       }
   };

   class E {
       D<C> d;

       void method()
       {
           d.method();
       }
   };
)";

    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testTemplateMemberVar.cpp"));

    // we want to make sure the template instantiation doesn't cause a spurious
    // D -> C relation
    // (regression test)
    REQUIRE(!Test_Util::usesInTheImplementationExists("D", "C", session));
    REQUIRE(Test_Util::usesInTheImplementationExists("E", "C", session));
    REQUIRE(Test_Util::usesInTheImplementationExists("E", "D", session));
}

TEST_CASE("Template member variable 2")
{
    const static char *source =
        R"(class C {};

   class D {
     public:
       template <typename T>
       void method()
       {
           T var;
       }
   };

   class E {
       D d;

       void method()
       {
           d.method<C>();
       }
   };
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testTemplateMemberVar2.cpp"));

    // we want to make sure the template instantiation doesn't cause a spurious
    // D -> C relation
    // (regression test)
    REQUIRE(!Test_Util::usesInTheImplementationExists("D", "C", session));
    REQUIRE(Test_Util::usesInTheImplementationExists("E", "C", session));
    REQUIRE(Test_Util::usesInTheImplementationExists("E", "D", session));
}

TEST_CASE("Template non template variable")
{
    const static char *source =
        R"(class C {};
   class D {};

   template <typename T>
   class E {
     public:
       void method()
       {
           T a;
           D b;
       }
   };

   class F {
       E<C> e;

       void method()
       {
           e.method();
       }
   };
)";

    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testTemplateNonTemplateVar.cpp"));
    REQUIRE(Test_Util::usesInTheImplementationExists("E", "D", session));
    REQUIRE(Test_Util::usesInTheImplementationExists("F", "E", session));
    REQUIRE(Test_Util::usesInTheImplementationExists("F", "C", session));
}

TEST_CASE("Weird template specialization")
{
    // based on BDLAT_DECL_ENUMERATION_TRAITS
    // in bdlat_typetraits.h
    const static char *source =
        R"(class C {};

   template <typename T>
   struct S {};

   template <>
   struct S<C> : C {
        typedef C Wrapper;
   };
)";

    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testWeirdTemplateSpecialization.cpp"));
    REQUIRE(!Test_Util::usesInTheImplementationExists("S", "C", session));
    REQUIRE(!Test_Util::isAExists("S", "C", session));
}

TEST_CASE("Template typedef")
{
    // Make sure that fixes for testWeirdTemplateSpecialization
    // don't break other things
    const static char *source =
        R"(class C {};
   class D {};

   template <typename T>
   class E {
     public:
       typedef D foo;
   };

    class F {
        E<C> d;
    };
)";

    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testTemplateTypedef.cpp"));
    REQUIRE(!Test_Util::usesInTheInterfaceExists("E", "C", session));
    REQUIRE(!Test_Util::usesInTheImplementationExists("E", "C", session));
    REQUIRE(Test_Util::usesInTheInterfaceExists("E", "E::foo", session));
    REQUIRE(Test_Util::usesInTheImplementationExists("E::foo", "D", session));
    REQUIRE(Test_Util::usesInTheImplementationExists("F", "E", session));
    REQUIRE(Test_Util::usesInTheImplementationExists("F", "C", session));
}

TEST_CASE("Template field")
{
    const static char *source =
        R"(class C {};

   template <typename T>
   class D {};

   template <typename TYPE>
   class E {
       D<TYPE> d;
   };

   template <>
   class E<C>;
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testTemplateField.cpp"));
    REQUIRE(Test_Util::usesInTheImplementationExists("E", "D", session));
}

TEST_CASE("Static field in method")
{
    const static char *source = R"(
class C {
  public:
    static constexpr int i = 0;
};

class D {
    void method()
    {
        int local = C::i;
    }
};
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testStaticFieldInMethod.cpp"));
    REQUIRE(Test_Util::usesInTheImplementationExists("D", "C", session));
}

TEST_CASE("Static reference in static field")
{
    const static char *source = R"(
class Zero {
  public:
    static constexpr int val = 0;
};

class One {
  public:
    static constexpr int val()
    {
        return 1;
    }
};

class C {
  private:
    static constexpr int zero = Zero::val;

  public:
    static constexpr int one = One::val();
};
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testStaticRefInStaticField.cpp"));
    REQUIRE(Test_Util::usesInTheImplementationExists("C", "Zero", session));
    REQUIRE(Test_Util::usesInTheImplementationExists("C", "One", session));
}

TEST_CASE("Static reference in field")
{
    const static char *source = R"(
class Zero {
  public:
    static constexpr int val = 0;
};

class One {
  public:
    static constexpr int val()
    {
        return 1;
    }
};

class C {
  private:
    int zero = Zero::val;

  public:
    int one = One::val();
};
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testStaticRefInField.cpp"));

    REQUIRE(Test_Util::usesInTheImplementationExists("C", "Zero", session));
    REQUIRE(Test_Util::usesInTheImplementationExists("C", "One", session));
}

TEST_CASE("Call expr in field")
{
    const static char *source = R"(
class C;

template<class T>
bool templateFn();

class D {
    bool field = templateFn<C>();
};
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testCallExprInField.cpp"));
    REQUIRE(Test_Util::usesInTheImplementationExists("D", "C", session));
}

TEST_CASE("Method return")
{
    const static char *source = R"(
class C {};
class D {};
class E {};

class F {
  public:
    C methodC() { return C(); }
    static D methodD() { return D(); }

  private:
    E methodE() { return E(); }
};
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testMethodReturn.cpp"));

    REQUIRE(Test_Util::usesInTheInterfaceExists("F", "C", session));
    REQUIRE(Test_Util::usesInTheInterfaceExists("F", "D", session));
    REQUIRE(!Test_Util::usesInTheInterfaceExists("F", "E", session));
    REQUIRE(Test_Util::usesInTheImplementationExists("F", "C", session));
    REQUIRE(Test_Util::usesInTheImplementationExists("F", "D", session));
    REQUIRE(Test_Util::usesInTheImplementationExists("F", "E", session));
}

TEST_CASE("Lambda return")
{
    const static char *source = R"(
class C {};
class D { public: D(int i) {} };

class E {
  public:
    static constexpr auto lambdaC = []() { return C(); };
    static constexpr auto lambdaD = []() -> D { return 1; };
};
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testLambdaReturn.cpp"));

    REQUIRE(Test_Util::usesInTheInterfaceExists("E", "C", session));
    REQUIRE(Test_Util::usesInTheInterfaceExists("E", "D", session));
    REQUIRE(Test_Util::usesInTheImplementationExists("E", "C", session));
    // E doesn't use D in its implementation (we don't pick up the implicit cast)
}

TEST_CASE("Template method return")
{
    const static char *source = R"(
template <typename T>
class C {};
class D {};

class T {};

class E {
  public:
    template <typename T>
    C<T> method()
    {
        return C<T>();
    }
};

class F {
  public:
    C<D> method()
    {
        E e;
        return e.method<D>();
    }
};
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testTempalteMethodReturn.cpp"));
    REQUIRE(Test_Util::usesInTheInterfaceExists("E", "C", session));
    REQUIRE(!Test_Util::usesInTheInterfaceExists("E", "T", session));
    REQUIRE(!Test_Util::usesInTheInterfaceExists("E", "D", session));
    REQUIRE(!Test_Util::usesInTheImplementationExists("E", "T", session));
    REQUIRE(Test_Util::usesInTheImplementationExists("E", "C", session));
    REQUIRE(Test_Util::usesInTheInterfaceExists("F", "C", session));
    REQUIRE(Test_Util::usesInTheInterfaceExists("F", "D", session));
    REQUIRE(Test_Util::usesInTheImplementationExists("F", "E", session));
    REQUIRE(Test_Util::usesInTheImplementationExists("F", "D", session));
}

TEST_CASE("Template method qualified return")
{
    const static char *source = R"(
template <typename T>
class C {};
class D {};

class T {};

class E {
  public:
    template <typename T>
    const C<T> method()
    {
        return C<T>();
    }
};

class F {
  public:
    const C<D> method()
    {
        E e;
        return e.method<D>();
    }
};
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testTempalteMethodQualifiedReturn.cpp"));

    REQUIRE(Test_Util::usesInTheInterfaceExists("E", "C", session));
    REQUIRE(!Test_Util::usesInTheInterfaceExists("E", "T", session));
    REQUIRE(!Test_Util::usesInTheInterfaceExists("E", "D", session));
    REQUIRE(!Test_Util::usesInTheImplementationExists("E", "T", session));
    REQUIRE(Test_Util::usesInTheImplementationExists("E", "C", session));
    REQUIRE(Test_Util::usesInTheInterfaceExists("F", "C", session));
    REQUIRE(Test_Util::usesInTheInterfaceExists("F", "D", session));
    REQUIRE(Test_Util::usesInTheImplementationExists("F", "E", session));
    REQUIRE(Test_Util::usesInTheImplementationExists("F", "D", session));
}

TEST_CASE("Template qualified field")
{
    const static char *source =
        R"(class C {};

   template <typename T>
   class D {};

   template <typename TYPE>
   class E {
       volatile D<TYPE> d;
   };

   template <>
   class E<C>;
)";
    ObjectStore session;

    REQUIRE(Test_Util::runOnCode(session, source, "testTemplateQualifiedField.cpp"));
    REQUIRE(Test_Util::usesInTheImplementationExists("E", "D", session));
}

TEST_CASE("Method arg template specialization")
{
    const static char *source = R"(
class C;

template<typename T>
struct hash {
    int operator()(const T& t) const noexcept;
};

template<>
struct hash<C> {
    int operator()(const C& c) const noexcept
    {
        return 0;
    }
};
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testMethodArgTemplateSpecialization.cpp"));
    REQUIRE(!Test_Util::usesInTheInterfaceExists("hash", "C", session));
    REQUIRE(!Test_Util::usesInTheImplementationExists("hash", "C", session));
}

TEST_CASE("Static method template specialization")
{
    const static char *source = R"(
class C {};

template<typename T>
class numeric_limits {
    static T quiet_NaN() noexcept;
};

template<>
class numeric_limits<C> {
    static C quiet_NaN() noexcept;
};

C numeric_limits<C>::quiet_NaN() noexcept
{
    return C();
}
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testStaticMethodTemplateSpecialization.cpp"));
    REQUIRE(!Test_Util::usesInTheInterfaceExists("numeric_limits", "C", session));
    REQUIRE(!Test_Util::usesInTheImplementationExists("numeric_limits", "C", session));
}

TEST_CASE("Typedef template")
{
    const static char *source = R"(
template <typename T>
class C;

template <typename T>
class Foo {};

template <typename T>
class Foo<C<T>> {
  public:
    typedef C<T> Type;
};
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testTypedefTemplate.cpp"));
    REQUIRE(!Test_Util::usesInTheInterfaceExists("Foo", "C", session));
}

TEST_CASE("Default argument")
{
    const static char *source = R"(
class C {
  public:
    operator char()
    {
        return 'C';
    }
};

class D {
  public:
    void method(char c = C())
    {
    }
};
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testDefaultArgument.cpp"));
    REQUIRE(Test_Util::usesInTheImplementationExists("D", "C", session));
    REQUIRE(!Test_Util::usesInTheInterfaceExists("D", "C", session));
}

TEST_CASE("Decltype")
{
    // Check that we resolve decltype() when doing a type lookup
    const static char *source = R"(
class C {};

class D {
  public:
    static C getC();
};

class E {
  public:
    decltype(D::getC()) method()
    {
        return D::getC();
    }
};
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testDecltype.cpp"));

    REQUIRE(Test_Util::usesInTheInterfaceExists("E", "C", session));
    REQUIRE(Test_Util::usesInTheImplementationExists("E", "D", session));
}

TEST_CASE("Dependent decltype")
{
    // Check that we resolve decltype() when doing a type lookup
    const static char *source = R"(
template <class TYPE>
class C {};

template <class TYPE>
class D {
  public:
    static C<TYPE> getC();
};

template <class TYPE>
class E {
  public:
    decltype(D<TYPE>::getC()) method()
    {
        return D<TYPE>::getC();
    }
};
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testDependentDecltype.cpp"));

    // While it would be great to be able to infer E -> C, this isn't possible
    // in general because the type resulting from the expression could change
    // arbitrarily depending upon template specializations. In this example, there
    // could be a specialization for D<int> which returns bool from getC();
    // Therefore clang correctly refuses to resolve the type of a dependent
    // decltype expression.
    //
    // E -> D tests CXXDependentScopeMemberExpr in
    // CodebaseDbVisitor::processChildStatements
    REQUIRE(!Test_Util::usesInTheInterfaceExists("E", "C", session));
    REQUIRE(Test_Util::usesInTheImplementationExists("E", "D", session));
}

TEST_CASE("Using false positive")
{
    const static char *source = R"(
namespace foo {
class C;
}

using foo::C;
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testUsingFalsePositive.cpp"));

    // make sure we didn't add ::C as an alias at file scope
    session.withROLock([&] {
        REQUIRE(!session.getType("C"));
    });
}

TEST_CASE("Re-exported type")
{
    // based on bslstl_iterator and bslstl_vector
    const static char *source = R"(
namespace native_std {
template <class ITER>
class reverse_iterator;
}

namespace bsl {
using native_std::reverse_iterator;

template <class VALUE_TYPE>
class vectorBase {
  public:
    typedef VALUE_TYPE *iterator;
    typedef bsl::reverse_iterator<iterator> reverse_iterator;
};
}
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testReExportedType.cpp"));
    REQUIRE(Test_Util::usesInTheImplementationExists("bsl::reverse_iterator", "native_std::reverse_iterator", session));
    REQUIRE(Test_Util::usesInTheImplementationExists("bsl::vectorBase::reverse_iterator",
                                                     "bsl::reverse_iterator",
                                                     session));
}

TEST_CASE("Const typename")
{
    // based on bsls_atomicoperations_default
    const static char *source = R"(
struct AtomicTypes {
    typedef void * Pointer;
};

class C {
  public:
    void method(typename AtomicTypes::Pointer const *ptrPtr);
};
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testConstTypename.cpp"));
    REQUIRE(Test_Util::usesInTheInterfaceExists("C", "AtomicTypes::Pointer", session));
}

TEST_CASE("Double pointer to builtin")
{
    // Unfortunately I can't think of a way of observing in the database if we
    // did the right or the wrong thing here.
    // The engineer has to manually review output to check there are no WARN
    // messages about failed lookups
    static const char *source = R"(
class C {
    void method(void **ptr);
    void method2(void *array[2]);
};
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testDoublePointerToBuiltin.cpp"));
}

TEST_CASE("Atomic type")
{
    // based on bsls_buildtarget
    static const char *source = R"(
struct Types {
    typedef long long Int64;
};

struct AtomicInt {
    _Atomic(int) d_value;
};

struct AtomicInt64 {
    _Atomic(Types::Int64) d_value;
};
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testAtomicType.cpp"));
    REQUIRE(Test_Util::usesInTheInterfaceExists("AtomicInt64", "Types::Int64", session));
}

TEST_CASE("Forward declt type lookup")
{
    // the interesting thing is that clang says the type is called
    // "bsls::struct Types::Int64"
    static const char *source = R"(
namespace bsls {
struct Types {
    typedef long long Int64;
};
}

class C {
  public:
    void method(bsls::Types::Int64 arg);
};
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testForwardDeclTypeLookup.cpp"));
    REQUIRE(Test_Util::usesInTheInterfaceExists("C", "bsls::Types::Int64", session));
}

TEST_CASE("Fully qualified name lookup")
{
    // sometimes clang prints types only partially qualified, leading to database
    // lookup failure: e.g. in this case it looks up "Types::Int64" not
    // "bsls::Types::Int64". Check that we correct this.
    static const char *source = R"(
namespace bsls {
struct Types {
    typedef long long Int64;
};

class C {
  public:
    void method(Types::Int64 arg);
};
}
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testFullyQualifiedNameLookup.cpp"));
    REQUIRE(Test_Util::usesInTheInterfaceExists("bsls::C", "bsls::Types::Int64", session));
}

TEST_CASE("Buildint type array")
{
    static const char *source = R"(
class C {
    static const char string[];
};
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testBuiltInTypeArray.cpp"));
    // Unfortunately have to manually check for the failed lookup WARN messsage
}

TEST_CASE("Const ref typedef")
{
    static const char *source = R"(
namespace bslmt {

// bslmt_platform.h
struct Platform {
    struct PosixThreads {};
    typedef PosixThreads ThreadPolicy;
};

// bslmt_threadutilimpl_pthread.h

template <class THREAD_POLICY>
struct ThreadUtilImpl;

template <>
struct ThreadUtilImpl<Platform::PosixThreads> {
    typedef int Handle;
};

// bslmt_threadutil.h
struct ThreadUtil {
    typedef ThreadUtilImpl<Platform::ThreadPolicy> Imp;
    typedef Imp::Handle Handle;

    void method(const Handle&);
};
}
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testConstRefTypedef.cpp"));
    REQUIRE(Test_Util::usesInTheInterfaceExists("bslmt::ThreadUtil", "bslmt::ThreadUtil::Handle", session));
}

TEST_CASE("Bsls atomic operations")
{
    // bsls::AtomicTypes has been a repeated pain point when dealing with lookup
    // failures. Check we do the right thing
    static const char *source = R"(
namespace bsls {

// bsls_atomicoperations_default.h
template <class IMP>
struct Atomic_TypeTraits;

template <class IMP>
struct AtomicOperations_DefaultInt {
  public:
    typedef Atomic_TypeTraits<IMP> AtomicTypes;

  private:
    // Tricky lookup is here:
    static int getInt(typename AtomicTypes::Int const *atomicInt);
};

// bsls_atomicoperations_all_all_clangintrinsics.h
struct AtomicOperations_ALL_ALL_ClangIntrinsics;
typedef AtomicOperations_ALL_ALL_ClangIntrinsics AtomicOperations_Imp;

template <>
struct Atomic_TypeTraits<AtomicOperations_ALL_ALL_ClangIntrinsics>
{
    struct __attribute((__aligned__(sizeof(int)))) Int
    {
        _Atomic(int) d_value;
    };
};

struct AtomicOperations_ALL_ALL_ClangIntrinsics
    : AtomicOperations_DefaultInt<AtomicOperations_ALL_ALL_ClangIntrinsics>
{
    typedef Atomic_TypeTraits<AtomicOperations_ALL_ALL_ClangIntrinsics>
            AtomicTypes;

    static int getInt(const AtomicTypes::Int *atomicInt);
};

// bsls_atomicoperations.h
struct AtomicOperations {
    typedef AtomicOperations_Imp Imp;
    typedef Atomic_TypeTraits<Imp> AtomicTypes;

    static int getInt(AtomicTypes::Int const *atomicInt);
};

}
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testBslsAtomicOperations.cpp"));

    REQUIRE(Test_Util::usesInTheImplementationExists("bsls::AtomicOperations_ALL_ALL_ClangIntrinsics::AtomicTypes",
                                                     "bsls::Atomic_TypeTraits",
                                                     session));
    REQUIRE(Test_Util::usesInTheInterfaceExists("bsls::AtomicOperations_ALL_ALL_ClangIntrinsics",
                                                "bsls::AtomicOperations_ALL_ALL_ClangIntrinsics::AtomicTypes",
                                                session));
    REQUIRE(
        Test_Util::usesInTheImplementationExists("bsls::AtomicOperations::Imp", "bsls::AtomicOperations_Imp", session));
    REQUIRE(Test_Util::usesInTheInterfaceExists("bsls::AtomicOperations", "bsls::AtomicOperations::Imp", session));
    REQUIRE(Test_Util::usesInTheImplementationExists("bsls::AtomicOperations::AtomicTypes",
                                                     "bsls::Atomic_TypeTraits",
                                                     session));
    REQUIRE(
        Test_Util::usesInTheInterfaceExists("bsls::AtomicOperations", "bsls::AtomicOperations::AtomicTypes", session));
}

TEST_CASE("Local type alias")
{
    // check we don't add local type aliases to the global namespace
    static const char *source = R"(
struct RealType;

class C {
    void method()
    {
        typedef RealType TypedefType;
        using UsingType = RealType;

        TypedefType *foo;
        UsingType *bar;
    }
};
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testLocalTypeAlias.cpp"));

    session.withROLock([&] {
        REQUIRE(!session.getType("TypedefType"));
        REQUIRE(!session.getType("UsingType"));
        REQUIRE(Test_Util::usesInTheImplementationExists("C", "RealType", session));
    });
}

TEST_CASE("Allocator traits")
{
    ObjectStore session;
    static const char *allocTraitSrc = R"(
namespace bslma {
template <class ALLOCATOR_TYPE>
struct AllocatorTraits_SomeTrait {
    static const bool value = false;
};
}
)";
    REQUIRE(Test_Util::runOnCode(session, allocTraitSrc, "bsl/bslma/bslma_allocatortraits.cpp"));

    static const char *foobarSrc = R"(
namespace foobar {
class C {};
}

namespace bslma {
template <class ALLOCATOR_TYPE>
struct AllocatorTraits_SomeTrait;

template<>
struct AllocatorTraits_SomeTrait<foobar::C> {
    static const bool value = true;
};
}
)";
    REQUIRE(Test_Util::runOnCode(session, foobarSrc, "foo/foobar/foobar_c.cpp"));

    REQUIRE(!Test_Util::usesInTheImplementationExists("foobar::C", "bslma::AllocatorTraits_SomeTrait", session));
    REQUIRE(!Test_Util::usesInTheInterfaceExists("foobar::C", "bslma::AllocatorTraits_SomeTrait", session));
    REQUIRE(!Test_Util::usesInTheImplementationExists("bslma::AllocatorTraits_SomeTrait", "foobar::C", session));
    REQUIRE(!Test_Util::usesInTheInterfaceExists("bslma::AllocatorTraits_SomeTrait", "foobar::C", session));
}
