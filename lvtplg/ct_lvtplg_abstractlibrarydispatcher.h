// ct_lvtplg_abstractlibrarydispatcher.h                             -*-C++-*-

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

#ifndef DIAGRAM_SERVER_CT_LVTPLG_ABSTRACTLIBRARYDISPATCHER_H
#define DIAGRAM_SERVER_CT_LVTPLG_ABSTRACTLIBRARYDISPATCHER_H

#include <KPluginMetaData>
#include <QFile>
#include <QJsonDocument>

#include <filesystem>
#include <iostream>
#include <memory>
#include <string>

typedef void (*functionPointer)();

namespace Codethink::lvtplg {

class AbstractLibraryDispatcher {
  public:
    struct ResolveContext {
        ResolveContext(functionPointer const& hook): hook(hook)
        {
        }

        virtual ~ResolveContext()
        {
        }

        functionPointer hook = nullptr;
    };

    virtual void reload() = 0;
    virtual ~AbstractLibraryDispatcher() = 0;
    virtual std::unique_ptr<ResolveContext> resolve(std::string const& functionName) = 0;
    virtual std::string fileName() = 0;

    virtual std::string pluginId()
    {
        return metadata().pluginId().toStdString();
    }

    virtual std::string metadataFilePath()
    {
        return (std::filesystem::path{this->fileName()}.parent_path() / "metadata.json").string();
    }

    virtual void setEnabled(bool enabled)
    {
        auto rawData = metadata().rawData();
        auto obj = rawData["KPlugin"].toObject();
        obj["EnabledByDefault"] = enabled;
        rawData["KPlugin"] = obj;
        auto file = QFile{QString::fromStdString(this->metadataFilePath())};
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            return;
        }
        file.write(QJsonDocument(rawData).toJson());
        file.close();
    }

    virtual bool isEnabled()
    {
        auto rawData = metadata().rawData();
        auto obj = rawData["KPlugin"].toObject();
        return obj["EnabledByDefault"].toBool();
    }

  protected:
    KPluginMetaData metadata()
    {
        return KPluginMetaData::fromJsonFile(QString::fromStdString(this->metadataFilePath()));
    }
};

inline AbstractLibraryDispatcher::~AbstractLibraryDispatcher() = default;

} // namespace Codethink::lvtplg

#endif // DIAGRAM_SERVER_CT_LVTPLG_ABSTRACTLIBRARYDISPATCHER_H
