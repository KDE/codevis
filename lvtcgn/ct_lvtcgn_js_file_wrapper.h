#ifndef CT_LVTCGN_JS_FILE_WRAPPER_H
#define CT_LVTCGN_JS_FILE_WRAPPER_H

#include <KTextTemplate/Engine>
#include <QObject>
#include <ktexttemplate/template.h>

namespace Codethink::lvtcgn {
class FileIO : public QObject {
    Q_OBJECT
  public:
    Q_INVOKABLE FileIO();
    Q_INVOKABLE bool saveFile(const QString& filename, const QString& contents);
    Q_INVOKABLE bool createPath(const QString& path);
    Q_INVOKABLE bool hasError();
    Q_INVOKABLE QString render(const QString& templateFile, const QVariantMap& data);
    Q_INVOKABLE QString openFile(const QString& filename);
    Q_INVOKABLE QString errorString();

  private:
    bool _hasError = false;
    QString _errorString;
    QHash<QString, KTextTemplate::Template> _loadedTemplates;
    KTextTemplate::Engine _templateEngine;
};

} // namespace Codethink::lvtcgn
#endif
