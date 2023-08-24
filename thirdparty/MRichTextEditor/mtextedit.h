#ifndef MTEXTEDIT_H_
#define MTEXTEDIT_H_

#include <QTextEdit>
#include <QMimeData>
#include <QImage>

#include <mrichtextedit_export.h>

class MRICHTEXTEDIT_EXPORT MTextEdit : public QTextEdit {
    Q_OBJECT
  public:
    MTextEdit(QWidget *parent);

    void        dropImage(const QImage& image, const QString& format);

  protected:
    bool        canInsertFromMimeData(const QMimeData *source) const override;
    void        insertFromMimeData(const QMimeData *source) override;
    QMimeData  *createMimeDataFromSelection() const override;

};

#endif
