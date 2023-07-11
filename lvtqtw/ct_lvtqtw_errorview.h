// ct_lvtqtw_errorview.h                               -*-C++-*-

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

#ifndef DEFINED_CT_LVTQTW_ERRORVIEW_H
#define DEFINED_CT_LVTQTW_ERRORVIEW_H

#include <lvtqtw_export.h>

#include <QWidget>
#include <memory>

namespace Codethink::lvtmdl {
class ErrorsModel;
}
namespace Ui {
class ErrorView;
}

namespace Codethink::lvtqtw {

class LVTQTW_EXPORT ErrorView : public QWidget {
    Q_OBJECT
  public:
    ErrorView(QWidget *parent = nullptr);
    ~ErrorView() override;

    void setModel(lvtmdl::ErrorsModel *model);

    void saveColumnVisibilityState();
    void loadColumnVisibilityState();
    void createColumnVisibilityMenu(const QPoint& pos);

  private:
    struct Private;
    std::unique_ptr<Ui::ErrorView> ui;
    std::unique_ptr<Private> d;
};

} // namespace Codethink::lvtqtw

#endif
