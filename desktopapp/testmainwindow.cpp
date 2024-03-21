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

#include <testmainwindow.h>

namespace CodevisApplicationTesting {

TestMainWindow::TestMainWindow(Codethink::lvtldr::NodeStorage& sharedNodeStorage,
                               Codethink::lvtqtc::UndoManager *undoManager,
                               Codethink::lvtmdl::DebugModel *debugModel):
    MainWindow(sharedNodeStorage, nullptr, undoManager, debugModel)
{
}

QString TestMainWindow::requestProjectName()
{
    return QStringLiteral("__test_project__");
}

} // namespace CodevisApplicationTesting
