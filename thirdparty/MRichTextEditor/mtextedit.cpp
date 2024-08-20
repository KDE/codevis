#include "mtextedit.h"

#include <QTextDocument>
#include <QTextCursor>
#include <QImage>
#include <QByteArray>
#include <QBuffer>

#include <stdlib.h>


MTextEdit::MTextEdit(QWidget *parent) : QTextEdit(parent) {
}


bool MTextEdit::canInsertFromMimeData(const QMimeData *source) const {
    return source->hasImage() || QTextEdit::canInsertFromMimeData(source);
}


void MTextEdit::insertFromMimeData(const QMimeData *source) {
    if (source->hasImage()) {
        QStringList formats = source->formats();
        QString format;
        for (int i=0; i<formats.size(); i++) {
            if (formats[i] == QStringLiteral("image/bmp"))  { format = QStringLiteral("BMP");  break; }
            if (formats[i] == QStringLiteral("image/jpeg")) { format = QStringLiteral("JPG");  break; }
            if (formats[i] == QStringLiteral("image/jpg"))  { format = QStringLiteral("JPG");  break; }
            if (formats[i] == QStringLiteral("image/gif"))  { format = QStringLiteral("GIF");  break; }
            if (formats[i] == QStringLiteral("image/png"))  { format = QStringLiteral("PNG");  break; }
            if (formats[i] == QStringLiteral("image/pbm"))  { format = QStringLiteral("PBM");  break; }
            if (formats[i] == QStringLiteral("image/pgm"))  { format = QStringLiteral("PGM");  break; }
            if (formats[i] == QStringLiteral("image/ppm"))  { format = QStringLiteral("PPM");  break; }
            if (formats[i] == QStringLiteral("image/tiff")) { format = QStringLiteral("TIFF"); break; }
            if (formats[i] == QStringLiteral("image/xbm"))  { format = QStringLiteral("XBM");  break; }
            if (formats[i] == QStringLiteral("image/xpm"))  { format = QStringLiteral("XPM");  break; }
            }
        if (!format.isEmpty()) {
            dropImage(qvariant_cast<QImage>(source->imageData()), QStringLiteral("JPG"));
            return;
            }
        }
    QTextEdit::insertFromMimeData(source);
}


QMimeData *MTextEdit::createMimeDataFromSelection() const {
    return QTextEdit::createMimeDataFromSelection();
}


void MTextEdit::dropImage(const QImage& image, const QString& format) {
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, format.toLocal8Bit().data());
    buffer.close();
    QByteArray base64 = bytes.toBase64();
    QByteArray base64l;
    for (int i=0; i<base64.size(); i++) {
        base64l.append(base64[i]);
        if (i%80 == 0) {
            base64l.append("\n");
            }
        }

    QTextCursor cursor = textCursor();
    QTextImageFormat imageFormat;
    imageFormat.setWidth  ( image.width() );
    imageFormat.setHeight ( image.height() );
    imageFormat.setName   ( QStringLiteral("data:image/%1;base64,%2")
                                .arg(QStringLiteral("%1.%2").arg(rand()).arg(format))
                            .arg(QString::fromLocal8Bit(base64l))
                                );
    cursor.insertImage    ( imageFormat );
}


#include "moc_mtextedit.cpp"
