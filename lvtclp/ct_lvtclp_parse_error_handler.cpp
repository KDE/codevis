/*
 / /* Copyright 2023 Codethink Ltd <codethink@codethink.co.uk>
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

#include <ct_lvtclp_cpp_tool.h>
#include <ct_lvtclp_parse_error_handler.h>
#include <ct_lvtshr_zip.h>

#include <QDir>
#include <QFileInfo>
#include <QList>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <thirdparty/result/result.hpp>

using namespace Codethink::lvtclp;

namespace {
struct CompressFileError {
    QString errorString;
};

cpp::result<QString, CompressFileError> createSysinfoFileAt(const QString& lPath, const QString& ignorePattern)
{
    QFile systemInformation(lPath + "/system_information.txt");
    if (!systemInformation.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return cpp::fail(CompressFileError{
            QStringLiteral("Error opening the sys info file.  %1").arg(systemInformation.errorString())});
    }

    QString systemInfoData =

        // this string should not be called with "tr", we do not want to
        // translate this to other languages, I have no intention on reading
        // a log file in russian.
        "CPU: " + QSysInfo::currentCpuArchitecture() + "\n" + "Operating System: " + QSysInfo::productType() + "\n"
        + "Version " + QSysInfo::productVersion() + "\n" + "Ignored File Information: " + ignorePattern + "\n"
        + "CodeVis version:" + QString(__DATE__);

    systemInformation.write(systemInfoData.toLocal8Bit());
    systemInformation.close();

    return systemInformation.fileName();
}
} // namespace

cpp::result<void, QString> ParseErrorHandler::saveOutput(ParseErrorHandler::SaveOutputInputArgs args)
{
    QTemporaryDir dir;

    const std::filesystem::path compile_commands_orig = args.compileCommands;
    const std::filesystem::path compile_commands_dest = (dir.path() + "compile_commands.json").toStdString();

    // We don't fail hard on this function, we try to save as much details as possible even if the
    // function failed in some parts.'
    QString errorString;
    try {
        std::filesystem::copy_file(compile_commands_orig, compile_commands_dest);
    } catch (std::filesystem::filesystem_error& e) {
        errorString = QStringLiteral("Could not copy compile_commands.json to the save folder: %1").arg(e.what());
    }

    auto res = createSysinfoFileAt(dir.path(), args.ignorePattern);
    if (!res) {
        if (!errorString.isEmpty()) {
            errorString += "\n";
        }
        errorString += res.error().errorString;
    }

    const QString sysInfoFile = [&res]() -> QString {
        return res ? res.value() : QString();
    }();

    for (const auto& [fileName, fileContents] : d_fileNameToContents) {
        QString filePath = dir.path() + "/" + fileName + ".txt";
        QFile f(filePath);
        if (!f.open(QIODevice::WriteOnly)) {
            if (!errorString.isEmpty()) {
                errorString += "\n";
            }
            errorString += f.errorString();
            continue;
        }

        QTextStream stream(&f);
        stream << fileContents;
        f.close();
    }

    const QFileInfo outputFile = QFileInfo{QString::fromStdString(args.outputPath.generic_string())};

    // QTemporaryDir can't be casted to QDir. :(
    QDir ourFolder(dir.path());
    auto res2 = lvtshr::Zip::compressFolder(ourFolder, outputFile);
    if (!res2) {
        if (!errorString.isEmpty()) {
            errorString += "\n";
        }
        errorString += QString::fromStdString(res2.error().what);
    }

    if (!errorString.isEmpty()) {
        return cpp::fail(errorString);
    }

    std::cout << "Error File saved at:" << outputFile.absoluteFilePath().toStdString() << "\n";
    return {};
}

void ParseErrorHandler::clear()
{
    d_fileNameToContents.clear();
}

bool ParseErrorHandler::hasErrors() const
{
    return !d_fileNameToContents.empty();
}

void ParseErrorHandler::receivedMessage(const QString& message, int threadId)
{
    std::cout << "GOT FROM THREAD" << message.toStdString() << std::endl;
    d_fileNameToContents[QString::number(threadId)] += message;
}

void ParseErrorHandler::setTool(CppTool *tool)
{
    std::cout << "Connecting tool to handler" << tool << std::endl;
    connect(tool, &lvtclp::CppTool::messageFromThread, this, &ParseErrorHandler::receivedMessage, Qt::QueuedConnection);
}

#include "moc_ct_lvtclp_parse_error_handler.cpp"
