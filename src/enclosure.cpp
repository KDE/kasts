/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "enclosure.h"
#include "enclosurelogging.h"

#include <QFile>
#include <QNetworkReply>
#include <QSqlQuery>

#include "database.h"
#include "datamanager.h"
#include "downloadprogressmodel.h"
#include "enclosuredownloadjob.h"
#include "entry.h"
#include "error.h"
#include "errorlogmodel.h"
#include "fetcher.h"

Enclosure::Enclosure(Entry *entry)
    : QObject(entry)
    , m_entry(entry)
{
    connect(this, &Enclosure::statusChanged, &DownloadProgressModel::instance(), &DownloadProgressModel::monitorDownloadProgress);
    connect(this, &Enclosure::downloadError, &ErrorLogModel::instance(), &ErrorLogModel::monitorErrorMessages);
    connect(&Fetcher::instance(), &Fetcher::downloadFileSizeUpdated, this, [this](QString url, int fileSize, int resumedAt) {
        if ((url == m_url) && ((m_size != fileSize) && (m_size != fileSize + resumedAt)) && (fileSize > 1000)) {
            // Sometimes, when resuming a download, the complete file size is
            // reported.  Other times only the remaining part.
            // Sometimes the value is rubbish (e.g. 2)
            // We assume that the value when starting a new download is correct.
            if (fileSize < resumedAt) {
                fileSize += resumedAt;
            }
            qDebug() << "Correct filesize for enclosure" << url << "from" << m_size << "to" << fileSize;
            setSize(fileSize);
        }
    });

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
    } else {
        // file does not exist
        setStatus(Downloadable);
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
    EnclosureDownloadJob *downloadJob = new EnclosureDownloadJob(m_url, path(), m_entry->title());
    downloadJob->start();

    m_downloadProgress = 0;
    Q_EMIT downloadProgressChanged();

    m_entry->feed()->setErrorId(0);
    m_entry->feed()->setErrorString(QString());

    connect(downloadJob, &KJob::result, this, [this, downloadJob]() {
        if (downloadJob->error() == 0) {
            processDownloadedFile();
        } else {
            QFile file(path());
            if (file.exists() && file.size() > 0) {
                setStatus(PartiallyDownloaded);
            } else {
                setStatus(Downloadable);
            }
            if (downloadJob->error() != QNetworkReply::OperationCanceledError) {
                m_entry->feed()->setErrorId(downloadJob->error());
                m_entry->feed()->setErrorString(downloadJob->errorString());
                Q_EMIT downloadError(Error::Type::MediaDownload, m_entry->feed()->url(), m_entry->id(), downloadJob->error(), downloadJob->errorString());
            }
        }
        disconnect(this, &Enclosure::cancelDownload, this, nullptr);
        Q_EMIT statusChanged(m_entry, m_status);
    });

    connect(this, &Enclosure::cancelDownload, this, [this, downloadJob]() {
        downloadJob->doKill();
        QFile file(path());
        if (file.exists() && file.size() > 0) {
            setStatus(PartiallyDownloaded);
        } else {
            setStatus(Downloadable);
        }
        disconnect(this, &Enclosure::cancelDownload, this, nullptr);
    });

    connect(downloadJob, &KJob::percentChanged, this, [=](KJob *, unsigned long percent) {
        m_downloadProgress = percent;
        Q_EMIT downloadProgressChanged();
    });

    setStatus(Downloading);
}

void Enclosure::processDownloadedFile()
{
    // This will be run if the enclosure has been downloaded successfully

    // First check if file size is larger than 0; otherwise something unexpected
    // must have happened
    QFile file(path());
    if (file.size() == 0) {
        deleteFile();
        return;
    }

    // Check if reported filesize in rss feed corresponds to real file size
    // if not, correct the filesize in the database
    // otherwise the file will get deleted because of mismatch in signature
    if (file.size() != m_size) {
        qCDebug(kastsEnclosure) << "enclosure file size mismatch" << m_entry->title() << "from" << m_size << "to" << file.size();
        setSize(file.size());
    }

    setStatus(Downloaded);
    // Unset "new" status of item
    if (m_entry->getNew())
        m_entry->setNew(false);
}

void Enclosure::deleteFile()
{
    qCDebug(kastsEnclosure) << "Trying to delete enclosure file" << path();
    // First check if file still exists; you never know what has happened
    if (QFile(path()).exists())
        QFile(path()).remove();
    // If file disappeared unexpectedly, then still change status to downloadable
    setStatus(Downloadable);
}

QString Enclosure::path() const
{
    return Fetcher::instance().enclosurePath(m_url);
}

Enclosure::Status Enclosure::status() const
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

int Enclosure::size() const
{
    return m_size;
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

void Enclosure::setSize(const int &size)
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

QString Enclosure::formattedSize() const
{
    return m_kformat.formatByteSize(m_size);
}

QString Enclosure::formattedDuration() const
{
    return m_kformat.formatDuration(m_duration * 1000);
}

QString Enclosure::formattedPlayPosition() const
{
    return m_kformat.formatDuration(m_playposition);
}
