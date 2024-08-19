#include <ct_lvtshr_zip.h>

#include <QDebug>
#include <QDir>
#include <QFileInfo>

#include <KZip>
#include <filesystem>
#include <zip.h>

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
    // Conversion dance.
    QByteArray archiveNameArray = zipFileName.absoluteFilePath().toLocal8Bit();
    const char *archive_name = archiveNameArray.constData();

    int err = 0;
    struct zip *zip_archive = zip_open(archive_name, 0, &err);
    if (zip_archive == nullptr) {
        char buf[100];
        zip_error_to_str(buf, sizeof(buf), err, errno);
        return cpp::fail(ZipError{buf});
    }

    for (int i = 0; i < zip_get_num_entries(zip_archive, 0); i++) {
        struct zip_stat st;
        zip_stat_init(&st);

        int err = zip_stat_index(zip_archive, i, 0, &st);
        if (err < 0) {
            return cpp::fail(ZipError{"Error stating the file"});
        }

        QString fileName = st.name;
        // If it's a directory, create it and run the next loop.'
        if (fileName.endsWith('/')) {
            QString dirName = folder.absolutePath() + "/" + fileName;
            std::filesystem::create_directories(dirName.toStdString());
            continue;
        }

        // Open the zip data, and the file in disk.
        struct zip_file *zf = zip_fopen_index(zip_archive, i, 0);
        if (!zf) {
            return cpp::fail(ZipError{"Failed to read zip archive"});
        }

        QFile localFile(folder.absolutePath() + "/" + fileName);
        if (!localFile.open(QIODevice::WriteOnly)) {
            return cpp::fail(ZipError{localFile.errorString().toStdString()});
        }

        // Save the zip data on the local disk file.
        // Alloc memory for its uncompressed contents
        std::vector<char> contents(st.size);
        if (zip_fread(zf, contents.data(), st.size) == -1) {
            return cpp::fail(ZipError{"Error reading file contents from zip file"});
        }

        if (localFile.write(contents.data(), contents.size()) == -1) {
            return cpp::fail(ZipError{localFile.errorString().toStdString()});
        }

        err = zip_fclose(zf);
        if (err != 0) {
            return cpp::fail(ZipError{"Failed to close file buffer from zip file"});
        }
    }

    err = zip_close(zip_archive);
    if (err != 0) {
        return cpp::fail(ZipError{"Failed to close zip file"});
    }

    return {};
}
