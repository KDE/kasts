/**
 * SPDX-FileCopyrightText: 2021-2022 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "fetchfeedsjob.h"

#include <QCryptographicHash>
#include <QObject>
#include <QSqlQuery>
#include <QTimer>

#include <KLocalizedString>
#include <ThreadWeaver/Queue>
#include <ThreadWeaver/ThreadWeaver>
#include <utility>

#include "database.h"
#include "datamanager.h"
#include "fetcher.h"
#include "settingsmanager.h"
#include "updatefeedjob.h"
#include "updaterlogging.h"

using namespace ThreadWeaver;

// TODO: refactor to feeduids
FetchFeedsJob::FetchFeedsJob(const QStringList &urls, QObject *parent)
    : KJob(parent)
    , m_urls(urls)
{
    connect(this, &FetchFeedsJob::processedAmountChanged, this, &FetchFeedsJob::monitorProgress);
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

    QHash<QString, QList<qint64>> feeduidsPerHost;

    // First get the last hashes from the database to know which feeds have actually been updated
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT feeduid FROM Feeds WHERE url=:url;"));
    for (const QString &url : std::as_const(m_urls)) {
        query.bindValue(QStringLiteral(":url"), url);
        if (!Database::instance().execute(query)) {
            return;
        }
        if (query.next()) {
            qint64 feeduid = query.value(QStringLiteral("feeduid")).toLongLong();
            m_feeduids += feeduid;
            feeduidsPerHost[QUrl(url).host()] += feeduid;
        }
    }
    query.finish(); // release lock on database

    qCDebug(kastsUpdater) << "list of feeduids to fetch" << m_feeduids;
    qCDebug(kastsUpdater) << "list of feeduids per host" << feeduidsPerHost;

    setTotalAmount(KJob::Unit::Items, m_feeduids.count());
    setProcessedAmount(KJob::Unit::Items, 0);

    qCDebug(kastsUpdater) << "Number of feed update threads:" << Queue::instance()->currentNumberOfThreads();

    // we make sure to download and process feeds from the same host in sequence
    // because otherwise there is a chance that we get temporarily banned for
    // too many simultaneous downloads
    for (const QList<qint64> &feeduids : std::as_const(feeduidsPerHost)) {
        ThreadWeaver::Sequence *s = new ThreadWeaver::Sequence;
        for (const qint64 feeduid : std::as_const(feeduids)) {
            qCDebug(kastsUpdater) << "Starting to fetch" << feeduid;
            Q_EMIT Fetcher::instance().feedUpdateStatusChanged(feeduid, true);

            UpdateFeedJob *updateFeedJob = new UpdateFeedJob(feeduid, this);
            connect(this, &FetchFeedsJob::aborting, updateFeedJob, &UpdateFeedJob::abort);
            connect(updateFeedJob, &UpdateFeedJob::finished, this, [this, feeduid]() {
                // TODO: add error processing
                setProcessedAmount(KJob::Unit::Items, processedAmount(KJob::Unit::Items) + 1);
                Q_EMIT Fetcher::instance().feedUpdateStatusChanged(feeduid, false);
            });

            *s << updateFeedJob;
            qCDebug(kastsUpdater) << "Enqueued updateFeedJob for feed" << feeduid;
        }
        Queue::instance()->stream() << s;
    }
    qCDebug(kastsUpdater) << "End of FetchFeedsJob::fetch";
}

void FetchFeedsJob::monitorProgress()
{
    // Check if all required feeds have finished updating
    if (processedAmount(KJob::Unit::Items) == totalAmount(KJob::Unit::Items)) {
        // TODO: this should actually be done after syncing has finished...

        // Check for "new" entries and queue them if necessary
        if (SettingsManager::self()->autoQueue()) {
            QList<qint64> entryuids;
            QSqlQuery query;
            query.prepare(QStringLiteral("SELECT entryuid FROM Entries WHERE new=:new ORDER BY updated ASC;"));
            query.bindValue(QStringLiteral(":new"), true);
            Database::instance().execute(query);
            while (query.next()) {
                entryuids += query.value(QStringLiteral("entryuid")).toLongLong();
            }

            qCDebug(kastsUpdater) << "Queueing new entries:" << entryuids;
            DataManager::instance().bulkQueueStatus(true, entryuids);

            if (SettingsManager::self()->autoDownload()) {
                qCDebug(kastsUpdater) << "Start downloading queued entries:" << entryuids;
                DataManager::instance().bulkDownloadEnclosures(entryuids);
            }
        }

        emitResult();
    }
}

bool FetchFeedsJob::aborted()
{
    return m_abort;
}

void FetchFeedsJob::abort()
{
    qCDebug(kastsUpdater) << "Fetching aborted";
    m_abort = true;
    Q_EMIT aborting();
}
