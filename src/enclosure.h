/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef ENCLOSURE_H
#define ENCLOSURE_H

#include <QDebug>
#include <QObject>
#include <QString>

class Entry;

class Enclosure : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int duration MEMBER m_duration CONSTANT)
    Q_PROPERTY(int size MEMBER m_size CONSTANT)
    Q_PROPERTY(QString title MEMBER m_title CONSTANT)
    Q_PROPERTY(QString type MEMBER m_type CONSTANT)
    Q_PROPERTY(QString url MEMBER m_url CONSTANT)
    Q_PROPERTY(Status status MEMBER m_status NOTIFY statusChanged)
    Q_PROPERTY(double downloadProgress MEMBER m_downloadProgress NOTIFY downloadProgressChanged)
    Q_PROPERTY(QString path READ path CONSTANT)
    Q_PROPERTY(QString playposition MEMBER m_playposition CONSTANT)

public:
    Enclosure(Entry *entry);

    enum Status {
        Downloadable,
        Downloading,
        Downloaded,
        Error,
    };
    Q_ENUM(Status)

    Q_INVOKABLE void download();
    Q_INVOKABLE void deleteFile();

    QString path() const;

Q_SIGNALS:
    void statusChanged();
    void downloadProgressChanged();
    void cancelDownload();

private:

    Entry *m_entry;
    int m_duration;
    int m_size;
    QString m_title;
    QString m_type;
    QString m_url;
    int m_playposition;
    double m_downloadProgress = 0;
    Status m_status;
};

#endif // ENCLOSURE_H
