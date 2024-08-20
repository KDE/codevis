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

#include <QDir>
#include <QFileInfo>
#include <QList>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <thirdparty/result/result.hpp>

#include <KZip>

using namespace Codethink::lvtclp;

namespace {
struct CompressFileError {
    QString errorString;
};

cpp::result<void, CompressFileError> compressFiles(QFileInfo const& saveTo, QList<QFileInfo> const& files)
{
    if (!QDir{}.exists(saveTo.absolutePath()) && !QDir{}.mkdir(saveTo.absolutePath())) {
        return cpp::fail(CompressFileError{QStringLiteral("compressFiles] Could not prepare path to save")});
    }

    auto zipFile = KZip(saveTo.absoluteFilePath());
    if (!zipFile.open(QIODevice::WriteOnly)) {
        return cpp::fail(
            CompressFileError{QStringLiteral("Could not open file to compress: %1").arg(zipFile.errorString())});
    }

    QString errMessage;
    for (auto const& fileToCompress : qAsConst(files)) {
        qDebug() << "Adding " << fileToCompress << "to the zip bundle";
        auto r = zipFile.addLocalFile(fileToCompress.absoluteFilePath(), "");
        if (!r) {
            if (!errMessage.isEmpty()) {
                errMessage += "\n";
            }
            errMessage += QStringLiteral("Could not add files to zip bundle: %1").arg(zipFile.errorString());
        }
    }

    if (!errMessage.isEmpty()) {
        return cpp::fail(errMessage);
    }

    return {};
}

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
    bool hasCompileCommands = true;
    QString errorString;
    try {
        std::filesystem::copy_file(compile_commands_orig, compile_commands_dest);
    } catch (std::filesystem::filesystem_error& e) {
        hasCompileCommands = false;
        errorString = QStringLiteral("Could not copy compile_commands.json to the save folder: %1").arg(e.what());
    }

    bool hasSysinfoFile = true;
    auto res = createSysinfoFileAt(dir.path(), args.ignorePattern);
    if (!res) {
        hasSysinfoFile = false;
        if (!errorString.isEmpty()) {
            errorString += "\n";
        }
        errorString += res.error().errorString;
    }

    const QString sysInfoFile = [&res]() -> QString {
        return res ? res.value() : QString();
    }();

    QList<QFileInfo> textFiles;
    if (hasSysinfoFile) {
        textFiles.append(QFileInfo{res.value()});
    }
    if (hasCompileCommands) {
        const QString compileCommandsFile = QString::fromStdString(compile_commands_dest.generic_string());
        textFiles.append(QFileInfo{compileCommandsFile});
    }

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
        textFiles.append(QFileInfo{filePath});
    }

    const QFileInfo outputFile = QFileInfo{QString::fromStdString(args.outputPath.generic_string())};
    auto res2 = compressFiles(outputFile, textFiles);
    if (!res2) {
        if (!errorString.isEmpty()) {
            errorString += "\n";
        }
        errorString += res2.error().errorString;
    }

    for (const auto& textFile : qAsConst(textFiles)) {
        std::filesystem::remove(textFile.absoluteFilePath().toStdString());
    }

    if (!errorString.isEmpty()) {
        return cpp::fail(errorString);
    }

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
    d_fileNameToContents[QString::number(threadId)] += message;
}

void ParseErrorHandler::setTool(CppTool *tool)
{
    connect(tool, &lvtclp::CppTool::messageFromThread, this, &ParseErrorHandler::receivedMessage, Qt::QueuedConnection);
}

#include "moc_ct_lvtclp_parse_error_handler.cpp"
