// ct_lvtqtc_ellipsistextitem.h                                       -*-C++-*-

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

#ifndef INCLUDED_CT_LVTQTC_ELLIPSISTEXTITEM
#define INCLUDED_CT_LVTQTC_ELLIPSISTEXTITEM

#include <lvtqtc_export.h>

#include <QGraphicsSimpleTextItem>
#include <memory>

namespace Codethink::lvtqtc {

// =======================
// class EllipsisTextItem
// =======================

class LVTQTC_EXPORT EllipsisTextItem : public QGraphicsSimpleTextItem {
  public:
    enum class Truncate : short { Yes, No };

    // CREATORS
    EllipsisTextItem(const QString& text = QString(), QGraphicsItem *parent = nullptr);
    ~EllipsisTextItem();

    // MODIFIERS
    void setText(const QString& text);
    void truncate(Truncate v);
    struct Private;

  private:
    std::unique_ptr<Private> d;

    void setupText();
    // called after a setText or a truncate
};

} // namespace Codethink::lvtqtc

#endif // INCLUDED_CT_LVTQTC_ELLIPSISTEXTITEM
