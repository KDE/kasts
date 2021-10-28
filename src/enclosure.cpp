/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "enclosure.h"
#include "enclosurelogging.h"

#include <KLocalizedString>
#include <QFile>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QNetworkReply>
#include <QSqlQuery>

#include <attachedpictureframe.h>
#include <id3v2frame.h>
#include <id3v2tag.h>
#include <mpegfile.h>

#include "audiomanager.h"
#include "database.h"
#include "datamanager.h"
#include "enclosuredownloadjob.h"
#include "entry.h"
#include "error.h"
#include "fetcher.h"
#include "models/downloadmodel.h"
#include "models/errorlogmodel.h"
#include "settingsmanager.h"
#include "storagemanager.h"
#include "sync/sync.h"

#include <solidextras/networkstatus.h>

Enclosure::Enclosure(Entry *entry)
    : QObject(entry)
    , m_entry(entry)
{
    connect(this, &Enclosure::playPositionChanged, this, &Enclosure::leftDurationChanged);
    connect(this, &Enclosure::statusChanged, &DownloadModel::instance(), &DownloadModel::monitorDownloadStatus);
    connect(this, &Enclosure::downloadError, &ErrorLogModel::instance(), &ErrorLogModel::monitorErrorMessages);
    connect(&Fetcher::instance(), &Fetcher::entryUpdated, this, [this](const QString &url, const QString &id) {
        if ((m_entry->feed()->url() == url) && (m_entry->id() == id)) {
            updateFromDb();
        }
    });

    // we use the relayed signal from AudioManager::playbackRateChanged by
    // DataManager; this is required to avoid a dependency loop on startup
    connect(&DataManager::instance(), &DataManager::playbackRateChanged, this, &Enclosure::leftDurationChanged);

    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT * FROM Enclosures WHERE id=:id"));
    query.bindValue(QStringLiteral(":id"), entry->id());
    Database::instance().execute(query);

    if (!query.next()) {
        return;
    }

    m_duration = query.value(QStringLiteral("duration")).toInt();
    m_size = query.value(QStringLiteral("size")).toInt();
    m_title = query.value(QStringLiteral("title")).toString();
    m_type = query.value(QStringLiteral("type")).toString();
    m_url = query.value(QStringLiteral("url")).toString();
    m_playposition = query.value(QStringLiteral("playposition")).toLongLong();
    m_status = dbToStatus(query.value(QStringLiteral("downloaded")).toInt());
    m_playposition_dbsave = m_playposition;

    checkSizeOnDisk();
}

void Enclosure::updateFromDb()
{
    // This method is used to update the most relevant fields from the RSS feed,
    // most notably the download URL.  It's deliberatly only updating the
    // duration and size if the URL has changed, since these values are
    // notably untrustworthy.  We generally get them from the files themselves
    // at the time they are downloaded.
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT * FROM Enclosures WHERE id=:id"));
    query.bindValue(QStringLiteral(":id"), m_entry->id());
    Database::instance().execute(query);

    if (!query.next()) {
        return;
    }

    if (m_url != query.value(QStringLiteral("url")).toString() && m_status != Downloaded) {
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
        if (m_title != query.value(QStringLiteral("title")).toString()) {
            m_title = query.value(QStringLiteral("title")).toString();
            Q_EMIT titleChanged(m_title);
        }
        if (m_type != query.value(QStringLiteral("type")).toString()) {
            m_type = query.value(QStringLiteral("type")).toString();
            Q_EMIT typeChanged(m_type);
        }
    }
}

int Enclosure::statusToDb(Enclosure::Status status)
{
    switch (status) {
    case Enclosure::Status::Downloadable:
        return 0;
    case Enclosure::Status::Downloading:
        return 1;
    case Enclosure::Status::PartiallyDownloaded:
        return 2;
    case Enclosure::Status::Downloaded:
        return 3;
    default:
        return -1;
    }
}

Enclosure::Status Enclosure::dbToStatus(int value)
{
    switch (value) {
    case 0:
        return Enclosure::Status::Downloadable;
    case 1:
        return Enclosure::Status::Downloading;
    case 2:
        return Enclosure::Status::PartiallyDownloaded;
    case 3:
        return Enclosure::Status::Downloaded;
    default:
        return Enclosure::Status::Error;
    }
}

void Enclosure::download()
{
    if (m_status == Downloaded) {
        return;
    }

    if (SolidExtras::NetworkStatus().metered() == SolidExtras::NetworkStatus::Yes && !SettingsManager::self()->allowMeteredEpisodeDownloads()) {
        Q_EMIT downloadError(Error::Type::MeteredNotAllowed,
                             m_entry->feed()->url(),
                             m_entry->id(),
                             0,
                             i18n("Podcast downloads not allowed due to user setting"),
                             QString());
        return;
    }

    checkSizeOnDisk();
    EnclosureDownloadJob *downloadJob = new EnclosureDownloadJob(m_url, path(), m_entry->title(), m_entry->feed()->url());
    downloadJob->start();

    qint64 resumedAt = m_sizeOnDisk;
    m_downloadProgress = 0;
    Q_EMIT downloadProgressChanged();

    m_entry->feed()->setErrorId(0);
    m_entry->feed()->setErrorString(QString());

    connect(downloadJob, &KJob::result, this, [this, downloadJob]() {
        checkSizeOnDisk();
        if (downloadJob->error() == 0) {
            processDownloadedFile();
        } else {
            QFile file(path());
            if (file.exists() && file.size() > 0) {
                setStatus(PartiallyDownloaded);
            } else {
                setStatus(Downloadable);
            }
            /*if (downloadJob->error() == QNetworkReply::InsecureRedirectError) {
                // Ask user to allow insecure redirects for this feed

            } else  */
            if (downloadJob->error() != QNetworkReply::OperationCanceledError) {
                m_entry->feed()->setErrorId(downloadJob->error());
                m_entry->feed()->setErrorString(downloadJob->errorString());
                Q_EMIT downloadError(Error::Type::MediaDownload,
                                     m_entry->feed()->url(),
                                     m_entry->id(),
                                     downloadJob->error(),
                                     downloadJob->errorString(),
                                     QString());
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
            setStatus(PartiallyDownloaded);
        } else {
            setStatus(Downloadable);
        }
        disconnect(this, &Enclosure::cancelDownload, this, nullptr);
    });

    connect(downloadJob, &KJob::processedAmountChanged, this, [=](KJob *kjob, KJob::Unit unit, qulonglong amount) {
        Q_ASSERT(unit == KJob::Unit::Bytes);

        qint64 totalSize = static_cast<qint64>(kjob->totalAmount(unit));
        qint64 currentSize = static_cast<qint64>(amount);

        if ((totalSize > 0) && (m_size != totalSize + resumedAt)) {
            qCDebug(kastsEnclosure) << "Correct filesize for enclosure" << m_entry->title() << "from" << m_size << "to" << totalSize + resumedAt;
            setSize(totalSize + resumedAt);
        }

        m_downloadSize = currentSize + resumedAt;
        m_downloadProgress = static_cast<double>(m_downloadSize) / static_cast<double>(m_size);
        Q_EMIT downloadProgressChanged();

        qCDebug(kastsEnclosure) << "m_downloadSize" << m_downloadSize;
        qCDebug(kastsEnclosure) << "m_downloadProgress" << m_downloadProgress;
        qCDebug(kastsEnclosure) << "m_size" << m_size;
    });

    setStatus(Downloading);
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
        qCDebug(kastsEnclosure) << "Correcting enclosure file size mismatch" << m_entry->title() << "from" << size() << "to" << m_sizeOnDisk;
        setSize(m_sizeOnDisk);
        setStatus(Downloaded);
    }

    // Unset "new" status of item
    if (m_entry->getNew()) {
        m_entry->setNew(false);
    }
}

void Enclosure::deleteFile()
{
    qCDebug(kastsEnclosure) << "Trying to delete enclosure file" << path();
    if (AudioManager::instance().entry() && (m_entry == AudioManager::instance().entry())) {
        qCDebug(kastsEnclosure) << "Track is still playing; let's unload it before deleting";
        AudioManager::instance().setEntry(nullptr);
    }

    // First check if file still exists; you never know what has happened
    if (QFile(path()).exists()) {
        QFile(path()).remove();
    }

    // If file disappeared unexpectedly, then still change status to downloadable
    setStatus(Downloadable);
    m_sizeOnDisk = 0;
    Q_EMIT sizeOnDiskChanged();
}

QString Enclosure::url() const
{
    return m_url;
}

QString Enclosure::path() const
{
    return StorageManager::instance().enclosurePath(m_url);
}

Enclosure::Status Enclosure::status() const
{
    return m_status;
}

QString Enclosure::cachedEmbeddedImage() const
{
    // if image is already cached, then return the path
    QString cachedpath = StorageManager::instance().imagePath(path());
    if (QFileInfo::exists(cachedpath)) {
        if (QFileInfo(cachedpath).size() != 0) {
            return QUrl::fromLocalFile(cachedpath).toString();
        }
    }

    if (m_status != Downloaded || path().isEmpty()) {
        return QStringLiteral("");
    }

    const auto mime = QMimeDatabase().mimeTypeForFile(path()).name();
    if (mime != QStringLiteral("audio/mpeg")) {
        return QStringLiteral("");
    }

    TagLib::MPEG::File f(path().toLatin1().data());
    if (!f.hasID3v2Tag()) {
        return QStringLiteral("");
    }

    bool imageFound = false;
    for (const auto &frame : f.ID3v2Tag()->frameListMap()["APIC"]) {
        auto pictureFrame = dynamic_cast<TagLib::ID3v2::AttachedPictureFrame *>(frame);
        QByteArray data(pictureFrame->picture().data(), pictureFrame->picture().size());
        if (!data.isEmpty()) {
            QFile file(cachedpath);
            file.open(QIODevice::WriteOnly);
            file.write(data);
            file.close();
            imageFound = true;
        }
    }

    if (imageFound) {
        return cachedpath;
    } else {
        return QStringLiteral("");
    }
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

qint64 Enclosure::sizeOnDisk() const
{
    return m_sizeOnDisk;
}

void Enclosure::setStatus(Enclosure::Status status)
{
    if (m_status != status) {
        m_status = status;

        QSqlQuery query;
        query.prepare(QStringLiteral("UPDATE Enclosures SET downloaded=:downloaded WHERE id=:id AND feed=:feed;"));
        query.bindValue(QStringLiteral(":id"), m_entry->id());
        query.bindValue(QStringLiteral(":feed"), m_entry->feed()->url());
        query.bindValue(QStringLiteral(":downloaded"), statusToDb(m_status));
        Database::instance().execute(query);

        Q_EMIT statusChanged(m_entry, m_status);
    }
}

void Enclosure::setPlayPosition(const qint64 &position)
{
    if (m_playposition != position) {
        m_playposition = position;
        qCDebug(kastsEnclosure) << "save playPosition" << position << m_entry->title();

        // let's only save the play position to the database every 15 seconds
        if ((abs(m_playposition - m_playposition_dbsave) > 15000) || position == 0) {
            qCDebug(kastsEnclosure) << "save playPosition to database" << position << m_entry->title();
            QSqlQuery query;
            query.prepare(QStringLiteral("UPDATE Enclosures SET playposition=:playposition WHERE id=:id AND feed=:feed"));
            query.bindValue(QStringLiteral(":id"), m_entry->id());
            query.bindValue(QStringLiteral(":feed"), m_entry->feed()->url());
            query.bindValue(QStringLiteral(":playposition"), m_playposition);
            Database::instance().execute(query);
            m_playposition_dbsave = m_playposition;

            // Also store position change to make sure that it can be synced to
            // e.g. gpodder
            Sync::instance().storePlayEpisodeAction(m_entry->id(), m_playposition_dbsave, m_playposition);
        }

        Q_EMIT playPositionChanged();
    }
}

void Enclosure::setDuration(const qint64 &duration)
{
    if (m_duration != duration) {
        m_duration = duration;

        // also save to database
        qCDebug(kastsEnclosure) << "updating entry duration" << duration << m_entry->title();
        QSqlQuery query;
        query.prepare(QStringLiteral("UPDATE Enclosures SET duration=:duration WHERE id=:id AND feed=:feed"));
        query.bindValue(QStringLiteral(":id"), m_entry->id());
        query.bindValue(QStringLiteral(":feed"), m_entry->feed()->url());
        query.bindValue(QStringLiteral(":duration"), m_duration);
        Database::instance().execute(query);

        Q_EMIT durationChanged();
    }
}

void Enclosure::setSize(const qint64 &size)
{
    if (m_size != size) {
        m_size = size;

        // also save to database
        QSqlQuery query;
        query.prepare(QStringLiteral("UPDATE Enclosures SET size=:size WHERE id=:id AND feed=:feed"));
        query.bindValue(QStringLiteral(":id"), m_entry->id());
        query.bindValue(QStringLiteral(":feed"), m_entry->feed()->url());
        query.bindValue(QStringLiteral(":size"), m_size);
        Database::instance().execute(query);

        Q_EMIT sizeChanged();
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
            setStatus(Downloaded);
        } else if (file.size() > 0) {
            // file was downloaded, but there is a size mismatch
            // set to PartiallyDownloaded such that download can be resumed
            setStatus(PartiallyDownloaded);
        } else {
            // file is empty
            setStatus(Downloadable);
        }
        if (file.size() != m_sizeOnDisk) {
            m_sizeOnDisk = file.size();
            m_downloadSize = m_sizeOnDisk;
            m_downloadProgress = (m_size == 0) ? 0.0 : static_cast<double>(m_sizeOnDisk) / static_cast<double>(m_size);
            Q_EMIT sizeOnDiskChanged();
        }
    } else {
        // file does not exist
        setStatus(Downloadable);
        if (m_sizeOnDisk != 0) {
            m_sizeOnDisk = 0;
            m_downloadSize = 0;
            m_downloadProgress = 0.0;
            Q_EMIT sizeOnDiskChanged();
        }
    }
}

QString Enclosure::formattedSize() const
{
    return m_kformat.formatByteSize(m_size);
}

QString Enclosure::formattedDownloadSize() const
{
    return m_kformat.formatByteSize(m_downloadSize);
}

QString Enclosure::formattedDuration() const
{
    return m_kformat.formatDuration(m_duration * 1000);
}

QString Enclosure::formattedLeftDuration() const
{
    qreal rate = 1.0;
    if (SettingsManager::self()->adjustTimeLeft()) {
        rate = AudioManager::instance().playbackRate();
        rate = (rate > 0.0) ? rate : 1.0;
    }
    qint64 diff = duration() * 1000 - playPosition();
    return m_kformat.formatDuration(diff / rate);
}

QString Enclosure::formattedPlayPosition() const
{
    return m_kformat.formatDuration(m_playposition);
}
