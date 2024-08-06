/*
 / /* Copyright 2024 Codethink Ltd <codethink@codethink.co.uk>
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

#ifndef CT_LVTCLP_PARSE_ERROR_HANDLER_H
#define CT_LVTCLP_PARSE_ERROR_HANDLER_H

#include <QObject>
#include <QString>
#include <filesystem>
#include <lvtclp_export.h>
#include <map>
#include <thirdparty/result/result.hpp>

namespace Codethink::lvtclp {
class CppTool;
class LVTCLP_EXPORT ParseErrorHandler : public QObject {
    Q_OBJECT

    std::map<QString, QString> d_fileNameToContents;

  public:
    Q_SLOT void receivedMessage(const QString& errMessage, int threadId);
    bool hasErrors() const;
    void clear();
    void setTool(CppTool *tool);

    struct SaveOutputInputArgs {
        std::filesystem::path compileCommands;
        std::filesystem::path outputPath;
        QString ignorePattern;
    };
    cpp::result<void, QString> saveOutput(SaveOutputInputArgs args);
};
} // namespace Codethink::lvtclp

#endif
