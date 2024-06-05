#include <ct_lvtcgn_js_file_wrapper.h>

#include <QFile>
#include <QString>
#include <QTextStream>

namespace Codethink::lvtcgn {
FileIO::FileIO()
{
}

bool FileIO::saveFile(const QString& filename, const QString& contents)
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        _hasError = true;
        _errorString = file.errorString();
        return false;
    }

    QTextStream stream(&file);
    stream << contents;
    file.close();
    return true;
}

QString FileIO::openFile(const QString& filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        _hasError = true;
        _errorString = file.errorString();
        return QString();
    }

    QTextStream stream(&file);
    return stream.readAll();
}

bool FileIO::hasError()
{
    return _hasError;
}

QString FileIO::errorString()
{
    return _errorString;
}
} // namespace Codethink::lvtcgn
