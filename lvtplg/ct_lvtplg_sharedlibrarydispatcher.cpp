// ct_lvtplg_sharedlibrarydispatcher.cpp                             -*-C++-*-

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

#include <ct_lvtplg_sharedlibrarydispatcher.h>

namespace Codethink::lvtplg {

const std::string SharedLibraryDispatcher::pluginDataId = "private::soLibData";

SharedLibraryDispatcher::SharedLibraryDispatcher(QString const& fileName): library(fileName)
{
}

void *SharedLibraryDispatcher::getPluginData()
{
    return nullptr;
}

functionPointer SharedLibraryDispatcher::resolve(std::string const& functionName)
{
    return library.resolve(functionName.c_str());
}

std::string SharedLibraryDispatcher::fileName()
{
    return library.fileName().toStdString();
}

bool SharedLibraryDispatcher::isValidPlugin(QDir const& pluginDir)
{
    auto pluginName = pluginDir.dirName();
    // TODO: MacOS support
#if defined(Q_OS_WINDOWS)
    auto so_ext = QString{".dll"};
#else
    auto so_ext = QString{".so"};
#endif
    return (pluginDir.exists("README.md") && pluginDir.exists(pluginName + so_ext));
}

std::unique_ptr<ILibraryDispatcher> SharedLibraryDispatcher::loadSinglePlugin(QDir const& pluginDir)
{
    auto pluginName = pluginDir.dirName();
    auto pluginSharedObjectBasename = pluginDir.path() + "/" + pluginName;
    auto lib = std::make_unique<SharedLibraryDispatcher>(pluginSharedObjectBasename);
    if (!lib->library.load()) {
        return nullptr;
    }
    return lib;
}

} // namespace Codethink::lvtplg
