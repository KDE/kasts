/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "enclosure.h"

#include <QFile>
#include <QNetworkReply>
#include <QSqlQuery>

#include "database.h"
#include "enclosuredownloadjob.h"
#include "entry.h"
#include "fetcher.h"

Enclosure::Enclosure(Entry *entry)
    : QObject(entry)
    , m_entry(entry)
{
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
    m_playposition = query.value(QStringLiteral("playposition")).toInt();

    QFile file(path());
    if(file.size() == m_size) {
        m_status = Downloaded;
    } else {
        if(file.exists()) {
            file.remove();
        }
        m_status = Downloadable;
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
            m_status = Downloaded;
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
        disconnect(this, &Enclosure::cancelDownload, this, nullptr);
    });

    connect(downloadJob, &KJob::percentChanged, this, [=](KJob*, unsigned long percent) {
        m_downloadProgress = percent;
        Q_EMIT downloadProgressChanged();
    });

    m_status = Downloading;
    Q_EMIT statusChanged();
}

void Enclosure::deleteFile()
{
    qDebug() << "Trying to delete enclosure file" << path();
    // First check if file still exists; you never know what has happened
    if (QFile(path()).exists())
        QFile(path()).remove();
    // If file disappeared unexpectedly, then still change status to downloadable
    m_status = Downloadable;
    Q_EMIT statusChanged();

}

QString Enclosure::path() const
{
    return Fetcher::instance().enclosurePath(m_url);
}
