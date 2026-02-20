/**
 * SPDX-FileCopyrightText: 2021-2022 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "fetchfeedsjob.h"

#include <QCryptographicHash>
#include <QSqlQuery>
#include <QTimer>

#include <KLocalizedString>
#include <QObject>
#include <ThreadWeaver/Queue>

#include "database.h"
#include "datamanager.h"
#include "entry.h"
#include "error.h"
#include "fetcher.h"
#include "models/errorlogmodel.h"
#include "settingsmanager.h"
#include "updatefeedjob.h"
#include "updaterlogging.h"

using namespace ThreadWeaver;

FetchFeedsJob::FetchFeedsJob(const QStringList &urls, QObject *parent)
    : KJob(parent)
    , m_urls(urls)
{
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

    qCDebug(kastsUpdater) << "Number of feed update threads:" << Queue::instance()->currentNumberOfThreads();

    // First get the last hashes from the database to know which feeds have actually been updated
    QSqlQuery query;
    QHash<QString, QString> lastHashes;
    query.prepare(QStringLiteral("SELECT url, lastHash FROM Feeds;"));
    if (!Database::instance().execute(query)) {
        return;
    }
    if (query.next()) {
        lastHashes[query.value(QStringLiteral("url")).toString()] = query.value(QStringLiteral("lastHash")).toString();
    }
    query.finish(); // release lock on database

    for (int i = 0; i < m_urls.count(); i++) {
        QString url = m_urls[i];
        QString lastHash = lastHashes[url];

        qCDebug(kastsUpdater) << "Starting to fetch" << url;
        Q_EMIT Fetcher::instance().feedUpdateStatusChanged(url, true);

        QNetworkRequest request((QUrl(url)));
        request.setTransferTimeout();

        QNetworkReply *reply = Fetcher::instance().get(request);
        connect(this, &FetchFeedsJob::aborting, reply, &QNetworkReply::abort);
        connect(reply, &QNetworkReply::finished, this, [this, reply, i, url, lastHash]() {
            qCDebug(kastsUpdater) << "got networkreply for" << reply;
            if (reply->error()) {
                if (!m_abort) {
                    qCDebug(kastsUpdater) << "Error fetching feed" << reply->errorString();
                    Q_EMIT logError(Error::Type::FeedUpdate, url, QString(), reply->error(), reply->errorString(), QString());
                }
                setProcessedAmount(KJob::Unit::Items, processedAmount(KJob::Unit::Items) + 1);
                Q_EMIT Fetcher::instance().feedUpdateStatusChanged(url, false);
            } else {
                QByteArray data = reply->readAll();

                // check if the feed has been really been updated by checking if
                // the hash is still the same
                QString newHash = QString::fromLatin1(QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex());
                qCDebug(kastsUpdater) << "RSS hashes (old and new)" << url << lastHash << newHash;

                if (newHash == lastHash) {
                    qCDebug(kastsUpdater) << "same RSS feed hash as last time; skipping feed update for" << url;
                    setProcessedAmount(KJob::Unit::Items, processedAmount(KJob::Unit::Items) + 1);
                    Q_EMIT Fetcher::instance().feedUpdateStatusChanged(url, false);
                } else {
                    UpdateFeedJob *updateFeedJob = new UpdateFeedJob(url, data);
                    connect(this, &FetchFeedsJob::aborting, updateFeedJob, &UpdateFeedJob::abort);
                    connect(updateFeedJob, &UpdateFeedJob::finished, this, [this, url]() {
                        setProcessedAmount(KJob::Unit::Items, processedAmount(KJob::Unit::Items) + 1);
                        Q_EMIT Fetcher::instance().feedUpdateStatusChanged(url, false);
                    });

                    Queue::instance()->stream() << updateFeedJob;
                    qCDebug(kastsUpdater) << "Just started updateFeedJob" << i + 1 << "for feed" << url;
                }
            }
            reply->deleteLater();
        });
        qCDebug(kastsUpdater) << "End of retrieveFeed for" << url;
    }
    qCDebug(kastsUpdater) << "End of FetchFeedsJob::fetch";
}

void FetchFeedsJob::monitorProgress()
{
    // Check if all required feeds have finished updating
    if (processedAmount(KJob::Unit::Items) == totalAmount(KJob::Unit::Items)) {
        // Check for "new" entries and queue them if necessary
        if (SettingsManager::self()->autoQueue()) {
            QSqlQuery query;
            query.prepare(QStringLiteral("SELECT entryuid FROM Entries WHERE new=:new ORDER BY updated ASC;"));
            query.bindValue(QStringLiteral(":new"), true);
            Database::instance().execute(query);
            while (query.next()) {
                qint64 entryuid = query.value(QStringLiteral("entryuid")).toLongLong();
                Entry *entry = DataManager::instance().getEntry(entryuid);
                if (entry) {
                    DataManager::instance().bulkQueueStatus(true, QList<qint64>({entryuid}));
                    if (SettingsManager::self()->autoDownload()) {
                        if (entry && entry->hasEnclosure() && entry->enclosure()) {
                            qCDebug(kastsUpdater) << "Start downloading queued entry" << entry->title();
                            entry->enclosure()->download();
                        }
                    }
                }
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
