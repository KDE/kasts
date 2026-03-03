/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <tobias.fella@kde.org>
 * SPDX-FileCopyrightText: 2026 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "enclosuredownloadjob.h"

#include <QFile>
#include <QNetworkReply>
#include <QTimer>

#include <KLocalizedString>

#include "fetcherlogging.h"

using namespace ThreadWeaver;

EnclosureDownloadJob::EnclosureDownloadJob(const QString &url, const QString &filename, QObject *parent)
    : QObject(parent)
    , m_url(url)
    , m_filename(filename)
{
}

void EnclosureDownloadJob::run(JobPointer, Thread *)
{
    startDownload();
}

void EnclosureDownloadJob::startDownload()
{
    m_manager = new NetworkAccessManager();

    // Set up the network request
    QNetworkRequest request((QUrl(m_url)));
    request.setTransferTimeout();

    // Open the file for writing
    bool fileOpenSuccess = false;

    QFile *file = new QFile(m_filename);
    if (file->exists() && file->size() > 0) {
        // try to resume download
        int resumedAt = file->size();
        qCDebug(kastsFetcher) << "Resuming download at" << resumedAt << "bytes";
        QByteArray rangeHeaderValue = QByteArray("bytes=") + QByteArray::number(resumedAt) + QByteArray("-");
        request.setRawHeader(QByteArray("Range"), rangeHeaderValue);
        fileOpenSuccess = file->open(QIODevice::WriteOnly | QIODevice::Append);
    } else {
        qCDebug(kastsFetcher) << "Starting new download";
        fileOpenSuccess = file->open(QIODevice::WriteOnly);
    }

    if (!fileOpenSuccess) {
        m_error = 1;
        m_errorString = QStringLiteral("Cannot open file to write download to %1").arg(m_filename);
        Q_EMIT finished();
        return;
    }

    QNetworkReply *m_reply = m_manager->get(request);
    if (!m_reply) {
        m_error = 1;
        m_errorString = QStringLiteral("Cannot construct QNetworkReply to download").arg(m_url);
        Q_EMIT finished();
        return;
    }

    qDebug() << m_reply;
    connect(m_reply, &QNetworkReply::readyRead, this, [m_reply, file]() {
        if (m_reply->isOpen() && file) {
            QByteArray data = m_reply->readAll();
            file->write(data);
        }
    });

    connect(m_reply, &QNetworkReply::finished, this, [this, m_reply, file]() {
        if (m_reply->isOpen() && file) {
            QByteArray data = m_reply->readAll();
            file->write(data);
            file->close();

            m_complete = true;
        }

        // clean up; close file if still open in case something has gone wrong
        if (file) {
            if (file->isOpen()) {
                file->close();
            }
            delete file;
        }
        m_reply->deleteLater();
        m_manager->deleteLater();
        qCDebug(kastsFetcher) << "downloading enclosure finished";
        Q_EMIT finished();
    });

    connect(m_reply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 total) {
        m_processedAmount = received;
        m_totalAmount = total;
        Q_EMIT processedAmountChanged(received, total);
        qCDebug(kastsFetcher) << "downloading enclosure, received:" << received << ", total:" << total;
    });

    connect(m_reply, &QNetworkReply::errorOccurred, this, [this, m_reply](QNetworkReply::NetworkError code) {
        m_error = code;
        m_errorString = m_reply->errorString();
    });
}

void EnclosureDownloadJob::requestAbort()
{
    // This will trigger the reply to finish, which will lead to finish being emitted
    m_reply->abort();
}

int EnclosureDownloadJob::error() const
{
    return m_error;
}

QString EnclosureDownloadJob::errorString() const
{
    return m_errorString;
}

bool EnclosureDownloadJob::isComplete() const
{
    return m_complete;
}
