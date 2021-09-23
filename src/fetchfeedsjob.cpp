/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "fetchfeedsjob.h"

#include <QTimer>

#include <KLocalizedString>

#include "error.h"
#include "fetcher.h"
#include "fetcherlogging.h"
#include "models/errorlogmodel.h"
#include "settingsmanager.h"
#include "updatefeedjob.h"

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
    if (Fetcher::instance().isMeteredConnection() && !SettingsManager::self()->allowMeteredFeedUpdates()) {
        setError(0);
        setErrorText(i18n("Podcast updates not allowed due to user setting"));
        emitResult();
        return;
    }

    setTotalAmount(KJob::Unit::Items, m_urls.count());
    setProcessedAmount(KJob::Unit::Items, 0);

    for (int i = 0; i < m_urls.count(); i++) {
        QString url = m_urls[i];

        UpdateFeedJob *updateFeedJob = new UpdateFeedJob(url, this);
        m_feedjobs[i] = updateFeedJob;
        connect(this, &FetchFeedsJob::abort, updateFeedJob, &UpdateFeedJob::abort);
        connect(updateFeedJob, &UpdateFeedJob::result, this, [this, url, updateFeedJob]() {
            if (updateFeedJob->error()) {
                Q_EMIT logError(Error::Type::FeedUpdate, url, QString(), updateFeedJob->error(), updateFeedJob->errorString(), QString());
            }
            setProcessedAmount(KJob::Unit::Items, processedAmount(KJob::Unit::Items) + 1);
        });
        updateFeedJob->start();
        qCDebug(kastsFetcher) << "Just started updateFeedJob" << i + 1;
    }
    qCDebug(kastsFetcher) << "End of FetchFeedsJob::fetch";
}

void FetchFeedsJob::monitorProgress()
{
    if (processedAmount(KJob::Unit::Items) == totalAmount(KJob::Unit::Items)) {
        emitResult();
    }
}
