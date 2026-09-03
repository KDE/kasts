/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <tobias.fella@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "enclosuredownloadjob.h"
#include "enclosuredownloadlogging.h"

#include <QNetworkReply>
#include <QTimer>

#include <KLocalizedString>

#include <attachedpictureframe.h>
#include <fileref.h>
#include <id3v2frame.h>
#include <id3v2tag.h>
#include <mpegfile.h>

#include "datamanager.h"
#include "datatypes.h"
#include "fetcher.h"
#include "objectslogging.h"

EnclosureDownloadJob::EnclosureDownloadJob(const qint64 entryuid,
                                           const QString &url,
                                           const QString &filename,
                                           const QString &title,
                                           const qint64 size,
                                           const qint64 duration,
                                           QObject *parent)
    : KJob(parent)
    , m_entryuid(entryuid)
    , m_url(url)
    , m_filename(filename)
    , m_title(title)
    , m_size(size)
    , m_duration(duration)
{
    setCapabilities(Killable);
    qCDebug(kastsObjects) << "Constructed EnclosureDownloadJob" << entryuid << url;
}

EnclosureDownloadJob::~EnclosureDownloadJob()
{
    qCDebug(kastsObjects) << "Destructed EnclosureDownloadJob" << m_entryuid << m_url;
}

void EnclosureDownloadJob::start()
{
    QTimer::singleShot(0, this, &EnclosureDownloadJob::startDownload);
}

void EnclosureDownloadJob::startDownload()
{
    checkSizeOnDisk();
    qint64 resumedAt = m_sizeOnDisk;
    m_downloadProgress = 0;
    m_downloadSize = 0;

    m_status = EnclosureDownloadJob::Status::Downloading;
    Q_EMIT statusChanged(m_status);

    DataManager::instance().bulkSetEnclosureStatuses(QList<DataTypes::EnclosureStatus>({DataTypes::EnclosureStatus::Downloading}), QList<qint64>({m_entryuid}));

    m_reply = getNetworkReply(m_url, m_filename);

    if (!m_reply) {
        setError(1);
        setErrorText(QStringLiteral("Cannot open file to write download to %1").arg(m_filename));
        emitResult();
        return;
    }

    // TODO: do we really need the entry title only for the description which is not realy used otherwise?
    Q_EMIT description(this, i18n("Downloading %1", m_title));

    connect(m_reply, &QNetworkReply::downloadProgress, this, [this, resumedAt](qint64 received, qint64 total) {
        DataManager::instance().bulkSetEnclosureStatuses(QList<DataTypes::EnclosureStatus>({DataTypes::EnclosureStatus::Downloading}),
                                                         QList<qint64>({m_entryuid}));

        if ((total > 0) && (m_size != total + resumedAt)) {
            qCDebug(kastsEnclosureDownload) << "Correct filesize for enclosure" << m_entryuid << "from" << m_size << "to" << total + resumedAt;
            m_size = total + resumedAt;
            DataManager::instance().bulkSetEnclosureSizes(QList<qint64>({total + resumedAt}), QList<qint64>({m_entryuid}));
        }

        setProcessedAmount(Bytes, received + resumedAt);
        setTotalAmount(Bytes, total);
    });

    connect(m_reply, &QNetworkReply::finished, this, [this]() {
        if (error() == 0 && m_status != EnclosureDownloadJob::Status::Canceled) {
            processDownloadedFile();
        } else {
            QFile file(m_filename);
            if (file.exists() && file.size() > 0) {
                DataManager::instance().bulkSetEnclosureStatuses(QList<DataTypes::EnclosureStatus>({DataTypes::EnclosureStatus::PartiallyDownloaded}),
                                                                 QList<qint64>({m_entryuid}));
            } else {
                DataManager::instance().bulkSetEnclosureStatuses(QList<DataTypes::EnclosureStatus>({DataTypes::EnclosureStatus::Downloadable}),
                                                                 QList<qint64>({m_entryuid}));
            }
        }
        m_reply->deleteLater();
        emitResult();
    });

    connect(m_reply, &QNetworkReply::errorOccurred, this, [this](QNetworkReply::NetworkError code) {
        setError(code);
        setErrorText(m_reply->errorString());
    });
}

QNetworkReply *EnclosureDownloadJob::getNetworkReply(const QString &url, const QString &filePath) const
{
    QNetworkRequest request((QUrl(url)));
    request.setTransferTimeout();

    bool fileOpenSuccess = false;

    QFile *file = new QFile(filePath);
    if (file->exists() && file->size() > 0) {
        // try to resume download
        int resumedAt = file->size();
        qCDebug(kastsEnclosureDownload) << "Resuming download at" << resumedAt << "bytes";
        QByteArray rangeHeaderValue = QByteArray("bytes=") + QByteArray::number(resumedAt) + QByteArray("-");
        request.setRawHeader(QByteArray("Range"), rangeHeaderValue);
        fileOpenSuccess = file->open(QIODevice::WriteOnly | QIODevice::Append);
    } else {
        qCDebug(kastsEnclosureDownload) << "Starting new download";
        fileOpenSuccess = file->open(QIODevice::WriteOnly);
    }

    if (!fileOpenSuccess) {
        return nullptr;
    }

    QNetworkReply *reply = Fetcher::instance().get(request);

    connect(reply, &QNetworkReply::readyRead, this, [=]() {
        if (reply->isOpen() && file) {
            QByteArray data = reply->readAll();
            file->write(data);
        }
    });

    connect(reply, &QNetworkReply::finished, this, [reply, url, file]() {
        if (reply->isOpen() && file) {
            QByteArray data = reply->readAll();
            file->write(data);
            file->close();
        }

        // clean up; close file if still open in case something has gone wrong
        if (file) {
            if (file->isOpen()) {
                file->close();
            }
            delete file;
        }
        reply->deleteLater();
    });

    return reply;
}

EnclosureDownloadJob::Status EnclosureDownloadJob::status() const
{
    return m_status;
}

qint64 EnclosureDownloadJob::entryuid() const
{
    return m_entryuid;
}

bool EnclosureDownloadJob::doKill()
{
    m_status = EnclosureDownloadJob::Status::Canceled;
    Q_EMIT statusChanged(m_status);

    if (m_reply) {
        m_reply->abort();
    } else {
        emitResult();
    }

    return true;
}

void EnclosureDownloadJob::checkSizeOnDisk()
{
    // In principle the database contains this status, we check anyway in case
    // something changed on disk
    QFile file(m_filename);
    if (file.exists()) {
        if (file.size() == m_size && file.size() > 0) {
            // file is on disk and has correct size, write to database if it
            // wasn't already registered so
            // this should, in principle, never happen unless the db was deleted
            DataManager::instance().bulkSetEnclosureStatuses(QList<DataTypes::EnclosureStatus>({DataTypes::EnclosureStatus::Downloaded}),
                                                             QList<qint64>({m_entryuid}));
        } else if (file.size() > 0) {
            // file was downloaded, but there is a size mismatch
            // set to PartiallyDownloaded such that download can be resumed
            DataManager::instance().bulkSetEnclosureStatuses(QList<DataTypes::EnclosureStatus>({DataTypes::EnclosureStatus::PartiallyDownloaded}),
                                                             QList<qint64>({m_entryuid}));
        } else {
            // file is empty
            DataManager::instance().bulkSetEnclosureStatuses(QList<DataTypes::EnclosureStatus>({DataTypes::EnclosureStatus::Downloadable}),
                                                             QList<qint64>({m_entryuid}));
        }
        if (file.size() != m_sizeOnDisk) {
            m_sizeOnDisk = file.size();
            m_downloadSize = m_sizeOnDisk;
            m_downloadProgress = (m_size == 0) ? 0.0 : static_cast<double>(m_sizeOnDisk) / static_cast<double>(m_size);
        }
    } else {
        // file does not exist
        DataManager::instance().bulkSetEnclosureStatuses(QList<DataTypes::EnclosureStatus>({DataTypes::EnclosureStatus::Downloadable}),
                                                         QList<qint64>({m_entryuid}));
        if (m_sizeOnDisk != 0) {
            m_sizeOnDisk = 0;
            m_downloadSize = 0;
            m_downloadProgress = 0.0;
        }
    }
}

void EnclosureDownloadJob::processDownloadedFile()
{
    // This will be run if the enclosure has been downloaded successfully

    // First check if file size is larger than 0; otherwise something unexpected
    // must have happened
    checkSizeOnDisk();
    if (m_sizeOnDisk == 0) {
        DataManager::instance().bulkDeleteEnclosures(QList<qint64>({m_entryuid}));
        return;
    }

    // Check if reported filesize in rss feed corresponds to real file size
    // if not, correct the filesize in the database
    // otherwise the file will get deleted because of mismatch in signature
    if (m_sizeOnDisk != m_size) {
        qCDebug(kastsEnclosureDownload) << "Correcting enclosure file size mismatch for" << m_title << "from" << m_size << "to" << m_sizeOnDisk;
        DataManager::instance().bulkSetEnclosureSizes(QList<qint64>({m_sizeOnDisk}), QList<qint64>({m_entryuid}));
        DataManager::instance().bulkSetEnclosureStatuses(QList<DataTypes::EnclosureStatus>({DataTypes::EnclosureStatus::Downloaded}),
                                                         QList<qint64>({m_entryuid}));
    }

    // Check the duration inside the tag, it should be more accurate than the
    // value from the feed entry
    TagLib::FileRef f(m_filename.toStdString().data());
    if (!f.isNull() && f.audioProperties()) {
        int fileduration = f.audioProperties()->lengthInSeconds();
        if (fileduration > 0 && fileduration != m_duration) {
            qCDebug(kastsEnclosureDownload) << "Correcting enclosure duration mismatch for" << m_title << "from" << m_duration << "to" << fileduration;
            DataManager::instance().bulkSetEnclosureDurations(QList<qint64>({fileduration}), QList<qint64>({m_entryuid}));
        }
    }

    // Unset "new" status of item
    DataManager::instance().bulkMarkNew(false, QList<qint64>({m_entryuid}));

    // Trigger update of image since the downloaded file can have an embedded image
    // Q_EMIT m_entry->imageChanged(m_entry->image());
    // FIXME: update of the image should be triggered in the model based on a downloadedChanged signal sent by Fetcher(?)
    // once this method has moved to Fetcher
}
