/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QFile>
#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QUrl>

#include "models/errorlogmodel.h"

class StorageManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(int storageMoveProgress MEMBER m_storageMoveProgress NOTIFY storageMoveProgressChanged)
    Q_PROPERTY(int storageMoveTotal MEMBER m_storageMoveTotal NOTIFY storageMoveTotalChanged)
    Q_PROPERTY(QString storagePath READ storagePath NOTIFY storagePathChanged)
    Q_PROPERTY(qint64 enclosureDirSize READ enclosureDirSize NOTIFY enclosureDirSizeChanged)
    Q_PROPERTY(qint64 imageDirSize READ imageDirSize NOTIFY imageDirSizeChanged)

public:
    static StorageManager &instance()
    {
        static StorageManager _instance;
        return _instance;
    }
    static StorageManager *create(QQmlEngine *engine, QJSEngine *)
    {
        engine->setObjectOwnership(&instance(), QQmlEngine::CppOwnership);
        return &instance();
    }

    static const int maxFilenameLength = 200;

    static QString storagePath();
    Q_INVOKABLE void setStoragePath(QUrl url);

    static QString imageDirPath();
    static QString imagePath(const QString &url);

    static QString enclosureDirPath();
    static QString enclosureDirPath(const QString &feedDirName);
    static QString enclosurePath(const QString &name, const QString &url, const QString &feedDirName);

    static qint64 enclosureDirSize();
    static qint64 imageDirSize();

    void removeImage(const QString &url);
    Q_INVOKABLE void clearImageCache();

    static QString passwordFilePath(const QString &username);
    static QString sanitizedFilePath(const QString &path);

Q_SIGNALS:
    void error(ErrorLogModel::Type type, const QString &message);

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

    static qint64 dirSize(const QString &path);

    int m_storageMoveProgress;
    int m_storageMoveTotal;
};
