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

#include "datatypes.h"
#include "models/errorlogmodel.h"

class Entry;

class Enclosure : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

    Q_PROPERTY(qint64 enclosureuid READ enclosureuid CONSTANT)
    Q_PROPERTY(qint64 size READ size NOTIFY sizeChanged)
    Q_PROPERTY(QString type MEMBER m_type NOTIFY typeChanged)
    Q_PROPERTY(QString url READ url NOTIFY urlChanged)
    Q_PROPERTY(DataTypes::EnclosureStatus status READ status NOTIFY statusChanged)
    Q_PROPERTY(double downloadProgress MEMBER m_downloadProgress NOTIFY downloadProgressChanged)
    Q_PROPERTY(qint64 downloadSize READ downloadSize NOTIFY downloadProgressChanged)
    Q_PROPERTY(QString path READ path NOTIFY pathChanged)
    Q_PROPERTY(qint64 playPosition READ playPosition WRITE setPlayPosition NOTIFY playPositionChanged)
    Q_PROPERTY(qint64 duration READ duration NOTIFY durationChanged)

public:
    Enclosure(Entry *entry);
    ~Enclosure();

    Q_INVOKABLE void download();
    Q_INVOKABLE void deleteFile();

    qint64 enclosureuid() const;
    QString path() const;
    QString url() const;
    DataTypes::EnclosureStatus status() const;
    qint64 playPosition() const;
    qint64 duration() const;
    qint64 size() const;
    qint64 downloadSize() const;

    void setPlayPosition(const qint64 &position);
    void checkSizeOnDisk();

Q_SIGNALS:
    void typeChanged(const QString &type);
    void urlChanged(const QString &url);
    void pathChanged(const QString &path);
    void statusChanged(Entry *entry, DataTypes::EnclosureStatus status);
    void downloadProgressChanged();
    void cancelDownload();
    void playPositionChanged();
    void durationChanged();
    void sizeChanged();
    void downloadError(const ErrorLogModel::Type type, const QString &message);

private:
    void updateFromDb();
    void processDownloadedFile();

    qint64 m_enclosureuid;
    qint64 m_entryuid;
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
    DataTypes::EnclosureStatus m_status;
};
