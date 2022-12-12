/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include <QNetworkReply>
#include <QTimer>

#include <KLocalizedString>

#include "enclosuredownloadjob.h"
#include "fetcher.h"

EnclosureDownloadJob::EnclosureDownloadJob(const QString &url, const QString &filename, const QString &title, const QString &feedurl, QObject *parent)
    : KJob(parent)
    , m_url(url)
    , m_filename(filename)
    , m_title(title)
    , m_feedurl(feedurl)
{
    setCapabilities(Killable);
}

void EnclosureDownloadJob::start()
{
    QTimer::singleShot(0, this, &EnclosureDownloadJob::startDownload);
}

void EnclosureDownloadJob::startDownload()
{
    m_reply = Fetcher::instance().download(m_url, m_filename, m_feedurl);

    Q_EMIT description(this, i18n("Downloading %1", m_title));

    connect(m_reply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 total) {
        setProcessedAmount(Bytes, received);
        setTotalAmount(Bytes, total);
    });

    connect(m_reply, &QNetworkReply::finished, this, [this]() {
        emitResult();
    });

    connect(m_reply, &QNetworkReply::errorOccurred, this, [this](QNetworkReply::NetworkError code) {
        setError(code);
        setErrorText(m_reply->errorString());
    });
}

bool EnclosureDownloadJob::doKill()
{
    m_reply->abort();
    return true;
}
