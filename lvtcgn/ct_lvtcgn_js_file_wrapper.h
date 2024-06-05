#ifndef CT_LVTCGN_JS_FILE_WRAPPER_H
#define CT_LVTCGN_JS_FILE_WRAPPER_H

#include <QObject>

namespace Codethink::lvtcgn {
class FileIO : public QObject {
    Q_OBJECT
  public:
    Q_INVOKABLE FileIO();
    Q_INVOKABLE bool saveFile(const QString& filename, const QString& contents);
    Q_INVOKABLE QString openFile(const QString& filename);
    Q_INVOKABLE bool hasError();
    Q_INVOKABLE QString errorString();

  private:
    bool _hasError = false;
    QString _errorString;
};

} // namespace Codethink::lvtcgn
#endif
