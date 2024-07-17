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

#include "logicaldepscanner.h"

bool runOnCode(const std::string& source, const std::string& fileName = "file.cpp")
{
    LogicalDepActionFactory actionFactory;

    auto frontendAction = actionFactory.create();

    const std::vector<std::string> args{
        "-std=c++17", // allow nested namespaces
    };

    bool res = clang::tooling::runToolOnCodeWithArgs(std::move(frontendAction), source, args, fileName);
    if (!res) {
        return res;
    }

    return true;
}

int main()
{
    const static char *source =
        R"(class C {};

   class D {
     public:
       template <typename T>
       static void some_method(T t, int a = 0)
       {
           C c;
       }
   };

   class E {
     public:
       void some_method()
       {
           D::some_method(1);
       }
   };
)";

    runOnCode(source, "testTemplateMethod.cpp");
}
