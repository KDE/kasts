/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <tobias.fella@kde.org>
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "enclosure.h"
#include "enclosurelogging.h"

#include <KLocalizedString>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QImage>
#include <QMimeDatabase>
#include <QNetworkReply>
#include <QSqlQuery>

#include <attachedpictureframe.h>
#include <fileref.h>
#include <id3v2frame.h>
#include <id3v2tag.h>
#include <mpegfile.h>

#include "audiomanager.h"
#include "database.h"
#include "datamanager.h"
#include "datatypes.h"
#include "entry.h"
#include "fetcher.h"
#include "models/errorlogmodel.h"
#include "objectslogging.h"
#include "utils/enclosuredownloadjob.h"
#include "utils/networkconnectionmanager.h"
#include "utils/storagemanager.h"

Enclosure::Enclosure(Entry *entry)
    : QObject(entry)
    , m_entry(entry)
{
    connect(this, &Enclosure::downloadError, &ErrorLogModel::instance(), &ErrorLogModel::monitorErrorMessages);
    connect(&Fetcher::instance(), &Fetcher::entriesUpdated, this, [this](const QList<qint64> &entryuids) {
        if (entryuids.contains(m_entryuid)) {
            updateFromDb();
        }
    });
    connect(&DataManager::instance(), &DataManager::entryPlayPositionsChanged, this, [this](const QList<qint64> &positions, const QList<qint64> &entryuids) {
        if (entryuids.contains(m_entryuid)) {
            qint64 index = entryuids.indexOf(m_entryuid);
            Q_ASSERT(index > -1);
            m_playposition = positions[index];
            Q_EMIT playPositionChanged();
        }
    });
    connect(&DataManager::instance(), &DataManager::enclosureDurationsChanged, this, [this](const QList<qint64> &durations, const QList<qint64> &entryuids) {
        if (entryuids.contains(m_entryuid)) {
            qint64 index = entryuids.indexOf(m_entryuid);
            Q_ASSERT(index > -1);
            m_duration = durations[index];
            Q_EMIT durationChanged();
        }
    });
    connect(&DataManager::instance(), &DataManager::enclosureSizesChanged, this, [this](const QList<qint64> &sizes, const QList<qint64> &entryuids) {
        if (entryuids.contains(m_entryuid)) {
            qint64 index = entryuids.indexOf(m_entryuid);
            Q_ASSERT(index > -1);
            m_size = sizes[index];
            Q_EMIT sizeChanged();
        }
    });
    connect(&DataManager::instance(),
            &DataManager::enclosureStatusesChanged,
            this,
            [this](const QList<DataTypes::EnclosureStatus> &statuses, const QList<qint64> &entryuids) {
                if (entryuids.contains(m_entryuid)) {
                    qint64 index = entryuids.indexOf(m_entryuid);
                    Q_ASSERT(index > -1);
                    m_status = statuses[index];
                    m_downloadProgress = 0;
                    m_downloadSize = 0;
                    Q_EMIT statusChanged(m_entry, m_status);
                }
            });
    // This connection is there to keep other objects of the currently playing
    // enclosure in sync without writing the position to the DB (yet)
    connect(&AudioManager::instance(), &AudioManager::positionChanged, this, [this](const qint64 position, const qint64 entryuid) {
        if (entryuid == m_entryuid) {
            m_playposition = position;
            Q_EMIT playPositionChanged();
        }
    });

    // TODO: this will just take the first enclosure found; we should handle
    // multiple ones
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT * FROM Enclosures WHERE entryuid=:entryuid"));
    query.bindValue(QStringLiteral(":entryuid"), entry->entryuid());
    Database::instance().execute(query);

    if (!query.next()) {
        return;
    }

    m_entryuid = query.value(QStringLiteral("entryuid")).toLongLong();
    m_enclosureuid = query.value(QStringLiteral("enclosureuid")).toLongLong();
    m_duration = query.value(QStringLiteral("duration")).toInt();
    m_size = query.value(QStringLiteral("size")).toInt();
    m_type = query.value(QStringLiteral("type")).toString();
    m_url = query.value(QStringLiteral("url")).toString();
    m_playposition = query.value(QStringLiteral("playposition")).toLongLong();
    m_status = DataTypes::dbToStatus(query.value(QStringLiteral("downloaded")).toInt());
    m_playposition_dbsave = m_playposition;

    // using qtimer to do this update after the constructor so the signals can be picked up correctly
    QTimer::singleShot(0, this, &Enclosure::checkSizeOnDisk);

    qCDebug(kastsObjects) << "Enclosure object" << m_enclosureuid << "constructed (corresponding entryuid is" << m_entryuid << ")";
}

Enclosure::~Enclosure()
{
    qCDebug(kastsObjects) << "Enclosure object" << m_enclosureuid << "destructed (corresponding entryuid is" << m_entryuid << ")";
}

void Enclosure::updateFromDb()
{
    // This method is used to update the most relevant fields from the RSS feed,
    // most notably the download URL.  It's deliberatly only updating the
    // duration and size if the URL has changed, since these values are
    // notably untrustworthy.  We generally get them from the files themselves
    // at the time they are downloaded.
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT * FROM Enclosures WHERE enclosureuid=:enclosureuid"));
    query.bindValue(QStringLiteral(":enclosureuid"), m_enclosureuid);
    Database::instance().execute(query);

    if (!query.next()) {
        return;
    }

    if (m_url != query.value(QStringLiteral("url")).toString() && m_status != DataTypes::EnclosureStatus::Downloaded) {
        // this means that the audio file has changed, or at least its location
        // let's only do something if the file isn't downloaded.
        // try to delete the file first (it actually shouldn't exist)
        deleteFile();

        m_url = query.value(QStringLiteral("url")).toString();
        Q_EMIT urlChanged(m_url);
        Q_EMIT pathChanged(path());

        if (m_duration != query.value(QStringLiteral("duration")).toInt()) {
            m_duration = query.value(QStringLiteral("duration")).toInt();
            Q_EMIT durationChanged();
        }
        if (m_size != query.value(QStringLiteral("size")).toInt()) {
            m_size = query.value(QStringLiteral("size")).toInt();
            Q_EMIT sizeChanged();
        }
        if (m_type != query.value(QStringLiteral("type")).toString()) {
            m_type = query.value(QStringLiteral("type")).toString();
            Q_EMIT typeChanged(m_type);
        }
    }
}

void Enclosure::download()
{
    if (m_status == DataTypes::EnclosureStatus::Downloaded) {
        return;
    }

    // TODO: move this check to fetcher; needs error refactoring to use uids
    if (!NetworkConnectionManager::instance().episodeDownloadsAllowed()) {
        if (NetworkConnectionManager::instance().networkReachable()) {
            Q_EMIT downloadError(
                ErrorLogModel::Type::MeteredStreamingNotAllowed,
                i18nc("@info:status Error message notification", "Download of episode %1 not allowed on metered connection", m_entry->title()));
            return;
        } else {
            Q_EMIT downloadError(
                ErrorLogModel::Type::NoNetwork,
                i18nc("@info:status Error message notification", "No network connection while attempting to download episode %1", m_entry->title()));
            return;
        }
    }

    checkSizeOnDisk();
    EnclosureDownloadJob *downloadJob = Fetcher::instance().enqueueEnclosureDownload(m_entryuid, m_url, path(), m_entry->title());

    qint64 resumedAt = m_sizeOnDisk;
    m_downloadProgress = 0;
    m_downloadSize = 0;
    Q_EMIT downloadProgressChanged();

    connect(downloadJob, &KJob::result, this, [this, downloadJob]() {
        checkSizeOnDisk();
        if (downloadJob->error() == 0) {
            processDownloadedFile();
        } else {
            QFile file(path());
            if (file.exists() && file.size() > 0) {
                DataManager::instance().bulkSetEnclosureStatuses(QList<DataTypes::EnclosureStatus>({DataTypes::EnclosureStatus::PartiallyDownloaded}),
                                                                 QList<qint64>({m_entryuid}));
            } else {
                DataManager::instance().bulkSetEnclosureStatuses(QList<DataTypes::EnclosureStatus>({DataTypes::EnclosureStatus::Downloadable}),
                                                                 QList<qint64>({m_entryuid}));
            }
            if (downloadJob->error() != QNetworkReply::OperationCanceledError) {
                Q_EMIT downloadError(ErrorLogModel::Type::MediaDownload,
                                     i18nc("@info:status Error message notification", "Error downloading media: %1", downloadJob->errorString()));
            }
        }
        disconnect(this, &Enclosure::cancelDownload, this, nullptr);
        Q_EMIT statusChanged(m_entry, m_status);
    });

    connect(this, &Enclosure::cancelDownload, this, [this, downloadJob]() {
        downloadJob->doKill();
        checkSizeOnDisk();
        QFile file(path());
        if (file.exists() && file.size() > 0) {
            DataManager::instance().bulkSetEnclosureStatuses(QList<DataTypes::EnclosureStatus>({DataTypes::EnclosureStatus::PartiallyDownloaded}),
                                                             QList<qint64>({m_entryuid}));
        } else {
            DataManager::instance().bulkSetEnclosureStatuses(QList<DataTypes::EnclosureStatus>({DataTypes::EnclosureStatus::Downloadable}),
                                                             QList<qint64>({m_entryuid}));
        }
        disconnect(this, &Enclosure::cancelDownload, this, nullptr);
    });

    connect(downloadJob, &KJob::processedAmountChanged, this, [this, resumedAt](KJob *kjob, KJob::Unit unit, qulonglong amount) {
        Q_ASSERT(unit == KJob::Unit::Bytes);

        DataManager::instance().bulkSetEnclosureStatuses(QList<DataTypes::EnclosureStatus>({DataTypes::EnclosureStatus::Downloading}),
                                                         QList<qint64>({m_entryuid}));

        qint64 totalSize = static_cast<qint64>(kjob->totalAmount(unit));
        qint64 currentSize = static_cast<qint64>(amount);

        if ((totalSize > 0) && (m_size != totalSize + resumedAt)) {
            qCDebug(kastsEnclosure) << "Correct filesize for enclosure" << m_entryuid << "from" << m_size << "to" << totalSize + resumedAt;
            DataManager::instance().bulkSetEnclosureSizes(QList<qint64>({totalSize + resumedAt}), QList<qint64>({m_entryuid}));
        }

        m_downloadSize = currentSize + resumedAt;
        m_downloadProgress = static_cast<double>(m_downloadSize) / static_cast<double>(m_size);
        Q_EMIT downloadProgressChanged();

        qCDebug(kastsEnclosure) << "m_downloadSize" << m_downloadSize;
        qCDebug(kastsEnclosure) << "m_downloadProgress" << m_downloadProgress;
        qCDebug(kastsEnclosure) << "m_size" << m_size;
    });

    DataManager::instance().bulkSetEnclosureStatuses(QList<DataTypes::EnclosureStatus>({DataTypes::EnclosureStatus::Queued}), QList<qint64>({m_entryuid}));
}

void Enclosure::processDownloadedFile()
{
    // This will be run if the enclosure has been downloaded successfully

    // First check if file size is larger than 0; otherwise something unexpected
    // must have happened
    checkSizeOnDisk();
    if (m_sizeOnDisk == 0) {
        deleteFile();
        return;
    }

    // Check if reported filesize in rss feed corresponds to real file size
    // if not, correct the filesize in the database
    // otherwise the file will get deleted because of mismatch in signature
    if (m_sizeOnDisk != size()) {
        qCDebug(kastsEnclosure) << "Correcting enclosure file size mismatch for" << m_entry->title() << "from" << size() << "to" << m_sizeOnDisk;
        DataManager::instance().bulkSetEnclosureSizes(QList<qint64>({m_sizeOnDisk}), QList<qint64>({m_entryuid}));
        DataManager::instance().bulkSetEnclosureStatuses(QList<DataTypes::EnclosureStatus>({DataTypes::EnclosureStatus::Downloaded}),
                                                         QList<qint64>({m_entryuid}));
    }

    // Check the duration inside the tag, it should be more accurate than the
    // value from the feed entry
    TagLib::FileRef f(path().toStdString().data());
    if (!f.isNull() && f.audioProperties()) {
        int fileduration = f.audioProperties()->lengthInSeconds();
        if (fileduration > 0 && fileduration != duration()) {
            qCDebug(kastsEnclosure) << "Correcting enclosure duration mismatch for" << m_entry->title() << "from" << duration() << "to" << fileduration;
            DataManager::instance().bulkSetEnclosureDurations(QList<qint64>({fileduration}), QList<qint64>({m_entryuid}));
        }
    }

    // Unset "new" status of item
    if (m_entry->getNew()) {
        DataManager::instance().bulkMarkNew(false, QList<qint64>({m_entryuid}));
    }

    // Trigger update of image since the downloaded file can have an embedded image
    Q_EMIT m_entry->imageChanged(m_entry->image());
    // TODO: update of the image should be triggered in the model based on a downloadedChanged signal sent by Fetcher(?)
    // once this method has moved to Fetcher
}

void Enclosure::deleteFile()
{
    DataManager::instance().bulkDeleteEnclosures(QList<qint64>({m_entryuid}));
}

qint64 Enclosure::enclosureuid() const
{
    return m_enclosureuid;
}

QString Enclosure::url() const
{
    return m_url;
}

QString Enclosure::path() const
{
    return StorageManager::enclosurePath(m_entry->title(), m_url, m_entry->feed()->dirname());
}

DataTypes::EnclosureStatus Enclosure::status() const
{
    return m_status;
}

qint64 Enclosure::playPosition() const
{
    return m_playposition;
}

qint64 Enclosure::duration() const
{
    return m_duration;
}

qint64 Enclosure::size() const
{
    return m_size;
}

void Enclosure::setPlayPosition(const qint64 &position)
{
    if (m_playposition != position) {
        m_playposition = position;
        DataManager::instance().bulkSetPlayPositions(QList<qint64>({position}), QList<qint64>({m_entryuid}));
        qCDebug(kastsEnclosure) << "save playPosition" << position << m_entry->title();
    }
}

void Enclosure::checkSizeOnDisk()
{
    // In principle the database contains this status, we check anyway in case
    // something changed on disk
    QFile file(path());
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

qint64 Enclosure::downloadSize() const
{
    return m_downloadSize;
}
