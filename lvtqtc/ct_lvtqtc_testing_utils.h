// ct_lvtqtc_testing_utils.h                                                                                   -*-C++-*-

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

#ifndef DIAGRAM_SERVER_CT_LVTQTC_TESTING_UTILS_H
#define DIAGRAM_SERVER_CT_LVTQTC_TESTING_UTILS_H

#include <ct_lvtprj_projectfile.h>
#include <ct_lvtqtc_graphicsview.h>
#include <ct_lvtqtc_lakosentity.h>
#include <ct_lvtqtc_lakosrelation.h>

#include <QPoint>
#include <functional>

namespace Codethink::lvtprj {

class ProjectFileForTesting : public ProjectFile {
  public:
    [[nodiscard]] std::filesystem::path sourceCodePath() const override
    {
        return "";
    }
};

} // namespace Codethink::lvtprj

namespace Codethink::lvtqtc {

class GraphicsViewWrapperForTesting : public Codethink::lvtqtc::GraphicsView {
  public:
    explicit GraphicsViewWrapperForTesting(lvtldr::NodeStorage& nodeStorage);
    [[nodiscard]] QPoint getEntityPosition(lvtshr::UniqueId uid) const;
    void moveEntityTo(lvtshr::UniqueId uid, QPointF newPos) const;
    [[nodiscard]] bool hasEntityWithId(lvtshr::UniqueId uid) const;
    [[nodiscard]] int countEntityChildren(lvtshr::UniqueId uid) const;
    [[nodiscard]] bool hasRelationWithId(lvtshr::UniqueId uidFrom, lvtshr::UniqueId uidTo) const;

  private:
    void findEntityAndRun(lvtshr::UniqueId uid,
                          std::function<void(lvtqtc::LakosEntity&)> const& f,
                          bool assertOnNotFound = true) const;
    void findRelationAndRun(lvtshr::UniqueId fromUid,
                            lvtshr::UniqueId toUid,
                            std::function<void(lvtqtc::LakosRelation&)> const& f,
                            bool assertOnNotFound = true) const;

    lvtprj::ProjectFileForTesting projectFileForTesting;
};

bool mousePressAt(ITool& tool, QPointF pos);
bool mouseMoveTo(ITool& tool, QPointF pos);
bool mouseReleaseAt(ITool& tool, QPointF pos);

} // namespace Codethink::lvtqtc

#endif // DIAGRAM_SERVER_CT_LVTQTC_TESTING_UTILS_H
