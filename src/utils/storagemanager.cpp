/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "storagemanager.h"
#include "storagemanagerlogging.h"

#include <KLocalizedString>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStandardPaths>

#include "enclosure.h"
#include "models/errorlogmodel.h"
#include "settingsmanager.h"
#include "storagemovejob.h"

StorageManager::StorageManager()
{
    connect(this, &StorageManager::error, &ErrorLogModel::instance(), &ErrorLogModel::monitorErrorMessages);
}

QString StorageManager::storagePath() const
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

    if (!SettingsManager::self()->storagePath().isEmpty()) {
        path = SettingsManager::self()->storagePath().toLocalFile();
    }

    // Create path if it doesn't exist yet
    QFileInfo().absoluteDir().mkpath(path);

    qCDebug(kastsStorageManager) << "Current storage path is" << path;

    return path;
}

void StorageManager::setStoragePath(QUrl url)
{
    qCDebug(kastsStorageManager) << "New storage path url:" << url;
    QUrl oldUrl = SettingsManager::self()->storagePath();
    QString oldPath = storagePath();
    QString newPath = oldPath;

    if (url.isEmpty()) {
        qCDebug(kastsStorageManager) << "(Re)set storage path to default location";
        SettingsManager::self()->setStoragePath(url);
        newPath = storagePath(); // retrieve default storage path, since url is empty
    } else if (url.isLocalFile()) {
        SettingsManager::self()->setStoragePath(url);
        newPath = url.toLocalFile();
    } else {
        qCDebug(kastsStorageManager) << "Cannot set storage path; path is not on local filesystem:" << url;
        return;
    }

    qCDebug(kastsStorageManager) << "Current storage path in settings:" << SettingsManager::self()->storagePath();
    qCDebug(kastsStorageManager) << "New storage path will be:" << newPath;

    if (oldPath != newPath) {
        // make list of dirs to be moved (images/ and enclosures/*/)
        QStringList list = {QStringLiteral("images")};
        for (const QString &subdir : QDir(oldPath + QStringLiteral("/enclosures/")).entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            list << QStringLiteral("enclosures/") + subdir;
        }
        list << QStringLiteral("enclosures");

        StorageMoveJob *moveJob = new StorageMoveJob(oldPath, newPath, list);
        connect(moveJob, &KJob::processedAmountChanged, this, [this, moveJob]() {
            m_storageMoveProgress = moveJob->processedAmount(KJob::Files);
            Q_EMIT storageMoveProgressChanged(m_storageMoveProgress);
        });
        connect(moveJob, &KJob::totalAmountChanged, this, [this, moveJob]() {
            m_storageMoveTotal = moveJob->totalAmount(KJob::Files);
            Q_EMIT storageMoveTotalChanged(m_storageMoveTotal);
        });
        connect(moveJob, &KJob::result, this, [this, moveJob, oldUrl, oldPath, newPath]() {
            if (moveJob->error() != 0) {
                // Go back to previous old path
                SettingsManager::self()->setStoragePath(oldUrl);
                QString title =
                    i18n("Old location:") + QStringLiteral(" ") + oldPath + QStringLiteral("; ") + i18n("New location:") + QStringLiteral(" ") + newPath;
                Q_EMIT error(Error::Type::StorageMoveError, QString(), QString(), moveJob->error(), moveJob->errorString(), title);
            }
            Q_EMIT storageMoveFinished();
            Q_EMIT storagePathChanged(newPath);

            // save settings now to avoid getting into an inconsistent app state
            SettingsManager::self()->save();
            disconnect(this, &StorageManager::cancelStorageMove, this, nullptr);
        });
        connect(this, &StorageManager::cancelStorageMove, this, [moveJob]() {
            moveJob->doKill();
        });
        Q_EMIT storageMoveStarted();
        moveJob->start();
    }
}

QString StorageManager::imageDirPath() const
{
    QString path = storagePath() + QStringLiteral("/images/");
    // Create path if it doesn't exist yet
    QFileInfo().absoluteDir().mkpath(path);
    return path;
}

QString StorageManager::imagePath(const QString &url) const
{
    return imageDirPath() + QString::fromStdString(QCryptographicHash::hash(url.toUtf8(), QCryptographicHash::Md5).toHex().toStdString());
}

QString StorageManager::enclosureDirPath() const
{
    return enclosureDirPath(QStringLiteral(""));
}

QString StorageManager::enclosureDirPath(const QString &feedname) const
{
    QString path = storagePath() + QStringLiteral("/enclosures/");

    if (!feedname.isEmpty()) {
        path += feedname + QStringLiteral("/");
    }

    // Create path if it doesn't exist yet
    QFileInfo().absoluteDir().mkpath(path);
    return path;
}

QString StorageManager::enclosurePath(const QString &name, const QString &url, const QString &feedname) const
{
    // Generate filename based on episode name and url hash with feedname as subdirectory
    QString enclosureFilenameBase = sanitizedFilePath(name) + QStringLiteral(".")
        + QString::fromStdString(QCryptographicHash::hash(url.toUtf8(), QCryptographicHash::Md5).toHex().toStdString()).left(6);

    QString enclosureFilenameExt = QFileInfo(QUrl::fromUserInput(url).fileName()).suffix();

    QString enclosureFilename = !enclosureFilenameExt.isEmpty() ? enclosureFilenameBase + QStringLiteral(".") + enclosureFilenameExt : enclosureFilenameBase;

    return enclosureDirPath(feedname) + enclosureFilename;
}

qint64 StorageManager::dirSize(const QString &path) const
{
    qint64 size = 0;
    QFileInfoList files = QDir(path).entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);

    for (QFileInfo info : files) {
        if (info.isDir()) {
            size += dirSize(info.filePath());
        }
        size += info.size();
    }

    return size;
}

void StorageManager::removeImage(const QString &url)
{
    qCDebug(kastsStorageManager) << "Removing image" << imagePath(url);
    QFile(imagePath(url)).remove();
    Q_EMIT imageDirSizeChanged();
}

void StorageManager::clearImageCache()
{
    qCDebug(kastsStorageManager) << imageDirPath();
    QStringList images = QDir(imageDirPath()).entryList(QDir::Files);
    qCDebug(kastsStorageManager) << images;
    for (QString image : images) {
        qCDebug(kastsStorageManager) << image;
        QFile(QDir(imageDirPath()).absoluteFilePath(image)).remove();
    }
    Q_EMIT imageDirSizeChanged();
}

qint64 StorageManager::enclosureDirSize() const
{
    return dirSize(enclosureDirPath());
}

qint64 StorageManager::imageDirSize() const
{
    return dirSize(imageDirPath());
}

QString StorageManager::formattedEnclosureDirSize() const
{
    return m_kformat.formatByteSize(enclosureDirSize());
}

QString StorageManager::formattedImageDirSize() const
{
    return m_kformat.formatByteSize(imageDirSize());
}

QString StorageManager::passwordFilePath(const QString &username) const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/") + username;
}

QString StorageManager::sanitizedFilePath(const QString &path) const
{
    // NOTE: Any changes here require a database migration!

    // Only keep alphanumeric ascii characters; this avoid any kind of issues
    // with the many types of filesystems out there.  Then remove excess whitespace
    // and limit the length of the string.
    QString newPath = path;
    newPath = newPath.remove(QRegularExpression(QStringLiteral("[^a-zA-Z0-9 ._()-]"))).simplified().left(maxFilenameLength);

    return newPath.isEmpty() ? QStringLiteral("Noname") : newPath;
}
