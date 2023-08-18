// ct_lvtqtc_ellipsistextitem.t.cpp                                   -*-C++-*-

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

#include <catch2-local-includes.h>
#include <ct_lvtqtc_ellipsistextitem.h>
#include <ct_lvttst_fixture_qt.h>

using namespace Codethink::lvtqtc;

TEST_CASE_METHOD(QTApplicationFixture, "Complete Graph")
{
    auto someText = EllipsisTextItem{"veryverylongtext"};
    REQUIRE(someText.text() == "veryverylongtex...");
    someText.truncate(EllipsisTextItem::Truncate::No);
    REQUIRE(someText.text() == "veryverylongtext");
    someText.setText("veryverylongtext__evenlonger");
    REQUIRE(someText.text() == "veryverylongtext__evenlonger");
}
