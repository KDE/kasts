/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QFile>
#include <QObject>
#include <QString>
#include <QUrl>

#include <KFormat>

#include "error.h"

class StorageManager : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int storageMoveProgress MEMBER m_storageMoveProgress NOTIFY storageMoveProgressChanged)
    Q_PROPERTY(int storageMoveTotal MEMBER m_storageMoveTotal NOTIFY storageMoveTotalChanged)
    Q_PROPERTY(QString storagePath READ storagePath NOTIFY storagePathChanged)
    Q_PROPERTY(qint64 enclosureDirSize READ enclosureDirSize NOTIFY enclosureDirSizeChanged)
    Q_PROPERTY(qint64 imageDirSize READ imageDirSize NOTIFY imageDirSizeChanged)
    Q_PROPERTY(QString formattedEnclosureDirSize READ formattedEnclosureDirSize NOTIFY enclosureDirSizeChanged)
    Q_PROPERTY(QString formattedImageDirSize READ formattedImageDirSize NOTIFY imageDirSizeChanged)

public:
    static StorageManager &instance()
    {
        static StorageManager _instance;
        return _instance;
    }

    QString storagePath() const;
    Q_INVOKABLE void setStoragePath(QUrl url);

    QString imageDirPath() const;
    QString imagePath(const QString &url) const;

    QString enclosureDirPath() const;
    QString enclosurePath(const QString &url) const;

    qint64 enclosureDirSize() const;
    qint64 imageDirSize() const;
    QString formattedEnclosureDirSize() const;
    QString formattedImageDirSize() const;

    void removeImage(const QString &url);
    Q_INVOKABLE void clearImageCache();

Q_SIGNALS:
    void error(Error::Type type, const QString &url, const QString &id, const int errorId, const QString &errorString, const QString &title);

    void storagePathChanged(QString path);
    void storageMoveStarted();
    void storageMoveFinished();
    void storageMoveProgressChanged(int progress);
    void storageMoveTotalChanged(int nrOfFeeds);
    void cancelStorageMove();

    void enclosureDirSizeChanged();
    void imageDirSizeChanged();

private:
    StorageManager();

    qint64 dirSize(const QString &path) const;

    int m_storageMoveProgress;
    int m_storageMoveTotal;
    KFormat m_kformat;
};
