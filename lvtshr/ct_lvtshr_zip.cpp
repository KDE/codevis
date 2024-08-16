#include <ct_lvtshr_zip.h>

#include <QDebug>
#include <QDir>
#include <QFileInfo>

#include <KZip>

using namespace Codethink::lvtshr;

cpp::result<void, ZipError> Zip::compressFolder(QDir& folder, const QFileInfo& zipFileName)
{
    if (!QDir{}.exists(zipFileName.absolutePath()) && !QDir{}.mkdir(zipFileName.absolutePath())) {
        return cpp::fail("[compressDir] Could not prepare path to save.");
    }

    auto zipFile = KZip(zipFileName.absoluteFilePath());
    if (!zipFile.open(QIODevice::WriteOnly)) {
        return cpp::fail("[compressDir] Could not open file to compress: "
                         + zipFileName.absoluteFilePath().toStdString() + zipFile.errorString().toStdString());
    }

    auto r = zipFile.addLocalDirectory(folder.path(), "");
    if (!r) {
        return cpp::fail("[compressDir] Could not add folder to project: " + folder.absolutePath().toStdString()
                         + zipFile.errorString().toStdString());
    }

    return {};
}

cpp::result<void, ZipError> Zip::uncompressToFolder(QDir& folder, const QFileInfo& zipFileName)
{
    auto zipFile = KZip(zipFileName.absoluteFilePath());
    if (!zipFile.open(QIODevice::ReadOnly)) {
        return cpp::fail(std::string("Could not open file to read contents:")
                         + zipFileName.absoluteFilePath().toStdString() + "  " + zipFile.errorString().toStdString());
    }

    const KArchiveDirectory *dir = zipFile.directory();
    bool copied = dir->copyTo(folder.absolutePath());
    if (!copied) {
        return cpp::fail(std::string("Error copying files to directory") + zipFileName.absoluteFilePath().toStdString()
                         + "  " + zipFile.errorString().toStdString());
    }

    return {};
}

bool compressDir(QFileInfo const& saveTo, QDir const& dirToCompress)
{
}
