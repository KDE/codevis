// ct_lvtclp_testudt.t.cpp                                           -*-C++-*-

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

// This file is home to tests for adding user defined types to the database

#include <ct_lvtmdb_fileobject.h>
#include <ct_lvtmdb_methodobject.h>
#include <ct_lvtmdb_namespaceobject.h>
#include <ct_lvtmdb_objectstore.h>
#include <ct_lvtmdb_typeobject.h>

#include <ct_lvtclp_testutil.h>
#include <ct_lvtshr_graphenums.h>

#include <algorithm>
#include <initializer_list>
#include <string>
#include <vector>

#include <catch2/catch.hpp>

using namespace Codethink::lvtclp;
using namespace Codethink::lvtmdb;
using namespace Codethink;

using UDTKind = Codethink::lvtshr::UDTKind;

const PyDefaultGilReleasedContext defaultGilContextForTesting;

TEST_CASE("User defined type")
{
    const static char *source = "namespace foo { class C {}; class D; }";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testUserDefinedType.cpp"));

    TypeObject *C = nullptr;
    TypeObject *D = nullptr;
    NamespaceObject *foo = nullptr;
    FileObject *file = nullptr;

    session.withROLock([&] {
        C = session.getType("foo::C");
        D = session.getType("foo::D");
        foo = session.getNamespace("foo");
        file = session.getFile("testUserDefinedType.cpp");
    });

    REQUIRE(C);
    REQUIRE(D);
    REQUIRE(foo);
    REQUIRE(file);

    C->withROLock([&] {
        REQUIRE(C->name() == "C");
        REQUIRE(C->kind() == UDTKind::Class);
        REQUIRE(C->access() == lvtshr::AccessSpecifier::e_none);
        REQUIRE(C->parentNamespace() == foo);
        REQUIRE(!C->parent());
    });

    D->withROLock([&] {
        REQUIRE(D->name() == "D");
        REQUIRE(D->kind() == UDTKind::Class);
        REQUIRE(D->access() == lvtshr::AccessSpecifier::e_none);
        REQUIRE(D->parentNamespace() == foo);
        REQUIRE(!D->parent());
    });

    foo->withROLock([&] {
        const auto classChildren = foo->typeChildren();
        auto childIt = std::find(classChildren.begin(), classChildren.end(), C);
        REQUIRE(childIt != classChildren.end());
        childIt = std::find(classChildren.begin(), classChildren.end(), D);
        REQUIRE(childIt != classChildren.end());
    });

    file->withROLock([&] {
        const auto classes = file->types();
        auto classes_it = std::find(classes.begin(), classes.end(), C);
        REQUIRE(classes_it != classes.end());
        classes_it = std::find(classes.begin(), classes.end(), D);
        REQUIRE(classes_it != classes.end());
    });
    C->withROLock([&] {
        auto files = C->files();
        auto files_it = std::find(files.begin(), files.end(), file);
        REQUIRE(files_it != files.end());
    });

    D->withROLock([&] {
        auto files = D->files();
        auto files_it = std::find(files.begin(), files.end(), file);
        REQUIRE(files_it != files.end());
    });
}

TEST_CASE("Struct declaration")
{
    const static char *source = "struct S {};";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testStructDeclaration.cpp"));

    // assume things we tested in testUserDefinedType are still good here
    TypeObject *S = nullptr;
    session.withROLock([&] {
        S = session.getType("S");
    });
    REQUIRE(S);

    S->withROLock([&] {
        REQUIRE(S->kind() == UDTKind::Struct);
    });
}

TEST_CASE("Union declaration")
{
    const static char *source = "union U {};";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testUnionDeclaration.cpp"));

    TypeObject *U = nullptr;
    session.withROLock([&] {
        U = session.getType("U");
    });
    // assume things we tested in testUserDefinedType are still good here
    REQUIRE(U);

    U->withROLock([&] {
        REQUIRE(U->kind() == UDTKind::Union);
    });
}

TEST_CASE("Local class")
{
    const static char *source =
        R"(void fn()
   {
       class C {};
   }
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testLocalClass.cpp"));

    session.withROLock([&] {
        REQUIRE(session.types().empty());
    });

    // we skip local classes
}

TEST_CASE("Anon class")
{
    const static char *source = "class {} foo;";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testAnonClass.cpp"));
    session.withROLock([&] {
        REQUIRE(session.types().empty());
    });

    // we skip anon classes
}

TEST_CASE("Nested classes")
{
    const static char *source = R"(
namespace foo {

class C {
  public:
    class Pub {};

  protected:
    class Prot {};

  private:
    class Priv {};
};

}
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testtNestedClasses.cpp"));

    TypeObject *C = nullptr;
    TypeObject *Pub = nullptr;
    TypeObject *Prot = nullptr;
    TypeObject *Priv = nullptr;
    NamespaceObject *foo = nullptr;

    session.withROLock([&] {
        C = session.getType("foo::C");
        Pub = session.getType("foo::C::Pub");
        Prot = session.getType("foo::C::Prot");
        Priv = session.getType("foo::C::Priv");
        foo = session.getNamespace("foo");
    });

    REQUIRE(Priv);
    REQUIRE(Prot);
    REQUIRE(Pub);
    REQUIRE(C);
    REQUIRE(foo);

    for (const auto& [klass, access] :
         std::map<TypeObject *, lvtshr::AccessSpecifier>{{Pub, lvtshr::AccessSpecifier::e_public},
                                                         {Prot, lvtshr::AccessSpecifier::e_protected},
                                                         {Priv, lvtshr::AccessSpecifier::e_private}}) {
        klass->withROLock([&, klass = klass, access = access] {
            REQUIRE(klass->parentNamespace() == foo);
            REQUIRE(klass->parent() == C);
            REQUIRE(klass->access() == access);
        });
    }
}

TEST_CASE("Is-A")
{
    const static char *source =
        R"(class C {};
   class D : public C {};
)";

    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testIsA.cpp"));
    REQUIRE(Test_Util::isAExists("D", "C", session));
}

TEST_CASE("Virtual Is-A")
{
    const static char *source =
        R"(class C {};
   class D: public virtual C {};
)";

    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testVirtualIsA.cpp"));
    REQUIRE(Test_Util::isAExists("D", "C", session));
}

TEST_CASE("Template Is-A")
{
    const static char *source =
        R"(class C {};

   template <typename T>
   class D {
       T t;
   };

   class E: public D<C> {
   };
)";

    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testTemplateIsA.cpp"));

    REQUIRE(Test_Util::isAExists("E", "D", session));
    REQUIRE(Test_Util::usesInTheInterfaceExists("E", "C", session));
    REQUIRE(!Test_Util::usesInTheImplementationExists("D", "T", session));
}

TEST_CASE("Template virtual Is-A")
{
    const static char *source =
        R"(class C {};

   template <typename T>
   class D {
       T t;
   };

   class E: public virtual D<C> {
   };
)";

    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testTemplateVirtualIsA.cpp"));

    REQUIRE(Test_Util::isAExists("E", "D", session));
    REQUIRE(Test_Util::usesInTheInterfaceExists("E", "C", session));
    REQUIRE(!Test_Util::usesInTheImplementationExists("D", "T", session));
}

TEST_CASE("Uses in method arguments")
{
    static const char *source = R"(
class C {};
class D {
    void method(const C& c)
    {
    }
};
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testUsesInMethodArguments.cpp"));

    TypeObject *C = nullptr;
    TypeObject *D = nullptr;
    MethodObject *method = nullptr;

    session.withROLock([&] {
        C = session.getType("C");
        D = session.getType("D");
        method = session.getMethod("D::method", "method(const class C & c)", std::string{}, "void");
    });

    REQUIRE(D);
    REQUIRE(C);
    REQUIRE(method);

    C->withROLock([&] {
        REQUIRE(C->methods().empty());

        // TODO: Make sure this is needed - We are not using this anywhere.
        /*  Not implemented in LVTMDB
                const auto usesInMethodArgs = C->usesInMethodArguments();
                REQUIRE(usesInMethodArgs.size() == 1);
                it = std::find(usesInMethodArgs.begin(), usesInMethodArgs.end(), method);
                REQUIRE(it != methods.end());
        */
    });

    D->withROLock([&] {
        const auto methods = D->methods();
        REQUIRE(methods.size() == 1);
        auto it = std::find(methods.begin(), methods.end(), method);
        REQUIRE(it != methods.end());

        // TODO: Make sure this is needed.
        // Not Implemented in LVTMDB
        // REQUIRE(D->usesInMethodArguments().empty());
    });
}

TEST_CASE("Uses in field type")
{
    static const char *source = R"(
class C {};
class D {
    C field;
};
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testUsesInFieldType.cpp"));
    TypeObject *C = nullptr;
    TypeObject *D = nullptr;
    FieldObject *field = nullptr;

    session.withROLock([&] {
        C = session.getType("C");
        D = session.getType("D");
        field = session.getField("D::field");
    });

    REQUIRE(C);
    REQUIRE(D);
    REQUIRE(field);

    D->withRWLock([&] {
        const auto fields = D->fields();
        REQUIRE(fields.size() == 1);
        auto it = std::find(fields.begin(), fields.end(), field);
        REQUIRE(it != fields.end());
        // REQUIRE(D->usesInFieldType().empty());
    });

    C->withROLock([&] {
    // Not implemented yet on lvtmdb.
#if 0
        const auto usesInFieldTypes = C->usesInFieldType();
        REQUIRE(usesInFieldTypes.size() == 1);
        it = std::find(usesInFieldTypes.begin(), usesInFieldTypes.end(), field);
        REQUIRE(it != usesInFieldTypes.end());
#endif
        REQUIRE(C->fields().empty());
    });
}

TEST_CASE("Uses in field array type")
{
    static const char *source = R"(
class C {};
class D {
    C field[3];
};
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testUsesInFieldType.cpp"));

    TypeObject *C = nullptr;
    TypeObject *D = nullptr;
    FieldObject *field = nullptr;

    session.withROLock([&] {
        C = session.getType("C");
        D = session.getType("D");
        field = session.getField("D::field");
    });

    REQUIRE(C);
    REQUIRE(D);
    REQUIRE(field);

    D->withRWLock([&] {
        const auto fields = D->fields();
        REQUIRE(fields.size() == 1);
        auto it = std::find(fields.begin(), fields.end(), field);
        REQUIRE(it != fields.end());
        // REQUIRE(D->usesInFieldType().empty());
    });

    C->withROLock([&] {
    // Not implemented yet on lvtmdb.
#if 0
        const auto usesInFieldTypes = C->usesInFieldType();
        REQUIRE(usesInFieldTypes.size() == 1);
        it = std::find(usesInFieldTypes.begin(), usesInFieldTypes.end(), field);
        REQUIRE(it != usesInFieldTypes.end());
#endif
        REQUIRE(C->fields().empty());
    });

    REQUIRE(Test_Util::usesInTheImplementationExists("D", "C", session));
}

TEST_CASE("Class template")
{
    static const char *source = R"(
template <typename T>
class C {
};

C<int> c;
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testClassTemplate.cpp"));

    session.withROLock([&] {
        REQUIRE(session.getType("C"));
        REQUIRE(session.types().size() == 1);
    });

    // check we didn't separately add C<int>
}

TEST_CASE("Class non type template")
{
    static const char *source = R"(
template <unsigned N>
class CharArray {
    char arr[N];
};

CharArray<1> arr;
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testClassNonTypeTemplate.cpp"));
    session.withROLock([&] {
        TypeObject *CharArray = session.getType("CharArray");
        REQUIRE(CharArray);

        // check we didn't separately add CharArray<1>
        REQUIRE(session.types().size() == 1);
    });
}

TEST_CASE("Class template template")
{
    static const char *source = R"(
template <class T>
class C {};

template <template<class> class T>
class ApplyChar {
    T<char> t;
};

ApplyChar<C> a;
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testClassTemplateTemplate.cpp"));
    session.withROLock([&] {
        REQUIRE(session.getType("C"));
        REQUIRE(session.getType("ApplyChar"));
        REQUIRE(session.types().size() == 2);
    });

    // check we didn't separately add ApplyChar<C>
}

TEST_CASE("Type aliase namespace")
{
    static const char *source = R"(
namespace std {
class Something;
}

namespace foo {
using Something = std::Something;
}

class C {
  public:
    foo::Something& some;
};
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testTypeAliasNamespace.cpp"));

    NamespaceObject *foo = nullptr;
    TypeObject *fooSomething = nullptr;
    FileObject *file = nullptr;

    session.withROLock([&] {
        foo = session.getNamespace("foo");
        fooSomething = session.getType("foo::Something");
        file = session.getFile("testTypeAliasNamespace.cpp");
    });

    REQUIRE(fooSomething);
    REQUIRE(foo);
    REQUIRE(file);

    fooSomething->withROLock([&] {
        REQUIRE(fooSomething->name() == "Something");
        REQUIRE(fooSomething->kind() == UDTKind::TypeAlias);
        REQUIRE(fooSomething->access() == lvtshr::AccessSpecifier::e_none);
        REQUIRE(fooSomething->parentNamespace() == foo);
        REQUIRE(!fooSomething->parent());
    });

    file->withROLock([&] {
        const auto udts = file->types();
        const auto udts_it = std::find(udts.begin(), udts.end(), fooSomething);
        REQUIRE(udts_it != udts.end());
    });

    REQUIRE(Test_Util::usesInTheImplementationExists("foo::Something", "std::Something", session));
    REQUIRE(Test_Util::usesInTheInterfaceExists("C", "foo::Something", session));
}

TEST_CASE("enum")
{
    static const char *source = R"(
namespace foo {
struct Util {
    enum Enum {
        e_ONE = 1,
        e_TWO,
    };
};

enum class Byte : unsigned char {};
}
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testEnum.cpp"));

    TypeObject *unscoped = nullptr;
    TypeObject *scoped = nullptr;
    TypeObject *util = nullptr;
    FileObject *file = nullptr;
    NamespaceObject *foo = nullptr;

    session.withROLock([&] {
        unscoped = session.getType("foo::Util::Enum");
        scoped = session.getType("foo::Byte");
        util = session.getType("foo::Util");
        file = session.getFile("testEnum.cpp");
        foo = session.getNamespace("foo");
    });

    REQUIRE(unscoped);
    REQUIRE(scoped);
    REQUIRE(util);
    REQUIRE(file);
    REQUIRE(foo);

    unscoped->withROLock([&] {
        REQUIRE(unscoped->name() == "Enum");
    });

    scoped->withROLock([&] {
        REQUIRE(scoped->name() == "Byte");
    });

    std::vector<TypeObject *> udts;
    file->withROLock([&] {
        udts = file->types();
    });

    for (const auto& e : {unscoped, scoped}) {
        e->withROLock([&] {
            REQUIRE(e->kind() == UDTKind::Enum);
            const auto udts_it = std::find(udts.begin(), udts.end(), e);
            REQUIRE(udts_it != udts.end());
            REQUIRE(e->parentNamespace() == foo);
        });
    }

    unscoped->withROLock([&] {
        REQUIRE(unscoped->parent() == util);
        REQUIRE(unscoped->access() == lvtshr::AccessSpecifier::e_public);
    });

    scoped->withROLock([&] {
        REQUIRE(!scoped->parent());
        REQUIRE(scoped->access() == lvtshr::AccessSpecifier::e_none);
    });
}

TEST_CASE("Internal linkage")
{
    static const char *source = R"(
namespace {
class C {};
typedef C D;

namespace inner {
    class Nested1 {
        class NestedSquared {};

        typedef class NestedSquared NS;
    };

    typedef Nested1 N;
}
}

namespace outer {
    namespace {
        class Nested2 {
            class NestedSquared2 {};
            typedef NestedSquared2 NS2;
        };

        typedef Nested2 N2;
    }
})";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testInternalLinkage.cpp"));

    session.withROLock([&] {
        REQUIRE(session.types().empty());
    });
}

TEST_CASE("Weird templates")
{
    // mostly based on bdlat_typetraits.h
    static const char *source = R"(
// baljsn_encodingstyle.h
namespace baljsn {
class EncodingStyle {
  public:
    enum Value {
      e_COMPACT = 0,
      e_PRETTY = 1
    };

    static int fromInt(Value *result, int number);
};
}

template <class TYPE>
struct bdlat_BasicEnumerationWrapper;

#define BDLAT_DECL_ENUMERATION_TRAITS(ClassName)                              \
    template <>                                                               \
    struct bdlat_BasicEnumerationWrapper<ClassName::Value> : ClassName {      \
        typedef ClassName Wrapper;                                            \
    };

BDLAT_DECL_ENUMERATION_TRAITS(baljsn::EncodingStyle)

template <class TYPE>
int bdlat_enumFromInt(TYPE *result, int number)
{
    typedef typename bdlat_BasicEnumerationWrapper<TYPE>::Wrapper Wrapper;
    return Wrapper::fromInt(result, number);
}

int main()
{
    baljsn::EncodingStyle::Value style;
    return bdlat_enumFromInt(&style, 1); // force template instantiation
}
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testWeirdTemplates.cpp"));

    session.withROLock([&] {
        auto *bad = session.getType("bdlat_BasicEnumerationWrapper<baljsn::EncodingStyle::Value>::Wrapper");
        REQUIRE(!bad);
    });
}
