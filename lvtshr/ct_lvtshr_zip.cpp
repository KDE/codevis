#include <ct_lvtshr_zip.h>

#include <QDebug>
#include <QDir>
#include <QFileInfo>

#include <filesystem>
#include <zip.h>

using namespace Codethink::lvtshr;

namespace {
cpp::result<void, ZipError> walk_directory(QDir& startdir, QDir& inputdir, zip_t *zipper)
{
    const auto allFilesAndFolders =
        inputdir.entryList(QDir::Filter::Files | QDir::Filter::Dirs | QDir::Filter::NoDotAndDotDot);
    for (const QString& file : allFilesAndFolders) {
        const QString fullName = inputdir.absolutePath() + "/" + file;
        QFileInfo infoFullName(fullName);

        int rightSize = fullName.length() - startdir.absolutePath().length();
        QByteArray partialName = fullName.right(rightSize).toLocal8Bit();

        qDebug() << "--------------------------";
        qDebug() << "StartDir" << startdir;
        qDebug() << "CurrentDir" << inputdir;
        qDebug() << fullName;
        qDebug() << fullName.right(rightSize);
        qDebug() << inputdir.absolutePath().length();
        qDebug() << startdir.absolutePath().length();
        qDebug() << "Calculating partial name:" << partialName;

        // Add Directories.
        if (infoFullName.isDir()) {
            if (zip_dir_add(zipper, partialName.constData(), ZIP_FL_ENC_UTF_8) < 0) {
                return cpp::fail(std::string(zip_strerror(zipper)));
            }

            QDir nextDir(fullName);
            auto res = walk_directory(startdir, nextDir, zipper);
            if (!res) {
                return res;
                ;
            }

            continue;
        }

        QByteArray bFullName = fullName.toLocal8Bit();
        zip_source_t *source = zip_source_file(zipper, bFullName.constData(), 0, 0);
        if (source == nullptr) {
            return cpp::fail(std::string{"Failed to add file to zip: " + std::string(zip_strerror(zipper))});
        }

        if (zip_file_add(zipper, partialName.constData(), source, ZIP_FL_ENC_UTF_8) < 0) {
            zip_source_free(source);
            return cpp::fail(std::string{"Failed to add file to zip: "} + std::string(zip_strerror(zipper)));
        }
    }
    return {};
}
} // namespace

cpp::result<void, ZipError> Zip::compressFolder(QDir& folder, const QFileInfo& zipFileName)
{
    if (!QDir{}.exists(zipFileName.absolutePath()) && !QDir{}.mkdir(zipFileName.absolutePath())) {
        return cpp::fail("[compressDir] Could not prepare path to save.");
    }

    QByteArray bZipFileName = zipFileName.absoluteFilePath().toLocal8Bit();

    int errorp;
    zip_t *zipper = zip_open(bZipFileName.constData(), ZIP_CREATE | ZIP_EXCL, &errorp);
    if (zipper == nullptr) {
        zip_error_t ziperror;
        zip_error_init_with_code(&ziperror, errorp);
        return cpp::fail(ZipError{std::string("Failed to open output file") + bZipFileName.constData() + ": "
                                  + zip_error_strerror(&ziperror)});
    }

    auto res = walk_directory(folder, folder, zipper);
    if (!res) {
        zip_close(zipper);
        return res;
    }

    zip_close(zipper);
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
