/**
 * SPDX-FileCopyrightText: 2021-2022 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "datamanager.h"
#include "datamanagerlogging.h"

#include <QDateTime>
#include <QDir>
#include <QHash>
#include <QList>
#include <QSqlDatabase>
#include <QSqlError>
#include <QStandardPaths>
#include <QUrl>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <qassert.h>
#include <qhashfunctions.h>
#include <utility>

#include "database.h"
#include "entry.h"
#include "feed.h"
#include "fetcher.h"
#include "models/episodemodel.h"
#include "queuemodel.h"
#include "settingsmanager.h"
#include "sync/sync.h"
#include "utils/storagemanager.h"

DataManager::DataManager()
{
    connect(&Fetcher::instance(),
            &Fetcher::feedDetailsUpdated,
            this,
            [this](const qint64 feeduid,
                   const QString &url,
                   const QString &name,
                   const QString &image,
                   const QString &link,
                   const QString &description,
                   const QDateTime &lastUpdated,
                   const QString &dirname) {
                qCDebug(kastsDataManager) << "Start updating feed details for" << url;
                Feed *feed = getFeed(feeduid);
                if (feed != nullptr) {
                    feed->setName(name);
                    feed->setImage(image);
                    feed->setLink(link);
                    feed->setDescription(description);
                    feed->setLastUpdated(lastUpdated);
                    feed->setDirname(dirname);
                    qCDebug(kastsDataManager) << "Retrieving authors";
                    feed->updateAuthors();
                    // For feeds that have just been added, this is probably the point
                    // where the Feed object gets created; let's set refreshing to
                    // true in order to show user feedback that the feed is still
                    // being fetched
                    feed->setRefreshing(true);
                }
            });
    connect(&Fetcher::instance(), &Fetcher::entryAdded, this, [this](const qint64 entryuid) {
        // Only add the new entry to m_entries
        m_entries[entryuid] = nullptr;
    });
    connect(&Fetcher::instance(), &Fetcher::feedUpdated, this, [this](const qint64 feeduid) {
        Q_EMIT feedEntriesUpdated(feeduid);
    });

    // Only read unique feeduids and entryuids from the database.
    // The feed and entry datastructures will be loaded lazily.
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT feeduid FROM Feeds;"));
    Database::instance().execute(query);
    while (query.next()) {
        m_feeds[query.value(QStringLiteral("feeduid")).toLongLong()] = nullptr;
    }
    query.finish();

    query.prepare(QStringLiteral("SELECT entryuid FROM Entries ORDER BY updated DESC;"));
    Database::instance().execute(query);
    while (query.next()) {
        m_entries[query.value(QStringLiteral("entryuid")).toLongLong()] = nullptr;
    }
    query.finish();
}

Feed *DataManager::getFeed(const qint64 feeduid) const
{
    if (m_feeds.contains(feeduid)) {
        if (m_feeds[feeduid] == nullptr) {
            loadFeed(feeduid);
        }
        return m_feeds[feeduid];
    }
    return nullptr;
}

Feed *DataManager::getFeed(const QString &feedurl) const
{
    return getFeed(getFeeduidFromUrl(feedurl));
}

Entry *DataManager::getEntry(const qint64 entryuid) const
{
    if (m_entries.contains(entryuid)) {
        if (m_entries[entryuid] == nullptr)
            loadEntry(entryuid);
        return m_entries[entryuid];
    }
    return nullptr;
}

Entry *DataManager::getEntry(const QString &id) const
{
    // Apply fuzzy logic to find matching entryuid
    return getEntry(findEntryuids(QStringList({id}))[0][0]);
}

void DataManager::removeFeed(Feed *feed)
{
    QList<Feed *> feeds;
    feeds << feed;
    removeFeeds(feeds);
}

void DataManager::removeFeeds(const QStringList &feedurls)
{
    QList<Feed *> feeds;
    for (const QString &feedurl : feedurls) {
        Feed *feed = getFeed(feedurl);
        if (feed) {
            feeds << feed;
        }
    }
    removeFeeds(feeds);
}

void DataManager::removeFeeds(const QVariantList feedVariantList)
{
    QList<Feed *> feeds;
    for (const QVariant &feedVariant : feedVariantList) {
        if (feedVariant.canConvert<Feed *>()) {
            if (feedVariant.value<Feed *>()) {
                feeds << feedVariant.value<Feed *>();
            }
        }
    }
    removeFeeds(feeds);
}

void DataManager::removeFeeds(const QList<Feed *> &feeds)
{
    for (Feed *feed : feeds) {
        if (feed) {
            const qint16 feeduid = feed->feeduid();

            qCDebug(kastsDataManager) << "deleting feed" << feeduid << "with url" << feeduid;

            // Get list of entries for this feed
            QList<qint64> entries;
            QSqlQuery query;
            query.prepare(QStringLiteral("SELECT entryuid FROM Entries WHERE feeduid=:feeduid;"));
            query.bindValue(QStringLiteral(":feeduid"), feeduid);
            Database::instance().execute(query);
            while (query.next()) {
                entries += query.value(QStringLiteral("entryuid")).toLongLong();
            }
            query.finish();

            // Remove entries from Queue
            bulkQueueStatus(false, entries);

            // Delete entries themselves
            qCDebug(kastsDataManager) << "delete entries of" << feeduid;
            for (auto &entryuid : std::as_const(entries)) {
                if (getEntry(entryuid)->hasEnclosure())
                    getEntry(entryuid)->enclosure()->deleteFile(); // delete enclosure (if it exists)
                if (!getEntry(entryuid)->image().isEmpty())
                    StorageManager::instance().removeImage(getEntry(entryuid)->image()); // delete entry images
                delete m_entries[entryuid]; // delete pointer
                m_entries.remove(entryuid); // delete the hash key
            }

            qCDebug(kastsDataManager) << "Remove feed image" << feed->image() << "for feed" << feeduid;
            qCDebug(kastsDataManager) << "Remove feed enclosure download directory" << feed->dirname() << "for feed" << feeduid;
            QDir enclosureDir = QDir(StorageManager::instance().enclosureDirPath() + feed->dirname());
            if (!feed->dirname().isEmpty() && enclosureDir.exists()) {
                enclosureDir.removeRecursively();
            }
            if (!feed->image().isEmpty())
                StorageManager::instance().removeImage(feed->image());
            m_feeds.remove(feeduid); // remove from m_feeds
            delete feed; // remove the pointer

            // Then delete everything from the database
            qCDebug(kastsDataManager) << "delete database part of" << feeduid;

            Database::instance().transaction();
            // Delete related Errors
            query.prepare(QStringLiteral("DELETE FROM Errors WHERE url IN (SELECT url FROM Feeds WHERE feeduid=:feeduid);"));
            query.bindValue(QStringLiteral(":feeduid"), feeduid);
            Database::instance().execute(query);

            // Delete FeedAuthors
            query.prepare(QStringLiteral("DELETE FROM FeedAuthors WHERE feeduid=:feeduid;"));
            query.bindValue(QStringLiteral(":feeduid"), feeduid);
            Database::instance().execute(query);

            // Delete EntryAuthors
            query.prepare(QStringLiteral("DELETE FROM EntryAuthors WHERE entryuid IN (SELECT entryuid FROM Entries WHERE feeduid=:feeduid);"));
            query.bindValue(QStringLiteral(":feeduid"), feeduid);
            Database::instance().execute(query);

            // Delete Chapters
            query.prepare(QStringLiteral("DELETE FROM Chapters WHERE entryuid IN (SELECT entryuid FROM Entries WHERE feeduid=:feeduid);"));
            query.bindValue(QStringLiteral(":feeduid"), feeduid);
            Database::instance().execute(query);

            // Delete Enclosures
            query.prepare(QStringLiteral("DELETE FROM Enclosures WHERE entryuid IN (SELECT entryuid FROM Entries WHERE feeduid=:feeduid);"));
            query.bindValue(QStringLiteral(":feeduid"), feeduid);
            Database::instance().execute(query);

            // Delete EpisodeActions
            query.prepare(QStringLiteral("DELETE FROM EpisodeActions WHERE id IN (SELECT id FROM Entries WHERE feeduid=:feeduid);"));
            query.bindValue(QStringLiteral(":feeduid"), feeduid);
            Database::instance().execute(query);

            // Delete Entries
            query.prepare(QStringLiteral("DELETE FROM Entries WHERE feeduid=:feeduid;"));
            query.bindValue(QStringLiteral(":feeduid"), feeduid);
            Database::instance().execute(query);

            // Delete Feed
            query.prepare(QStringLiteral("DELETE FROM Feeds WHERE feeduid=:feeduid;"));
            query.bindValue(QStringLiteral(":feeduid"), feeduid);
            Database::instance().execute(query);
            Database::instance().commit();

            // Save this action to the database (including timestamp) in order to be
            // able to sync with remote services
            Sync::instance().storeRemoveFeedAction(feeduid);

            Q_EMIT feedRemoved(feeduid);
        }
    }

    // if settings allow, then upload these changes immediately to sync server
    Sync::instance().doQuickSync();
}

void DataManager::addFeed(const QString &url)
{
    addFeeds(QStringList(url), true);
}

void DataManager::addFeeds(const QStringList &urls, const bool fetch)
{
    // First check if the URLs are not empty
    // TODO: Add more checks like checking if URLs exist; however this will mean async...
    QStringList newUrls;
    for (const QString &url : urls) {
        if (!url.trimmed().isEmpty() && !feedExists(url)) {
            qCDebug(kastsDataManager) << "Feed already exists or URL is empty" << url.trimmed();
            newUrls << QUrl::fromUserInput(url.trimmed()).toString();
        }
    }

    if (newUrls.count() == 0)
        return;

    // This method will add the relevant internal data structures, and then add
    // a preliminary entry into the database.  Those details (as well as entries,
    // authors and enclosures) will be updated by calling Fetcher::fetch() which
    // will trigger a full update of the feed and all related items.
    for (const QString &url : std::as_const(newUrls)) {
        qint64 feeduid = 0;
        qCDebug(kastsDataManager) << "Adding new feed:" << url;

        Database::instance().transaction();
        QSqlQuery query;
        query.prepare(
            QStringLiteral("INSERT INTO Feeds (name, url, image, link, description, subscribed, lastUpdated, new, dirname, lastHash, filterType, sortType) "
                           "VALUES (:name, :url, :image, :link, :description, :subscribed, :lastUpdated, :new, :dirname, :lastHash, :filterType, :sortType);"));
        query.bindValue(QStringLiteral(":name"), url);
        query.bindValue(QStringLiteral(":url"), url);
        query.bindValue(QStringLiteral(":image"), QLatin1String(""));
        query.bindValue(QStringLiteral(":link"), QLatin1String(""));
        query.bindValue(QStringLiteral(":description"), QLatin1String(""));
        query.bindValue(QStringLiteral(":subscribed"), QDateTime::currentSecsSinceEpoch());
        query.bindValue(QStringLiteral(":lastUpdated"), 0);
        query.bindValue(QStringLiteral(":new"), true);
        query.bindValue(QStringLiteral(":dirname"), QLatin1String(""));
        query.bindValue(QStringLiteral(":lastHash"), QLatin1String(""));
        query.bindValue(QStringLiteral(":filterType"), 0);
        query.bindValue(QStringLiteral(":sortType"), 0);
        if (Database::instance().execute(query)) {
            QVariant lastId = query.lastInsertId();
            if (lastId.isValid()) {
                feeduid = lastId.toInt();
            } else {
                qCDebug(kastsDataManager) << "new feed did not get a valid feeduid" << url << feeduid;
                return;
            }
        }
        Database::instance().commit();

        // TODO: check whether the entry in the database happened correctly?

        m_feeds[feeduid] = new Feed(feeduid);

        // Save this action to the database (including timestamp) in order to be
        // able to sync with remote services
        Sync::instance().storeAddFeedAction(url);

        Q_EMIT feedAdded(feeduid);
    }

    if (fetch) {
        Fetcher::instance().fetch(newUrls);
    }

    // if settings allow, upload these changes immediately to sync servers
    Sync::instance().doQuickSync();
}

qint64 DataManager::lastPlayingEntry()
{
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT entryuid FROM Queue WHERE playing=:playing;"));
    query.bindValue(QStringLiteral(":playing"), true);
    Database::instance().execute(query);
    if (!query.next())
        return 0;
    return query.value(QStringLiteral("entryuid")).toLongLong();
}

void DataManager::setLastPlayingEntry(const qint64 entryuid)
{
    Database::instance().transaction();
    QSqlQuery query;
    // First set playing to false for all Queue items
    query.prepare(QStringLiteral("UPDATE Queue SET playing=:playing;"));
    query.bindValue(QStringLiteral(":playing"), false);
    Database::instance().execute(query);
    // Now set the correct track to playing=true
    query.prepare(QStringLiteral("UPDATE Queue SET playing=:playing WHERE entryuid=:entryuid;"));
    query.bindValue(QStringLiteral(":playing"), true);
    query.bindValue(QStringLiteral(":entryuid"), entryuid);
    Database::instance().execute(query);
    Database::instance().commit();
}

void DataManager::deletePlayedEnclosures()
{
    QSqlQuery query;
    query.prepare(
        QStringLiteral("SELECT * FROM Entries INNER JOIN Enclosures ON Entries.entryuid = Enclosures.entryuid WHERE"
                       "(downloaded=:downloaded OR downloaded=:partiallydownloaded) AND (read=:read);"));
    query.bindValue(QStringLiteral(":downloaded"), Enclosure::statusToDb(Enclosure::Downloaded));
    query.bindValue(QStringLiteral(":partiallydownloaded"), Enclosure::statusToDb(Enclosure::PartiallyDownloaded));
    query.bindValue(QStringLiteral(":read"), true);
    Database::instance().execute(query);
    while (query.next()) {
        const qint64 entryuid = query.value(QStringLiteral("entryuid")).toLongLong();
        qCDebug(kastsDataManager) << "Found entry which has been downloaded and is marked as played; deleting now:" << entryuid;
        Entry *entry = getEntry(entryuid);
        if (entry->hasEnclosure()) {
            entry->enclosure()->deleteFile();
        }
    }
}

void DataManager::importFeeds(const QString &path)
{
    QUrl url(path);
    QFile file(url.isLocalFile() ? url.toLocalFile() : url.toString());

    if (file.open(QIODevice::ReadOnly)) {
        QStringList urls;
        QXmlStreamReader xmlReader(&file);
        while (!xmlReader.atEnd()) {
            xmlReader.readNext();
            if (xmlReader.tokenType() == 4 && xmlReader.attributes().hasAttribute(QStringLiteral("xmlUrl"))) {
                urls += xmlReader.attributes().value(QStringLiteral("xmlUrl")).toString();
            }
        }
        qCDebug(kastsDataManager) << "Start importing urls:" << urls;
        addFeeds(urls, true);
    }
    // TODO: Report error when file cannot be opened
}

void DataManager::exportFeeds(const QString &path)
{
    QUrl url(path);
    QFile file(url.isLocalFile() ? url.toLocalFile() : url.toString());
    if (file.open(QIODevice::WriteOnly)) {
        QXmlStreamWriter xmlWriter(&file);
        xmlWriter.setAutoFormatting(true);
        xmlWriter.writeStartDocument(QStringLiteral("1.0"));
        xmlWriter.writeStartElement(QStringLiteral("opml"));
        xmlWriter.writeAttribute(QStringLiteral("version"), QStringLiteral("1.0"));
        xmlWriter.writeEmptyElement(QStringLiteral("head"));
        xmlWriter.writeStartElement(QStringLiteral("body"));
        QSqlQuery query;
        query.prepare(QStringLiteral("SELECT url, name FROM Feeds;"));
        Database::instance().execute(query);
        while (query.next()) {
            xmlWriter.writeEmptyElement(QStringLiteral("outline"));
            xmlWriter.writeAttribute(QStringLiteral("xmlUrl"), query.value(0).toString());
            xmlWriter.writeAttribute(QStringLiteral("title"), query.value(1).toString());
        }
        xmlWriter.writeEndElement();
        xmlWriter.writeEndElement();
        xmlWriter.writeEndDocument();
    }
    // TODO: Report error when file could not be opened
}

void DataManager::loadFeed(const qint64 feeduid) const
{
    if (m_feeds[feeduid]) {
        // nothing to do if Feed object already exists
        return;
    }

    m_feeds[feeduid] = new Feed(feeduid);
}

void DataManager::loadEntry(const qint64 entryuid) const
{
    if (m_entries[entryuid]) {
        // nothing to do if Entry object already exists
        return;
    }

    m_entries[entryuid] = new Entry(entryuid);
}

bool DataManager::feedExists(const QString &url)
{
    // using cleanUrl to do "fuzzy" check on the podcast URL
    QString cleanedUrl = cleanUrl(url);
    QStringList urls;

    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT url FROM Feeds;"));
    Database::instance().execute(query);
    while (query.next()) {
        urls += cleanUrl(query.value(QStringLiteral("url")).toString());
    }

    if (urls.contains(cleanedUrl)) {
        return true;
    }
    return false;
}

void DataManager::bulkMarkReadByIndex(bool state, const QModelIndexList &list) const
{
    bulkMarkRead(state, getEntryuidsFromModelIndexList(list));
}

void DataManager::bulkMarkRead(bool state, const QList<qint64> &entryuids) const
{
    QSet<qint64> feeduids;

    Database::instance().transaction();
    QSqlQuery query;
    query.prepare(QStringLiteral("UPDATE Entries SET read=:read WHERE entryuid=:entryuid;"));
    for (const qint64 &entryuid : std::as_const(entryuids)) {
        query.bindValue(QStringLiteral(":entryuid"), entryuid);
        query.bindValue(QStringLiteral(":read"), state);
        Database::instance().execute(query);
    }
    Database::instance().commit();

    query.prepare(QStringLiteral("SELECT feeduid FROM Entries WHERE entryuid=:entryuid;"));
    for (const qint64 &entryuid : std::as_const(entryuids)) {
        query.bindValue(QStringLiteral(":entryuid"), entryuid);
        Database::instance().execute(query);
        while (query.next()) {
            feeduids += query.value(QStringLiteral("feeduid")).toLongLong();
        }
    }

    // Emit the signals to also update instantiated entry/enclosure/feed objects
    Q_EMIT entryReadStatusChanged(state, entryuids);
    if (state && SettingsManager::self()->resetPositionOnPlayed()) {
        bulkSetPlayPositions(QList<qint64>(entryuids.count(), 0), entryuids);
    }
    for (const qint64 &feeduid : std::as_const(feeduids)) {
        Q_EMIT unreadEntryCountChanged(feeduid);
    }

    // Follow-up actions in case entry is marked as read
    if (state) {
        // 1) Remove item from queue (will also automatically 2) unset the new status)
        bulkQueueStatus(false, entryuids);

        // 5) Delete episode if that setting is set
        if (SettingsManager::self()->autoDeleteOnPlayed() == 1) {
            bulkDeleteEnclosures(entryuids);
        }

        // 6) Log a sync action to sync this state with (gpodder) server
        Sync::instance().storePlayedEpisodeActions(entryuids);
    }

    // if settings allow, upload these changes immediately to sync servers
    if (state) {
        Sync::instance().doQuickSync();
    }
}

void DataManager::bulkMarkNewByIndex(bool state, const QModelIndexList &list) const
{
    bulkMarkNew(state, getEntryuidsFromModelIndexList(list));
}

void DataManager::bulkMarkNew(bool state, const QList<qint64> &entryuids) const
{
    QSet<qint64> feeduids;

    Database::instance().transaction();
    QSqlQuery query;
    query.prepare(QStringLiteral("UPDATE Entries SET new=:new WHERE entryuid=:entryuid;"));
    for (const qint64 &entryuid : std::as_const(entryuids)) {
        query.bindValue(QStringLiteral(":entryuid"), entryuid);
        query.bindValue(QStringLiteral(":new"), state);
        Database::instance().execute(query);
    }
    Database::instance().commit();

    query.prepare(QStringLiteral("SELECT feeduid FROM Entries WHERE entryuid=:entryuid;"));
    for (const qint64 &entryuid : std::as_const(entryuids)) {
        query.bindValue(QStringLiteral(":entryuid"), entryuid);
        Database::instance().execute(query);
        while (query.next()) {
            feeduids += query.value(QStringLiteral("feeduid")).toLongLong();
        }
    }

    Q_EMIT entryNewStatusChanged(state, entryuids);
    for (const qint64 &feeduid : std::as_const(feeduids)) {
        Q_EMIT newEntryCountChanged(feeduid);
    }
}

void DataManager::bulkMarkFavoriteByIndex(bool state, const QModelIndexList &list) const
{
    bulkMarkFavorite(state, getEntryuidsFromModelIndexList(list));
}

void DataManager::bulkMarkFavorite(bool state, const QList<qint64> &entryuids) const
{
    QSet<qint64> feeduids;

    Database::instance().transaction();
    QSqlQuery query;
    query.prepare(QStringLiteral("UPDATE Entries SET favorite=:favorite WHERE entryuid=:entryuid;"));
    for (const qint64 &entryuid : std::as_const(entryuids)) {
        query.bindValue(QStringLiteral(":entryuid"), entryuid);
        query.bindValue(QStringLiteral(":favorite"), state);
        Database::instance().execute(query);
    }
    Database::instance().commit();

    query.prepare(QStringLiteral("SELECT feeduid FROM Entries WHERE entryuid=:entryuid;"));
    for (const qint64 &entryuid : std::as_const(entryuids)) {
        query.bindValue(QStringLiteral(":entryuid"), entryuid);
        Database::instance().execute(query);
        while (query.next()) {
            feeduids += query.value(QStringLiteral("feeduid")).toLongLong();
        }
    }

    Q_EMIT entryFavoriteStatusChanged(state, entryuids);
    for (const qint64 &feeduid : std::as_const(feeduids)) {
        Q_EMIT favoriteEntryCountChanged(feeduid);
    }
}

void DataManager::bulkQueueStatusByIndex(bool state, const QModelIndexList &list) const
{
    bulkQueueStatus(state, getEntryuidsFromModelIndexList(list));
}

void DataManager::bulkQueueStatus(bool state, const QList<qint64> &entryuids) const
{
    if (state) { // i.e. add to queue
        QueueModel::instance().addToQueue(entryuids);

        // Follow-up action: set entry to unread
        bulkMarkRead(false, entryuids);

    } else { // i.e. remove from queue
        QueueModel::instance().removeFromQueue(entryuids);

        // Unset "new" state
        bulkMarkNew(false, entryuids);
    }

    Q_EMIT entryQueueStatusChanged(state, entryuids);
}

void DataManager::bulkDownloadEnclosuresByIndex(const QModelIndexList &list) const
{
    bulkDownloadEnclosures(getEntryuidsFromModelIndexList(list));
}

void DataManager::bulkDownloadEnclosures(const QList<qint64> &entryuids) const
{
    // TODO: move away from instantiation of entries
    bulkQueueStatus(true, entryuids);
    for (const qint64 &entryuid : std::as_const(entryuids)) {
        if (getEntry(entryuid)->hasEnclosure()) {
            getEntry(entryuid)->enclosure()->download();
        }
    }
}

void DataManager::bulkDeleteEnclosuresByIndex(const QModelIndexList &list) const
{
    bulkDeleteEnclosures(getEntryuidsFromModelIndexList(list));
}

void DataManager::bulkDeleteEnclosures(const QList<qint64> &entryuids) const
{
    // TODO: use database directly here?
    for (const qint64 &entryuid : std::as_const(entryuids)) {
        if (getEntry(entryuid)->hasEnclosure()) {
            if (getEntry(entryuid)->enclosure()->status() == Enclosure::Downloading) {
                getEntry(entryuid)->enclosure()->cancelDownload();
            }
            getEntry(entryuid)->enclosure()->deleteFile();
        }
    }
}

void DataManager::bulkSetPlayPositions(const QList<qint64> &playPositions, const QList<qint64> &entryuids) const
{
    Q_ASSERT(playPositions.count() == entryuids.count());

    QSqlQuery query;
    Database::instance().transaction();
    // TODO: switch to saving the position on the entry?
    query.prepare(QStringLiteral("UPDATE Enclosures SET playposition=:playposition WHERE entryuid=:entryuid;"));
    for (qint64 i = 0; i < entryuids.count(); ++i) {
        query.bindValue(QStringLiteral(":entryuid"), entryuids[i]);
        query.bindValue(QStringLiteral(":playposition"), playPositions[i]);
        Database::instance().execute(query);
    }
    Database::instance().commit();

    qCDebug(kastsDataManager) << "Set and saved playpositions for entries:" << entryuids << ", positions:" << playPositions;
    Q_EMIT entryPlayPositionsChanged(playPositions, entryuids);

    // Also store position change to make sure that it can be synced to
    // e.g. gpodder
    Sync::instance().storePlayEpisodeActions(entryuids, playPositions, playPositions);
}

void DataManager::bulkSetEnclosureDurations(const QList<qint64> &durations, const QList<qint64> &entryuids) const
{
    Q_ASSERT(durations.count() == entryuids.count());

    // also save to database
    Database::instance().transaction();
    QSqlQuery query;
    query.prepare(QStringLiteral("UPDATE Enclosures SET duration=:duration WHERE entryuid=:entryuid;"));
    for (qint64 i = 0; i < entryuids.count(); ++i) {
        query.bindValue(QStringLiteral(":entryuid"), entryuids[i]);
        query.bindValue(QStringLiteral(":duration"), durations[i]);
        Database::instance().execute(query);
    }
    Database::instance().commit();

    qCDebug(kastsDataManager) << "Updated entry durations for entries:" << entryuids << ", durations:" << durations;
    Q_EMIT enclosureDurationsChanged(durations, entryuids);
}

void DataManager::bulkSetEnclosureSizes(const QList<qint64> &sizes, const QList<qint64> &entryuids) const
{
    Q_ASSERT(sizes.count() == entryuids.count());

    // also save to database
    Database::instance().transaction();
    QSqlQuery query;
    query.prepare(QStringLiteral("UPDATE Enclosures SET size=:size WHERE entryuid=:entryuid;"));
    for (qint64 i = 0; i < entryuids.count(); ++i) {
        query.bindValue(QStringLiteral(":entryuid"), entryuids[i]);
        query.bindValue(QStringLiteral(":size"), sizes[i]);
        Database::instance().execute(query);
    }
    Database::instance().commit();

    qCDebug(kastsDataManager) << "Updated entry enclosure sizes for entries:" << entryuids << ", durations:" << sizes;
    Q_EMIT enclosureSizesChanged(sizes, entryuids);
}

void DataManager::bulkSetEnclosureStatuses(const QList<Enclosure::Status> &statuses, const QList<qint64> &entryuids) const
{
    Q_ASSERT(statuses.count() == entryuids.count());

    Database::instance().transaction();
    QSqlQuery query;
    query.prepare(QStringLiteral("UPDATE Enclosures SET downloaded=:downloaded WHERE entryuid=:entryuid;"));
    for (qint64 i = 0; i < entryuids.count(); ++i) {
        query.bindValue(QStringLiteral(":entryuid"), entryuids[i]);
        query.bindValue(QStringLiteral(":downloaded"), Enclosure::statusToDb(statuses[i]));
        Database::instance().execute(query);
    }
    Database::instance().commit();

    qCDebug(kastsDataManager) << "Updated entry enclosure statuses for entries:" << entryuids << ", statuses:" << statuses;
    Q_EMIT enclosureStatusesChanged(statuses, entryuids);
}

QList<qint64> DataManager::getEntryuidsFromModelIndexList(const QModelIndexList &list) const
{
    QList<qint64> entryuids;
    for (const QModelIndex &index : std::as_const(list)) {
        entryuids += index.data(EpisodeModel::Roles::EntryuidRole).value<qint64>();
    }
    qCDebug(kastsDataManager) << "Entryuids of selection:" << entryuids;
    return entryuids;
}

QString DataManager::cleanUrl(const QString &url)
{
    // this is a method to create a "canonical" version of a podcast url which
    // would account for some common cases where the URL is different but is
    // actually pointing to the same data.  Currently covering:
    // - http vs https (scheme is actually removed altogether!)
    // - encoded vs non-encoded URLs
    return QUrl(url).authority() + QUrl(url).path(QUrl::FullyDecoded)
        + (QUrl(url).hasQuery() ? QStringLiteral("?") + QUrl(url).query(QUrl::FullyDecoded) : QString());
}

qint64 DataManager::getEntryuidFromId(const QString &id) const
{
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT entryuid FROM Entries WHERE id=:id;"));
    query.bindValue(QStringLiteral(":id"), id);
    Database::instance().execute(query);
    if (!query.next()) {
        return 0;
    }
    return query.value(QStringLiteral("entryuid")).toLongLong();
}

QList<QList<qint64>> DataManager::findEntryuids(const QStringList &ids, const QStringList &enclosureUrls) const
{
    // We try to find the id for the entry based on either the GUID
    // or the episode download URL.  We also try to match with a percent-
    // decoded version of the URL.
    // There can be several hits (e.g. different entries pointing to the
    // same download URL; we add all of them to make sure everything's
    // consistent.
    // This method guarantees to return exactly the same amount of items as the
    // ids list, with the results in the same order. If no match is found, a
    // {0} element is returned for that index in the returned list.
    QList<QList<qint64>> entryuids;
    if (enclosureUrls.count() > 0) {
        Q_ASSERT(ids.count() == enclosureUrls.count());
    }

    // We use two different algorithms depending on how many items we have to
    // look up. For a small amount, doing queries on JOIN tables is very memory
    // efficient. But for larger amounts, this becomes very slow; then we just
    // retrieve all values and do the lookup in c++.
    if (ids.count() < 100) {
        QSqlQuery query;
        if (enclosureUrls.count() > 0) {
            query.prepare(
                QStringLiteral("SELECT Enclosures.entryuid FROM Enclosures JOIN Entries ON Entries.entryuid=Enclosures.entryuid "
                               "WHERE Enclosures.url=:url OR "
                               "Enclosures.url=:decodeurl OR Entries.id=:id;"));
            for (qint64 i = 0; i < ids.count(); ++i) {
                QList<qint64> foundEntryuids;
                query.bindValue(QStringLiteral(":url"), enclosureUrls[i]);
                query.bindValue(QStringLiteral(":decodeurl"), QUrl::fromPercentEncoding(enclosureUrls[i].toUtf8()));
                query.bindValue(QStringLiteral(":id"), ids[i]);
                Database::instance().execute(query);
                if (!query.next()) {
                    qCDebug(kastsDataManager) << "cannot find episode with id:" << ids[i];
                    entryuids += QList<qint64>({0});
                    continue;
                }
                do {
                    foundEntryuids += query.value(QStringLiteral("Enclosures.entryuid")).toLongLong();
                } while (query.next());
                entryuids += foundEntryuids;
            }
        } else {
            query.prepare(QStringLiteral("SELECT entryuid FROM Entries WHERE id=:id;"));
            for (qint64 i = 0; i < ids.count(); ++i) {
                QList<qint64> foundEntryuids;
                query.bindValue(QStringLiteral(":id"), ids[i]);
                Database::instance().execute(query);
                if (!query.next()) {
                    qCDebug(kastsDataManager) << "cannot find episode with id:" << ids[i];
                    entryuids += QList<qint64>({0});
                    continue;
                }
                do {
                    foundEntryuids += query.value(QStringLiteral("entryuid")).toLongLong();
                } while (query.next());
                entryuids += foundEntryuids;
            }
        }
    } else { // ids.count() is large
        QList<qint64> db_entryuids;
        QStringList db_urls, db_ids;
        QSqlQuery query;
        query.prepare(
            QStringLiteral("SELECT Enclosures.entryuid, Enclosures.url, Entries.id FROM Enclosures JOIN Entries ON Entries.entryuid=Enclosures.entryuid;"));
        Database::instance().execute(query);
        while (query.next()) {
            db_entryuids += query.value(QStringLiteral("Enclosures.entryuid")).toLongLong();
            db_urls += query.value(QStringLiteral("Enclosures.url")).toString();
            db_ids += query.value(QStringLiteral("Entries.id")).toString();
        }
        Q_ASSERT(db_entryuids.count() == db_ids.count());
        Q_ASSERT(db_entryuids.count() == db_urls.count());

        for (qint64 i = 0; i < ids.count(); ++i) {
            QSet<qint64> matched_indices;
            matched_indices += db_ids.indexOf(ids[i]);
            if (enclosureUrls.count() > 0) {
                matched_indices += db_urls.indexOf(enclosureUrls[i]);
                matched_indices += db_urls.indexOf(QUrl::fromPercentEncoding(enclosureUrls[i].toUtf8()));
            }
            QList<qint64> foundEntryuids;
            for (const qint64 index : std::as_const(matched_indices)) {
                if (index > -1) {
                    foundEntryuids += db_entryuids[index];
                }
            }
            if (foundEntryuids.count() > 0) {
                entryuids += foundEntryuids;
                qCDebug(kastsDataManager) << "found matches for episode (fast algorithm):" << foundEntryuids;
                qCDebug(kastsDataManager) << "searched for:" << ids[i] << enclosureUrls[i];
            } else {
                entryuids += QList<qint64>({0});
                qCDebug(kastsDataManager) << "cannot find episode with id (fast algorithm):" << ids[i];
            }
        }
    }

    qCDebug(kastsDataManager) << "ids and enclosureUrls to lookup:" << ids << enclosureUrls;
    qCDebug(kastsDataManager) << "found these related entryuids:" << entryuids;
    qCDebug(kastsDataManager) << "number of ids:" << ids.count();
    qCDebug(kastsDataManager) << "number of enclosureUrls:" << enclosureUrls.count();
    qCDebug(kastsDataManager) << "number of entryuids:" << entryuids.count();
    Q_ASSERT(entryuids.count() == ids.count());
    return entryuids;
}

qint64 DataManager::getFeeduidFromUrl(const QString &url) const
{
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT feeduid FROM Feeds WHERE url=:url;"));
    query.bindValue(QStringLiteral(":url"), url);
    Database::instance().execute(query);
    if (!query.next()) {
        return 0;
    }
    return query.value(QStringLiteral("feeduid")).toLongLong();
}
