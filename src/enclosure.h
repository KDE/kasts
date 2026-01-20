/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <tobias.fella@kde.org>
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QDebug>
#include <QObject>
#include <QQmlEngine>
#include <QString>

#include <KFormat>

#include "error.h"

class Entry;

class Enclosure : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

    Q_PROPERTY(qint64 size READ size WRITE setSize NOTIFY sizeChanged)
    Q_PROPERTY(QString formattedSize READ formattedSize NOTIFY sizeChanged)
    Q_PROPERTY(qint64 sizeOnDisk READ sizeOnDisk NOTIFY sizeOnDiskChanged)
    Q_PROPERTY(QString type MEMBER m_type NOTIFY typeChanged)
    Q_PROPERTY(QString url READ url NOTIFY urlChanged)
    Q_PROPERTY(Status status READ status WRITE setStatus NOTIFY statusChanged)
    Q_PROPERTY(double downloadProgress MEMBER m_downloadProgress NOTIFY downloadProgressChanged)
    Q_PROPERTY(QString formattedDownloadSize READ formattedDownloadSize NOTIFY downloadProgressChanged)
    Q_PROPERTY(QString path READ path NOTIFY pathChanged)
    Q_PROPERTY(QString cachedEmbeddedImage READ cachedEmbeddedImage CONSTANT)
    Q_PROPERTY(qint64 playPosition READ playPosition WRITE setPlayPosition NOTIFY playPositionChanged)
    Q_PROPERTY(QString formattedLeftDuration READ formattedLeftDuration NOTIFY leftDurationChanged)
    Q_PROPERTY(QString formattedPlayPosition READ formattedPlayPosition NOTIFY playPositionChanged)
    Q_PROPERTY(qint64 duration READ duration WRITE setDuration NOTIFY durationChanged)
    Q_PROPERTY(QString formattedDuration READ formattedDuration NOTIFY durationChanged)

public:
    Enclosure(Entry *entry);

    enum Status {
        Error = -1,
        Downloadable = 0,
        Downloading,
        PartiallyDownloaded,
        Downloaded,
    };
    Q_ENUM(Status)

    static int statusToDb(Status status); // needed to translate Enclosure::Status values to int for sqlite
    static Status dbToStatus(int value); // needed to translate from int to Enclosure::Status values for sqlite

    Q_INVOKABLE void download();
    Q_INVOKABLE void deleteFile();

    QString path() const;
    QString url() const;
    Status status() const;
    QString cachedEmbeddedImage() const;
    qint64 playPosition() const;
    qint64 duration() const;
    qint64 size() const;
    qint64 sizeOnDisk() const;
    QString formattedSize() const;
    QString formattedDuration() const;
    QString formattedLeftDuration() const;
    QString formattedPlayPosition() const;
    QString formattedDownloadSize() const;

    void setStatus(Status status);
    void setPlayPosition(const qint64 &position);
    void setDuration(const qint64 &duration);
    void setSize(const qint64 &size);
    void checkSizeOnDisk();

Q_SIGNALS:
    void typeChanged(const QString &type);
    void urlChanged(const QString &url);
    void pathChanged(const QString &path);
    void statusChanged(Entry *entry, Status status);
    void downloadProgressChanged();
    void cancelDownload();
    void playPositionChanged();
    void leftDurationChanged();
    void durationChanged();
    void sizeChanged();
    void sizeOnDiskChanged();
    void downloadError(const Error::Type type, const QString &url, const QString &id, const int errorId, const QString &errorString, const QString &title);

private:
    void updateFromDb();
    void processDownloadedFile();

    Entry *m_entry;
    qint64 m_duration;
    qint64 m_size = 0;
    qint64 m_sizeOnDisk = 0;
    QString m_type;
    QString m_url;
    qint64 m_playposition;
    qint64 m_playposition_dbsave;
    double m_downloadProgress = 0;
    qint64 m_downloadSize = 0;
    Status m_status;
    KFormat m_kformat;
};
