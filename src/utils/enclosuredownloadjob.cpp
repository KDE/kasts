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

#include "fetcher.h"
#include "objectslogging.h"

EnclosureDownloadJob::EnclosureDownloadJob(const qint64 entryuid, const QString &url, const QString &filename, const QString &title, QObject *parent)
    : KJob(parent)
    , m_entryuid(entryuid)
    , m_url(url)
    , m_filename(filename)
    , m_title(title)
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
    m_status = EnclosureDownloadJob::Status::Downloading;
    Q_EMIT statusChanged(m_status);

    m_reply = getNetworkReply(m_url, m_filename);

    if (!m_reply) {
        setError(1);
        setErrorText(QStringLiteral("Cannot open file to write download to %1").arg(m_filename));
        emitResult();
        return;
    }

    // TODO: do we really need the entry title only for the description which is not realy used otherwise?
    Q_EMIT description(this, i18n("Downloading %1", m_title));

    connect(m_reply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 total) {
        setProcessedAmount(Bytes, received);
        setTotalAmount(Bytes, total);
    });

    connect(m_reply, &QNetworkReply::finished, this, [this]() {
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
