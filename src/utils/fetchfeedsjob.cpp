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
#include <ThreadWeaver/Queue>

#include "database.h"
#include "datamanager.h"
#include "datatypes.h"
#include "entry.h"
#include "error.h"
#include "fetcher.h"
#include "fetcherlogging.h"
#include "models/errorlogmodel.h"
#include "settingsmanager.h"
#include "updatefeedjob.h"

using namespace ThreadWeaver;
using namespace DataTypes;

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

    qCDebug(kastsFetcher) << "Number of feed update threads:" << Queue::instance()->currentNumberOfThreads();

    for (int i = 0; i < m_urls.count(); i++) {
        const QString url = m_urls[i];

        // First get all the required data from the database and do some basic checks
        FeedDetails oldFeedDetails;
        QSqlQuery query;
        query.prepare(QStringLiteral("SELECT * FROM Feeds WHERE url=:url;"));
        query.bindValue(QStringLiteral(":url"), url);
        if (!dbExecute(query)) {
            continue;
        }
        if (query.next()) {
            DataTypes::FeedDetails feedDetail;
            oldFeedDetails.name = query.value(QStringLiteral("name")).toString();
            oldFeedDetails.url = url;
            oldFeedDetails.image = query.value(QStringLiteral("image")).toString();
            oldFeedDetails.link = query.value(QStringLiteral("link")).toString();
            oldFeedDetails.description = query.value(QStringLiteral("description")).toString();
            oldFeedDetails.deleteAfterCount = query.value(QStringLiteral("deleteAfterCount")).toInt();
            oldFeedDetails.deleteAfterType = query.value(QStringLiteral("deleteAfterType")).toInt();
            oldFeedDetails.subscribed = query.value(QStringLiteral("subscribed")).toInt();
            oldFeedDetails.lastUpdated = query.value(QStringLiteral("lastUpdated")).toInt();
            oldFeedDetails.isNew = query.value(QStringLiteral("new")).toBool();
            oldFeedDetails.notify = query.value(QStringLiteral("notify")).toBool();
            oldFeedDetails.dirname = query.value(QStringLiteral("dirname")).toString();
            oldFeedDetails.lastHash = query.value(QStringLiteral("lastHash")).toString();
            oldFeedDetails.filterType = query.value(QStringLiteral("filterType")).toInt();
            oldFeedDetails.sortType = query.value(QStringLiteral("sortType")).toInt();
            oldFeedDetails.state = DataTypes::RecordState::Unmodified;
        } else {
            continue;
        }
        query.finish(); // release lock on database

        // Now that we have the feed details, we make vectors of the data that's
        // already in the database relating to this feed
        // NOTE: We will do the feed authors after this step, because otherwise
        // we can't check for duplicates and we'll keep adding more of the same!
        query.prepare(QStringLiteral("SELECT * FROM Entries WHERE feed=:feed;"));
        query.bindValue(QStringLiteral(":feed"), url);
        dbExecute(query);
        while (query.next()) {
            EntryDetails entryDetails;
            entryDetails.feed = url;
            entryDetails.id = query.value(QStringLiteral("id")).toString();
            entryDetails.title = query.value(QStringLiteral("title")).toString();
            entryDetails.content = query.value(QStringLiteral("content")).toString();
            entryDetails.created = query.value(QStringLiteral("created")).toInt();
            entryDetails.updated = query.value(QStringLiteral("updated")).toInt();
            entryDetails.read = query.value(QStringLiteral("read")).toBool();
            entryDetails.isNew = query.value(QStringLiteral("new")).toBool();
            entryDetails.link = query.value(QStringLiteral("link")).toString();
            entryDetails.hasEnclosure = query.value(QStringLiteral("hasEnclosure")).toBool();
            entryDetails.image = query.value(QStringLiteral("image")).toString();
            entryDetails.state = DataTypes::RecordState::RemovedFromFeed;
            oldFeedDetails.entries[entryDetails.id] = entryDetails;
        }
        query.finish();

        query.prepare(QStringLiteral("SELECT * FROM Authors WHERE feed=:feed AND id='';"));
        query.bindValue(QStringLiteral(":feed"), url);
        dbExecute(query);
        while (query.next()) {
            AuthorDetails authorDetails;
            authorDetails.name = query.value(QStringLiteral("name")).toString();
            authorDetails.email = query.value(QStringLiteral("email")).toString();
            authorDetails.state = DataTypes::RecordState::RemovedFromFeed;
            oldFeedDetails.authors[authorDetails.name] = authorDetails;
        }
        query.finish();

        for (EntryDetails &entryDetails : oldFeedDetails.entries) {
            query.prepare(QStringLiteral("SELECT * FROM Enclosures WHERE feed=:feed AND id=:id;"));
            query.bindValue(QStringLiteral(":feed"), url);
            query.bindValue(QStringLiteral(":id"), entryDetails.id);
            dbExecute(query);
            while (query.next()) {
                EnclosureDetails enclosureDetails;
                enclosureDetails.duration = query.value(QStringLiteral("duration")).toInt();
                enclosureDetails.size = query.value(QStringLiteral("size")).toInt();
                enclosureDetails.title = query.value(QStringLiteral("title")).toString();
                enclosureDetails.type = query.value(QStringLiteral("type")).toString();
                enclosureDetails.url = query.value(QStringLiteral("url")).toString();
                enclosureDetails.playPosition = query.value(QStringLiteral("id")).toInt();
                enclosureDetails.downloaded = Enclosure::dbToStatus(query.value(QStringLiteral("downloaded")).toInt());
                enclosureDetails.state = DataTypes::RecordState::RemovedFromFeed;
                entryDetails.enclosures[enclosureDetails.url] = enclosureDetails;
            }
            query.finish();

            query.prepare(QStringLiteral("SELECT * FROM Authors WHERE feed=:feed AND id=:id;"));
            query.bindValue(QStringLiteral(":feed"), url);
            query.bindValue(QStringLiteral(":id"), entryDetails.id);
            dbExecute(query);
            while (query.next()) {
                AuthorDetails authorDetails;
                authorDetails.name = query.value(QStringLiteral("name")).toString();
                authorDetails.email = query.value(QStringLiteral("email")).toString();
                authorDetails.state = DataTypes::RecordState::RemovedFromFeed;
                entryDetails.authors[authorDetails.name] = authorDetails;
            }
            query.finish();

            query.prepare(QStringLiteral("SELECT * FROM Chapters WHERE feed=:feed AND id=:id;"));
            query.bindValue(QStringLiteral(":feed"), url);
            query.bindValue(QStringLiteral(":id"), entryDetails.id);
            dbExecute(query);
            while (query.next()) {
                ChapterDetails chapterDetails;
                chapterDetails.start = query.value(QStringLiteral("start")).toInt();
                chapterDetails.title = query.value(QStringLiteral("title")).toString();
                chapterDetails.link = query.value(QStringLiteral("link")).toString();
                chapterDetails.image = query.value(QStringLiteral("image")).toString();
                chapterDetails.state = DataTypes::RecordState::RemovedFromFeed;
                entryDetails.chapters[chapterDetails.start] = chapterDetails;
            }
            query.finish();
        }

        qCDebug(kastsFetcher) << "Starting to fetch" << url;
        Q_EMIT Fetcher::instance().feedUpdateStatusChanged(url, true);

        QNetworkRequest request((QUrl(url)));
        request.setTransferTimeout();

        QNetworkReply *reply = Fetcher::instance().get(request);
        connect(this, &FetchFeedsJob::aborting, reply, &QNetworkReply::abort);
        connect(reply, &QNetworkReply::finished, this, [this, reply, i, url, oldFeedDetails]() {
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

                // check if the feed has been really been updated by checking if
                // the hash is still the same
                QString newHash = QString::fromLatin1(QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex());
                qCDebug(kastsFetcher) << "RSS hashes (old and new)" << url << oldFeedDetails.lastHash << newHash;

                if (newHash == oldFeedDetails.lastHash) {
                    qCDebug(kastsFetcher) << "same RSS feed hash as last time; skipping feed update for" << url;
                    setProcessedAmount(KJob::Unit::Items, processedAmount(KJob::Unit::Items) + 1);
                    Q_EMIT Fetcher::instance().feedUpdateStatusChanged(url, false);
                } else {
                    UpdateFeedJob *updateFeedJob = new UpdateFeedJob(data, oldFeedDetails);
                    connect(this, &FetchFeedsJob::aborting, updateFeedJob, &UpdateFeedJob::abort);
                    connect(updateFeedJob, &UpdateFeedJob::finished, this, [this, url]() {
                        setProcessedAmount(KJob::Unit::Items, processedAmount(KJob::Unit::Items) + 1);
                        Q_EMIT Fetcher::instance().feedUpdateStatusChanged(url, false);
                    });
                    connect(updateFeedJob, &UpdateFeedJob::resultsAvailable, this, [](FeedDetails feedDetails) {
                        qCDebug(kastsFetcher) << "Got results back from UpdateFeedJob" << feedDetails.url;
                    });

                    stream() << updateFeedJob;
                    qCDebug(kastsFetcher) << "Just started updateFeedJob" << i + 1 << "for feed" << url;
                }
            }
            reply->deleteLater();
        });
        qCDebug(kastsFetcher) << "End of retrieveFeed for" << url;
    }
    qCDebug(kastsFetcher) << "End of FetchFeedsJob::fetch";
}

void FetchFeedsJob::monitorProgress()
{
    // Check if all required feeds have finished updating
    if (processedAmount(KJob::Unit::Items) == totalAmount(KJob::Unit::Items)) {
        // Check for "new" entries and queue them if necessary
        if (SettingsManager::self()->autoQueue()) {
            QSqlQuery query;
            Database::instance().transaction();
            query.prepare(QStringLiteral("SELECT id FROM Entries WHERE new=:new ORDER BY updated ASC;"));
            query.bindValue(QStringLiteral(":new"), true);
            Database::instance().execute(query);
            while (query.next()) {
                QString id = query.value(QStringLiteral("id")).toString();
                Entry *entry = DataManager::instance().getEntry(id);
                if (entry) {
                    entry->setQueueStatusInternal(true);
                    if (SettingsManager::self()->autoDownload()) {
                        if (entry && entry->hasEnclosure() && entry->enclosure()) {
                            qCDebug(kastsFetcher) << "Start downloading queued entry" << entry->title();
                            entry->enclosure()->download();
                        }
                    }
                }
            }
            Database::instance().commit();
        }

        emitResult();
    }
}

bool FetchFeedsJob::dbExecute(QSqlQuery &query)
{
    bool state = Database::instance().execute(query);

    if (!state) {
        // TODO: reenable line below by adding an error signal to this class
        // Q_EMIT error(Error::Type::Database, QString(), QString(), query.lastError().type(), query.lastQuery(), query.lastError().text());
    }

    return state;
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
