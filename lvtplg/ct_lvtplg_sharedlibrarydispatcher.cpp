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

#include <QDebug>
#include <ct_lvtplg_sharedlibrarydispatcher.h>

namespace Codethink::lvtplg {

const std::string SharedLibraryDispatcher::pluginDataId = "private::soLibData";

SharedLibraryDispatcher::SharedLibraryDispatcher(QString const& fileName): library(fileName)
{
}

std::unique_ptr<AbstractLibraryDispatcher::ResolveContext>
SharedLibraryDispatcher::resolve(std::string const& functionName)
{
    return std::make_unique<AbstractLibraryDispatcher::ResolveContext>(library.resolve(functionName.c_str()));
}

std::string SharedLibraryDispatcher::fileName()
{
    return library.fileName().toStdString();
}

// TODO: Implement Shared Library Reload
void SharedLibraryDispatcher::reload()
{
    qWarning() << "Shared Library Reload: Not implemented yet";
}

// TODO: Implement Shared Library Unload
void SharedLibraryDispatcher::unload()
{
    qWarning() << "Shared Library Reload: Not implemented yet";
}

bool SharedLibraryDispatcher::isValidPlugin(QDir const& pluginDir)
{
    const auto pluginName = pluginDir.dirName();
    // TODO: MacOS support
#if defined(Q_OS_WINDOWS)
    const auto so_ext = QStringLiteral(".dll");
#else
    const auto so_ext = QStringLiteral(".so");
#endif
    const bool metadataExists = pluginDir.exists(QStringLiteral("metadata.json"));
    const bool readmeExists = pluginDir.exists(QStringLiteral("README.md"));
    const bool pluginExists = pluginDir.exists(pluginName + so_ext);
    return metadataExists && readmeExists && pluginExists;
}

std::unique_ptr<AbstractLibraryDispatcher> SharedLibraryDispatcher::loadSinglePlugin(QDir const& pluginDir)
{
    auto pluginName = pluginDir.dirName();
    auto pluginSharedObjectBasename = pluginDir.path() + QDir::separator() + pluginName;
    auto lib = std::make_unique<SharedLibraryDispatcher>(pluginSharedObjectBasename);
    if (!lib->library.load()) {
        return nullptr;
    }
    return lib;
}

} // namespace Codethink::lvtplg
