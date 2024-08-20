#include <ct_lvtcgn_js_file_wrapper.h>

#include <KTextTemplate/Engine>
#include <KTextTemplate/Template>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QTextStream>
#include <ktexttemplate/context.h>
#include <ktexttemplate/engine.h>
#include <ktexttemplate/template.h>

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

    qDebug() << "Successfully saved" << filename;
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

bool FileIO::createPath(const QString& path)
{
    QDir dir(path);
    if (dir.exists()) {
        return true;
    }

    return dir.mkpath(path);
}

bool FileIO::hasError()
{
    return _hasError;
}

QString FileIO::errorString()
{
    return _errorString;
}

QString FileIO::render(const QString& templateFile, const QVariantMap& data)
{
    qDebug() << "-=-=-=-=-=-" << templateFile << "-=-=-=-=-=-=";
    _hasError = false;
    _errorString.clear();

    QFileInfo fInfo(templateFile);
    if (!fInfo.exists()) {
        _hasError = true;
        _errorString = "No such file " + templateFile;
        return {};
    }

    // Hack to load the templates only once during the execution of the script.
    if (!_loadedTemplates.contains(templateFile)) {
        QFile file(templateFile);
        file.open(QIODevice::ReadOnly);
        const QString contents = file.readAll();
        KTextTemplate::Template tmplate = _templateEngine.newTemplate(contents, templateFile);
        if (tmplate->error()) {
            _hasError = true;
            _errorString = tmplate->errorString();
            return {};
        }

        _loadedTemplates[templateFile] = tmplate;
    }

    // Convert the Variant map to HashedMap.
    QHash<QString, QVariant> hashedData;
    for (const auto [key, val] : data.asKeyValueRange()) {
        hashedData[key] = val;
    }

    KTextTemplate::Context context(hashedData);
    const auto ret = _loadedTemplates[templateFile]->render(&context);
    if (_loadedTemplates[templateFile]->error()) {
        _hasError = true;
        _errorString = _loadedTemplates[templateFile]->errorString();
        return {};
    }
    return ret;
}

} // namespace Codethink::lvtcgn

#include "moc_ct_lvtcgn_js_file_wrapper.cpp"
