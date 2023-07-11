// ct_lvtplg_sharedlibrarydispatcher.h                               -*-C++-*-

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

#ifndef DIAGRAM_SERVER_CT_LVTPLG_SHAREDLIBRARYDISPATCHER_H
#define DIAGRAM_SERVER_CT_LVTPLG_SHAREDLIBRARYDISPATCHER_H

#include <ct_lvtplg_librarydispatcherinterface.h>

#include <QDir>
#include <QLibrary>
#include <QString>

#include <memory>
#include <string>

namespace Codethink::lvtplg {

class SharedLibraryDispatcher : public ILibraryDispatcher {
  public:
    static const std::string pluginDataId;

    SharedLibraryDispatcher(QString const& fileName);

    void *getPluginData() override;
    functionPointer resolve(std::string const& functionName) override;
    std::string fileName() override;

    static bool isValidPlugin(QDir const& pluginDir);
    static std::unique_ptr<ILibraryDispatcher> loadSinglePlugin(QDir const& pluginDir);

  private:
    QLibrary library;
};

} // namespace Codethink::lvtplg

#endif // DIAGRAM_SERVER_CT_LVTPLG_SHAREDLIBRARYDISPATCHER_H
