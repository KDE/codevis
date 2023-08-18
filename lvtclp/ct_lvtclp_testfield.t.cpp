// ct_lvtclp_testfield.t.cpp                                           -*-C++-*-

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

// This file is for tests for CodebaseDbVisitor::VisitFieldDecl

#include <ct_lvtmdb_fieldobject.h>
#include <ct_lvtmdb_objectstore.h>
#include <ct_lvtmdb_typeobject.h>

#include <ct_lvtclp_testutil.h>
#include <ct_lvtshr_graphenums.h>

#include <algorithm>
#include <cstdlib>
#include <iostream>

#include <catch2-local-includes.h>

using namespace Codethink::lvtclp;
using namespace Codethink::lvtmdb;
using namespace Codethink;

const PyDefaultGilReleasedContext defaultGilContextForTesting;

TEST_CASE("Field declaration")
{
    const static char *source = R"(
class C {
    int i;
};
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testFieldDecl.cpp"));

    TypeObject *C = nullptr;
    FieldObject *i = nullptr;
    session.withROLock([&] {
        C = session.getType("C");
        i = session.getField("C::i");
    });
    REQUIRE(C);
    REQUIRE(i);

    i->withROLock([&] {
        REQUIRE(i->name() == "i");
        REQUIRE(i->signature() == "int");
        REQUIRE(i->access() == lvtshr::AccessSpecifier::e_private);
        REQUIRE(!i->isStatic());
        REQUIRE(i->parent() == C);

        // int is not a class
        REQUIRE(i->variableTypes().empty());
    });
}

TEST_CASE("Field signatures")
{
    const static char *source = R"(
class C {
    int val;
    int *ptr;
    int& ref = val;

    const int constVal = 0;
    volatile int volatileVal;
};
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testFieldSignatures.cpp"));

    FieldObject *val = nullptr;
    FieldObject *ptr = nullptr;
    FieldObject *ref = nullptr;
    FieldObject *constVal = nullptr;
    FieldObject *volatileVal = nullptr;
    session.withROLock([&] {
        val = session.getField("C::val");
        ptr = session.getField("C::ptr");
        ref = session.getField("C::ref");
        constVal = session.getField("C::constVal");
        volatileVal = session.getField("C::volatileVal");
    });

    REQUIRE(val);
    REQUIRE(ptr);
    REQUIRE(ref);
    REQUIRE(constVal);
    REQUIRE(volatileVal);

    val->withROLock([&] {
        REQUIRE(val->signature() == "int");
    });

    ptr->withROLock([&] {
        REQUIRE(ptr->signature() == "int *");
    });

    ref->withROLock([&] {
        REQUIRE(ref->signature() == "int &");
    });

    constVal->withROLock([&] {
        REQUIRE(constVal->signature() == "const int");
    });

    volatileVal->withROLock([&] {
        REQUIRE(volatileVal->signature() == "volatile int");
    });
}

TEST_CASE("Field access")
{
    const static char *source = R"(
class C {
  public:
    int pub;

  protected:
    int prot;

  private:
    int priv;
};
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testFieldAccess.cpp"));

    using AS = lvtshr::AccessSpecifier;

    FieldObject *pub = nullptr;
    FieldObject *prot = nullptr;
    FieldObject *priv = nullptr;

    session.withROLock([&] {
        pub = session.getField("C::pub");
        prot = session.getField("C::prot");
        priv = session.getField("C::priv");
    });
    REQUIRE(pub);
    REQUIRE(prot);
    REQUIRE(priv);

    pub->withROLock([&] {
        REQUIRE(pub->access() == AS::e_public);
    });

    prot->withROLock([&] {
        REQUIRE(prot->access() == AS::e_protected);
    });

    priv->withROLock([&] {
        REQUIRE(priv->access() == AS::e_private);
    });
}

TEST_CASE("Field relations")
{
    const static char *source = R"(
class Pub {};
class Prot {};
class Priv {};

class C {
  public:
    Pub pub;

  protected:
    Prot prot;

  private:
    Priv priv;
};
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testFieldRelations.cpp"));

    REQUIRE(Test_Util::usesInTheInterfaceExists("C", "Pub", session));
    REQUIRE(!Test_Util::usesInTheImplementationExists("C", "Pub", session));
    REQUIRE(Test_Util::usesInTheInterfaceExists("C", "Prot", session));
    REQUIRE(!Test_Util::usesInTheImplementationExists("C", "Prot", session));
    REQUIRE(!Test_Util::usesInTheInterfaceExists("C", "Priv", session));
    REQUIRE(Test_Util::usesInTheImplementationExists("C", "Priv", session));
}

TEST_CASE("Static fields")
{
    const static char *source = R"(
class C {
    static int i;
};
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testStaticField.cpp"));

    FieldObject *i = nullptr;
    session.withROLock([&] {
        i = session.getField("C::i");
    });
    REQUIRE(i);

    i->withROLock([&] {
        REQUIRE(i->isStatic());
    });
}

TEST_CASE("Field types")
{
    const static char *source = R"(
class C {};
class D {
    C c;
};
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testFieldTypes.cpp"));

    TypeObject *cType = nullptr;
    FieldObject *cField = nullptr;
    session.withROLock([&] {
        cType = session.getType("C");
        cField = session.getField("D::c");
    });
    REQUIRE(cType);
    REQUIRE(cField);

    cField->withROLock([&] {
        const auto variableTypes = cField->variableTypes();
        REQUIRE(variableTypes.size() == 1);
        REQUIRE(*variableTypes.begin() == cType);
    });
}

TEST_CASE("Field template")
{
    const static char *source = R"(
class T {};

template <typename T>
class C {};

template <typename T>
class D {};

class E {};

class F {
    C<D<E>> cde;
};
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testFieldTemplate.cpp"));

    TypeObject *C = nullptr;
    TypeObject *D = nullptr;
    TypeObject *E = nullptr;
    FieldObject *cde = nullptr;
    session.withROLock([&] {
        C = session.getType("C");
        D = session.getType("D");
        E = session.getType("E");
        cde = session.getField("F::cde");
    });
    REQUIRE(C);
    REQUIRE(D);
    REQUIRE(E);
    REQUIRE(cde);

    cde->withROLock([&] {
        REQUIRE(cde->signature() == "C<D<class E> >");
        const auto variableTypes = cde->variableTypes();
        REQUIRE(variableTypes.size() == 3);
        auto it = std::find(variableTypes.begin(), variableTypes.end(), C);
        REQUIRE(it != variableTypes.end());
        it = std::find(variableTypes.begin(), variableTypes.end(), D);
        REQUIRE(it != variableTypes.end());
        it = std::find(variableTypes.begin(), variableTypes.end(), E);
        REQUIRE(it != variableTypes.end());
    });

    REQUIRE(Test_Util::usesInTheImplementationExists("F", "C", session));
    REQUIRE(Test_Util::usesInTheImplementationExists("F", "D", session));
    REQUIRE(Test_Util::usesInTheImplementationExists("F", "E", session));
    REQUIRE(!Test_Util::usesInTheImplementationExists("C", "T", session));
    REQUIRE(!Test_Util::usesInTheImplementationExists("D", "T", session));
    REQUIRE(!Test_Util::usesInTheImplementationExists("E", "T", session));
    REQUIRE(!Test_Util::usesInTheImplementationExists("F", "T", session));
}

TEST_CASE("Field class template")
{
    const static char *source = R"(
class T {};

template <typename T>
class C {
    T t;
};

class D {};

class E {
    C<D> cd;
};
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testFieldClassTemplate.cpp"));

    TypeObject *C = nullptr;
    TypeObject *D = nullptr;
    FieldObject *t = nullptr;
    FieldObject *cd = nullptr;

    session.withROLock([&] {
        C = session.getType("C");
        D = session.getType("D");
        t = session.getField("C::t");
        cd = session.getField("E::cd");
    });

    REQUIRE(C);
    REQUIRE(D);
    REQUIRE(t);
    REQUIRE(cd);

    t->withROLock([&] {
        REQUIRE(t->signature() == "T");
        REQUIRE(t->variableTypes().empty());
    });

    cd->withROLock([&] {
        REQUIRE(cd->signature() == "C<class D>");
        const auto cdVariableTypes = cd->variableTypes();
        REQUIRE(cdVariableTypes.size() == 2);
        auto it = std::find(cdVariableTypes.begin(), cdVariableTypes.end(), C);
        REQUIRE(it != cdVariableTypes.end());
        it = std::find(cdVariableTypes.begin(), cdVariableTypes.end(), D);
        REQUIRE(it != cdVariableTypes.end());
    });

    REQUIRE(Test_Util::usesInTheImplementationExists("E", "C", session));
    REQUIRE(Test_Util::usesInTheImplementationExists("E", "D", session));
    REQUIRE(!Test_Util::usesInTheImplementationExists("C", "T", session));
}

TEST_CASE("Anon field")
{
    const static char *source = R"(
struct Foo {
    unsigned one;
    int :32; // padding
    unsigned two;
};
)";
    ObjectStore session;
    REQUIRE(Test_Util::runOnCode(session, source, "testAnonField.cpp"));

    TypeObject *foo = nullptr;
    session.withROLock([&] {
        foo = session.getType("Foo");
    });

    REQUIRE(foo);
    foo->withROLock([&] {
        REQUIRE(foo->fields().size() == 2);
    });
}
