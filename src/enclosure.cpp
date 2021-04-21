/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "enclosure.h"

#include <QFile>
#include <QNetworkReply>
#include <QSqlQuery>

#include "database.h"
#include "datamanager.h"
#include "enclosuredownloadjob.h"
#include "entry.h"
#include "fetcher.h"
#include "downloadprogressmodel.h"

Enclosure::Enclosure(Entry *entry)
    : QObject(entry)
    , m_entry(entry)
{
    // Set up signals to make DownloadProgressModel aware of ongoing downloads
    connect(this, &Enclosure::statusChanged, this, [this]() {
        Q_EMIT downloadStatusChanged(m_entry, m_status);
    });
    connect(this, &Enclosure::downloadStatusChanged, &DownloadProgressModel::instance(), &DownloadProgressModel::monitorDownloadProgress);

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
    m_status = query.value(QStringLiteral("downloaded")).toBool() ? Downloaded : Downloadable;
    m_playposition_dbsave = m_playposition;

    // In principle the database contains this status, we check anyway in case
    // something changed on disk
    QFile file(path());
    if (file.exists()) {
        if(file.size() == m_size) {
            if (m_status == Downloadable) {
                // file is on disk, but was not expected, write to database
                // this should never happen
                m_status = Downloaded;
                query.prepare(QStringLiteral("UPDATE Enclosures SET downloaded=:downloaded WHERE id=:id;"));
                query.bindValue(QStringLiteral(":id"), entry->id());
                query.bindValue(QStringLiteral(":downloaded"), true);
                Database::instance().execute(query);
            }
        } else {
            if (m_status == Downloaded) {
                // file was downloaded, but there is a size mismatch
                // delete file and update status in database
                file.remove();
                m_status = Downloadable;
                query.prepare(QStringLiteral("UPDATE Enclosures SET downloaded=:downloaded WHERE id=:id;"));
                query.bindValue(QStringLiteral(":id"), entry->id());
                query.bindValue(QStringLiteral(":downloaded"), false);
                Database::instance().execute(query);
            }
        }
    } else {
        if (m_status == Downloaded) {
            // file was supposed to be on disk, but isn't there
            // update status and write to the database
            file.remove();
            m_status = Downloadable;
            query.prepare(QStringLiteral("UPDATE Enclosures SET downloaded=:downloaded WHERE id=:id;"));
            query.bindValue(QStringLiteral(":id"), entry->id());
            query.bindValue(QStringLiteral(":downloaded"), false);
            Database::instance().execute(query);
        }
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
        if(downloadJob->error() == 0) {
            processDownloadedFile();
        } else {
            m_status = Downloadable;
            if(downloadJob->error() != QNetworkReply::OperationCanceledError) {
                m_entry->feed()->setErrorId(downloadJob->error());
                m_entry->feed()->setErrorString(downloadJob->errorString());
            }
        }
        disconnect(this, &Enclosure::cancelDownload, this, nullptr);
        Q_EMIT statusChanged();
    });

    connect(this, &Enclosure::cancelDownload, this, [this, downloadJob]() {
        downloadJob->doKill();
        m_status = Downloadable;
        Q_EMIT statusChanged();
        Q_EMIT DataManager::instance().downloadCountChanged(m_entry->feed()->url());
        disconnect(this, &Enclosure::cancelDownload, this, nullptr);
    });

    connect(downloadJob, &KJob::percentChanged, this, [=](KJob*, unsigned long percent) {
        m_downloadProgress = percent;
        Q_EMIT downloadProgressChanged();
    });

    m_status = Downloading;
    Q_EMIT statusChanged();
}

void Enclosure::processDownloadedFile() {
    // This will be run if the enclosure has been downloaded successfully
    m_status = Downloaded;
    QSqlQuery query;
    query.prepare(QStringLiteral("UPDATE Enclosures SET downloaded=:downloaded WHERE id=:id;"));
    query.bindValue(QStringLiteral(":id"), m_entry->id());
    query.bindValue(QStringLiteral(":downloaded"), true);
    Database::instance().execute(query);
    // Unset "new" status of item
    if (m_entry->getNew()) m_entry->setNew(false);

    // Check if reported filesize in rss feed corresponds to real file size
    // if not, correct the filesize in the database
    // otherwise the file will get deleted because of mismatch in signature
    QFile file(path());
    if(file.size() != m_size) {
        qDebug() << "enclosure file size mismatch" << m_entry->title();
        setSize(file.size());
    }
    Q_EMIT DataManager::instance().downloadCountChanged(m_entry->feed()->url());

}

void Enclosure::deleteFile()
{
    qDebug() << "Trying to delete enclosure file" << path();
    // First check if file still exists; you never know what has happened
    if (QFile(path()).exists())
        QFile(path()).remove();
    // If file disappeared unexpectedly, then still change status to downloadable
    m_status = Downloadable;
    QSqlQuery query;
    query.prepare(QStringLiteral("UPDATE Enclosures SET downloaded=:downloaded WHERE id=:id;"));
    query.bindValue(QStringLiteral(":id"), m_entry->id());
    query.bindValue(QStringLiteral(":downloaded"), false);
    Database::instance().execute(query);
    Q_EMIT statusChanged();
    Q_EMIT DataManager::instance().downloadCountChanged(m_entry->feed()->url());
}

QString Enclosure::path() const
{
    return Fetcher::instance().enclosurePath(m_url);
}

Enclosure::Status Enclosure::status() const
{
    return m_status;
}

qint64 Enclosure::playPosition() const{
    return m_playposition;
}

qint64 Enclosure::duration() const {
    return m_duration;
}

int Enclosure::size() const {
    return m_size;
}

void Enclosure::setPlayPosition(const qint64 &position)
{
    m_playposition = position;
    qDebug() << "save playPosition" << position << m_entry->title();
    Q_EMIT playPositionChanged();

    // let's only save the play position to the database every 15 seconds
    if ((abs(m_playposition - m_playposition_dbsave) > 15000) || position == 0) {
        qDebug() << "save playPosition to database" << position << m_entry->title();
        QSqlQuery query;
        query.prepare(QStringLiteral("UPDATE Enclosures SET playposition=:playposition WHERE id=:id AND feed=:feed"));
        query.bindValue(QStringLiteral(":id"), m_entry->id());
        query.bindValue(QStringLiteral(":feed"), m_entry->feed()->url());
        query.bindValue(QStringLiteral(":playposition"), m_playposition);
        Database::instance().execute(query);
        m_playposition_dbsave = m_playposition;
    }
}

void Enclosure::setDuration(const qint64 &duration)
{
    m_duration = duration;
    Q_EMIT durationChanged();

    // also save to database
    qDebug() << "updating entry duration" << duration << m_entry->title();
    QSqlQuery query;
    query.prepare(QStringLiteral("UPDATE Enclosures SET duration=:duration WHERE id=:id AND feed=:feed"));
    query.bindValue(QStringLiteral(":id"), m_entry->id());
    query.bindValue(QStringLiteral(":feed"), m_entry->feed()->url());
    query.bindValue(QStringLiteral(":duration"), m_duration);
    Database::instance().execute(query);
}

void Enclosure::setSize(const int &size)
{
    m_size = size;
    Q_EMIT sizeChanged();

    // also save to database
    QSqlQuery query;
    query.prepare(QStringLiteral("UPDATE Enclosures SET size=:size WHERE id=:id AND feed=:feed"));
    query.bindValue(QStringLiteral(":id"), m_entry->id());
    query.bindValue(QStringLiteral(":feed"), m_entry->feed()->url());
    query.bindValue(QStringLiteral(":size"), m_size);
    Database::instance().execute(query);
}
