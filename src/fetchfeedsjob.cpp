/**
 * SPDX-FileCopyrightText: 2021-2022 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "fetchfeedsjob.h"

#include <QTimer>

#include <KLocalizedString>
#include <ThreadWeaver/Queue>

#include "error.h"
#include "fetcher.h"
#include "fetcherlogging.h"
#include "models/errorlogmodel.h"
#include "settingsmanager.h"
#include "updatefeedjob.h"

using namespace ThreadWeaver;

FetchFeedsJob::FetchFeedsJob(const QStringList &urls, QObject *parent)
    : KJob(parent)
    , m_urls(urls)
{
    for (int i = 0; i < m_urls.count(); i++) {
        m_feedjobs += nullptr;
    }
    connect(this, &FetchFeedsJob::processedAmountChanged, this, &FetchFeedsJob::monitorProgress);
    connect(this, &FetchFeedsJob::logError, &ErrorLogModel::instance(), &ErrorLogModel::monitorErrorMessages);
}

void FetchFeedsJob::start()
{
    QTimer::singleShot(0, this, &FetchFeedsJob::fetch);
}

void FetchFeedsJob::fetch()
{
    if (m_urls.count() == 0) {
        emitResult();
        return;
    }

    setTotalAmount(KJob::Unit::Items, m_urls.count());
    setProcessedAmount(KJob::Unit::Items, 0);

    qCDebug(kastsFetcher) << "Number of feed update threads:" << Queue::instance()->currentNumberOfThreads();

    for (int i = 0; i < m_urls.count(); i++) {
        QString url = m_urls[i];

        qCDebug(kastsFetcher) << "Starting to fetch" << url;
        Q_EMIT Fetcher::instance().feedUpdateStatusChanged(url, true);

        QNetworkRequest request((QUrl(url)));
        request.setTransferTimeout();

        QNetworkReply *reply = Fetcher::instance().get(request);
        connect(this, &FetchFeedsJob::aborting, reply, &QNetworkReply::abort);
        connect(reply, &QNetworkReply::finished, this, [this, reply, i, url]() {
            qCDebug(kastsFetcher) << "got networkreply for" << reply;
            if (reply->error()) {
                if (!m_abort) {
                    qCDebug(kastsFetcher) << "Error fetching feed" << reply->errorString();
                    Q_EMIT logError(Error::Type::FeedUpdate, url, QString(), reply->error(), reply->errorString(), QString());
                }
                setProcessedAmount(KJob::Unit::Items, processedAmount(KJob::Unit::Items) + 1);
                Q_EMIT Fetcher::instance().feedUpdateStatusChanged(url, false);
            } else {
                QByteArray data = reply->readAll();

                UpdateFeedJob *updateFeedJob = new UpdateFeedJob(url, data);
                m_feedjobs[i] = updateFeedJob;
                connect(this, &FetchFeedsJob::aborting, updateFeedJob, &UpdateFeedJob::abort);
                connect(updateFeedJob, &UpdateFeedJob::finished, this, [this, url]() {
                    setProcessedAmount(KJob::Unit::Items, processedAmount(KJob::Unit::Items) + 1);
                    Q_EMIT Fetcher::instance().feedUpdateStatusChanged(url, false);
                });

                stream() << updateFeedJob;
                qCDebug(kastsFetcher) << "Just started updateFeedJob" << i + 1;
            }
            reply->deleteLater();
        });
        qCDebug(kastsFetcher) << "End of retrieveFeed for" << url;
    }
    qCDebug(kastsFetcher) << "End of FetchFeedsJob::fetch";
}

void FetchFeedsJob::monitorProgress()
{
    if (processedAmount(KJob::Unit::Items) == totalAmount(KJob::Unit::Items)) {
        emitResult();
    }
}

bool FetchFeedsJob::aborted()
{
    return m_abort;
}

void FetchFeedsJob::abort()
{
    qCDebug(kastsFetcher) << "Fetching aborted";
    m_abort = true;
    Q_EMIT aborting();
}
