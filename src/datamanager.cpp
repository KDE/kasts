/**
 * SPDX-FileCopyrightText: 2021-2022 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "datamanager.h"
#include "datamanagerlogging.h"

#include <QDateTime>
#include <QDir>
#include <QSqlDatabase>
#include <QSqlError>
#include <QStandardPaths>
#include <QUrl>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <qsqlquery.h>
#include <qstringliteral.h>

#include "audiomanager.h"
#include "database.h"
#include "entry.h"
#include "feed.h"
#include "fetcher.h"
#include "models/episodemodel.h"
#include "sync/sync.h"
#include "utils/storagemanager.h"

DataManager::DataManager()
{
    connect(&Fetcher::instance(),
            &Fetcher::feedDetailsUpdated,
            this,
            [this](const int feedid,
                   const QString &url,
                   const QString &name,
                   const QString &image,
                   const QString &link,
                   const QString &description,
                   const QDateTime &lastUpdated,
                   const QString &dirname) {
                qCDebug(kastsDataManager) << "Start updating feed details for" << feedid << url;
                Feed *feed = getFeed(feedid);
                if (feed != nullptr) {
                    feed->setUrl(url);
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
    connect(&Fetcher::instance(), &Fetcher::entryAdded, this, [this](const int &feedid, const int &entryid) {
        Q_UNUSED(feedid)
        // Only add the new entry to m_entries
        // we will repopulate m_entrymap once all new entries have been added,
        // such that m_entrymap will show all new entries in the correct order
        m_entries[entryid] = nullptr;
    });
    connect(&Fetcher::instance(), &Fetcher::feedUpdated, this, [this](const int &feedid) {
        // Update m_entrymap for feedurl, such that the new and old entries show
        // up in the correct order
        // TODO: put this code into a separate method and re-use this in the constructor
        QSqlQuery query;
        m_entrymap[feedid].clear();
        query.prepare(QStringLiteral("SELECT entryid FROM Entries WHERE feedid=:feedid ORDER BY updated DESC;"));
        query.bindValue(QStringLiteral(":feedid"), feedid);
        Database::instance().execute(query);
        while (query.next()) {
            m_entrymap[feedid] += query.value(QStringLiteral("entryid")).toInt();
        }

        Q_EMIT feedEntriesUpdated(feedid);
    });

    // Only read unique feedurls and entry ids from the database.
    // The feed and entry datastructures will be loaded lazily.
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT feedid FROM Feeds;"));
    Database::instance().execute(query);
    while (query.next()) {
        m_feedmap += query.value(QStringLiteral("feedid")).toInt();
        m_feeds[query.value(QStringLiteral("feedid")).toInt()] = nullptr;
    }

    for (auto &feedid : m_feedmap) {
        query.prepare(QStringLiteral("SELECT entryid FROM Entries WHERE feedid=:feedid ORDER BY updated DESC;"));
        query.bindValue(QStringLiteral(":feedid"), feedid);
        Database::instance().execute(query);
        while (query.next()) {
            m_entrymap[feedid] += query.value(QStringLiteral("entryid")).toInt();
            m_entries[query.value(QStringLiteral("entryid")).toInt()] = nullptr;
        }
    }

    // qCDebug(kastsDataManager) << "entrymap contains:" << m_entrymap;

    query.prepare(QStringLiteral("SELECT entryid FROM Queue ORDER BY listnr;"));
    Database::instance().execute(query);
    while (query.next()) {
        m_queuemap += query.value(QStringLiteral("entryid")).toInt();
    }

    qCDebug(kastsDataManager) << "Queuemap contains:" << m_queuemap;
}

Feed *DataManager::getFeedByIndex(const int index) const
{
    if (index >= 0 && index < m_feedmap.size()) {
        return getFeed(m_feedmap[index]);
    }
    return nullptr;
}

Feed *DataManager::getFeed(const int &feedid) const
{
    if (m_feeds.contains(feedid)) {
        if (m_feeds[feedid] == nullptr) {
            loadFeed(feedid);
        }
        return m_feeds[feedid];
    }
    return nullptr;
}

Entry *DataManager::getEntry(const int &entryid) const
{
    if (m_entries.contains(entryid)) {
        if (m_entries[entryid] == nullptr)
            loadEntry(entryid);
        return m_entries[entryid];
    }
    return nullptr;
}

Entry *DataManager::getEntry(const int feed_index, const int entry_index) const
{
    if (feed_index < m_feedmap.size() && entry_index < m_entrymap[m_feedmap[feed_index]].size()) {
        return getEntry(m_entrymap[m_feedmap[feed_index]][entry_index]);
    }
    return nullptr;
}

Entry *DataManager::getEntry(const Feed *feed, const int entry_index) const
{
    if (feed && entry_index < m_entrymap[feed->feedid()].size()) {
        return getEntry(m_entrymap[feed->feedid()][entry_index]);
    }
    return nullptr;
}

int DataManager::feedCount() const
{
    return m_feedmap.count();
}

QList<int> DataManager::getIdList(const Feed *feed) const
{
    return m_entrymap[feed->feedid()];
}

int DataManager::entryCount(const int feed_index) const
{
    return m_entrymap[m_feedmap[feed_index]].count();
}

int DataManager::entryCount(const Feed *feed) const
{
    return m_entrymap[feed->feedid()].count();
}

void DataManager::removeFeed(Feed *feed)
{
    QList<Feed *> feeds;
    feeds << feed;
    removeFeeds(feeds);
}

void DataManager::removeFeed(const int index)
{
    // Get feed pointer
    Feed *feed = getFeed(m_feedmap[index]);
    if (feed) {
        removeFeed(feed);
    }
}

void DataManager::removeFeeds(const QList<int> &feedids)
{
    QList<Feed *> feeds;
    for (const int &feedid : feedids) {
        Feed *feed = getFeed(feedid);
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
            const int feedid = feed->feedid();
            const QString feedurl = feed->url();
            int index = m_feedmap.indexOf(feedid);

            qCDebug(kastsDataManager) << "deleting feed" << feedurl << "with index" << index;

            // Delete the object instances and mappings
            // First delete entries in Queue
            qCDebug(kastsDataManager) << "delete queueentries of" << feedurl;
            QList<int> removeFromQueueList;
            for (auto &entryid : m_queuemap) {
                if (getEntry(entryid)->feed()->feedid() == feedid) {
                    if (AudioManager::instance().entry() == getEntry(entryid)) {
                        AudioManager::instance().next();
                    }
                    removeFromQueueList += entryid;
                }
            }
            bulkQueueStatus(false, removeFromQueueList);

            // Delete entries themselves
            qCDebug(kastsDataManager) << "delete entries of" << feedurl;
            for (auto &entryid : m_entrymap[feedid]) {
                if (getEntry(entryid)->hasEnclosure())
                    getEntry(entryid)->enclosure()->deleteFile(); // delete enclosure (if it exists)
                if (!getEntry(entryid)->image().isEmpty())
                    StorageManager::instance().removeImage(getEntry(entryid)->image()); // delete entry images
                delete m_entries[entryid]; // delete pointer
                m_entries.remove(entryid); // delete the hash key
            }
            m_entrymap.remove(feedid); // remove all the entry mappings belonging to the feed

            qCDebug(kastsDataManager) << "Remove feed image" << feed->image() << "for feed" << feedurl;
            qCDebug(kastsDataManager) << "Remove feed enclosure download directory" << feed->dirname() << "for feed" << feedurl;
            QDir enclosureDir = QDir(StorageManager::instance().enclosureDirPath() + feed->dirname());
            if (!feed->dirname().isEmpty() && enclosureDir.exists()) {
                enclosureDir.removeRecursively();
            }
            if (!feed->image().isEmpty())
                StorageManager::instance().removeImage(feed->image());
            m_feeds.remove(m_feedmap[index]); // remove from m_oldfeeds
            m_feedmap.removeAt(index); // remove from m_oldfeedmap
            delete feed; // remove the pointer

            // Then delete everything from the database
            qCDebug(kastsDataManager) << "delete database part of" << feedurl;

            // Delete related Errors
            QSqlQuery query;
            query.prepare(QStringLiteral("DELETE FROM Errors WHERE url=:url;"));
            query.bindValue(QStringLiteral(":url"), feedurl);
            Database::instance().execute(query);

            // Delete Authors
            query.prepare(QStringLiteral("DELETE FROM Authors WHERE feed=:feed;"));
            query.bindValue(QStringLiteral(":feed"), feedurl);
            Database::instance().execute(query);

            // Delete Chapters
            query.prepare(QStringLiteral("DELETE FROM Chapters WHERE feed=:feed;"));
            query.bindValue(QStringLiteral(":feed"), feedurl);
            Database::instance().execute(query);

            // Delete Entries
            query.prepare(QStringLiteral("DELETE FROM Entries WHERE feed=:feed;"));
            query.bindValue(QStringLiteral(":feed"), feedurl);
            Database::instance().execute(query);

            // Delete Enclosures
            query.prepare(QStringLiteral("DELETE FROM Enclosures WHERE feed=:feed;"));
            query.bindValue(QStringLiteral(":feed"), feedurl);
            Database::instance().execute(query);

            // Delete Feed
            query.prepare(QStringLiteral("DELETE FROM Feeds WHERE url=:url;"));
            query.bindValue(QStringLiteral(":url"), feedurl);
            Database::instance().execute(query);

            // Save this action to the database (including timestamp) in order to be
            // able to sync with remote services
            Sync::instance().storeRemoveFeedAction(feedid);

            Q_EMIT feedRemoved(index);
        }
    }

    // if settings allow, then upload these changes immediately to sync server
    Sync::instance().doQuickSync();
}

void DataManager::addFeed(const QString &url)
{
    addFeed(url, true);
}

void DataManager::addFeed(const QString &url, const bool fetch)
{
    addFeeds(QStringList(url), fetch);
}

void DataManager::addFeeds(const QStringList &urls)
{
    addFeeds(urls, true);
}

void DataManager::addFeeds(const QStringList &urls, const bool fetch)
{
    // First check if the URLs are not empty
    // TODO: Add more checks like checking if URLs exist; however this will mean async...
    QStringList newUrls;
    QList<int> newFeedids;
    for (const QString &url : urls) {
        if (!url.trimmed().isEmpty() && !feedExists(url)) {
            qCDebug(kastsDataManager) << "Feed already exists or URL is empty" << url.trimmed();
            newUrls << url.trimmed();
        }
    }

    if (newUrls.count() == 0)
        return;

    // This method will add the relevant internal data structures, and then add
    // a preliminary entry into the database.  Those details (as well as entries,
    // authors and enclosures) will be updated by calling Fetcher::fetch() which
    // will trigger a full update of the feed and all related items.
    for (const QString &url : std::as_const(newUrls)) {
        qCDebug(kastsDataManager) << "Adding new feed:" << url;

        QString urlFromInput = QUrl::fromUserInput(url).toString();
        QSqlQuery query;
        query.prepare(
            QStringLiteral("INSERT INTO Feeds VALUES (:name, :url, :image, :link, :description, :subscribed, "
                           ":lastUpdated, :new, :dirname, :lastHash, :filterType, :sortType);"));
        query.bindValue(QStringLiteral(":name"), urlFromInput);
        query.bindValue(QStringLiteral(":url"), urlFromInput);
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
        Database::instance().execute(query);

        QVariant lastid = query.lastInsertId();
        Q_ASSERT(lastid.isValid() && !lastid.isNull());
        int feedid = lastid.toInt();
        // TODO: check whether the entry in the database happened correctly?

        newFeedids.append(feedid);
        m_feeds[feedid] = new Feed(feedid);
        m_feedmap.append(feedid);

        // Save this action to the database (including timestamp) in order to be
        // able to sync with remote services
        Sync::instance().storeAddFeedAction(feedid);

        Q_EMIT feedAdded(urlFromInput);
    }

    if (fetch) {
        Fetcher::instance().fetch(newFeedids);
    }

    // if settings allow, upload these changes immediately to sync servers
    Sync::instance().doQuickSync();
}

Entry *DataManager::getQueueEntry(int index) const
{
    return getEntry(m_queuemap[index]);
}

int DataManager::queueCount() const
{
    return m_queuemap.count();
}

QList<int> DataManager::queue() const
{
    return m_queuemap;
}

bool DataManager::entryInQueue(const Entry *entry)
{
    return entryInQueue(entry->entryid());
}

bool DataManager::entryInQueue(const int &entryid) const
{
    return m_queuemap.contains(entryid);
}

void DataManager::moveQueueItem(const int from, const int to)
{
    // First move the items in the internal data structure
    m_queuemap.move(from, to);

    // Then make sure that the database Queue table reflects these changes
    updateQueueListnrs();

    // Make sure that the QueueModel is aware of the changes so it can update
    Q_EMIT queueEntryMoved(from, to);
}

void DataManager::addToQueue(const int &entryid)
{
    // If item is already in queue, then stop here
    if (m_queuemap.contains(entryid))
        return;

    // Add to internal queuemap data structure
    m_queuemap += entryid;
    qCDebug(kastsDataManager) << "Queue mapping is now:" << m_queuemap;

    // Get index of this entry
    const int index = m_queuemap.indexOf(entryid); // add new entry to end of queue

    // Add to Queue database
    QSqlQuery query;
    query.prepare(QStringLiteral("INSERT INTO Queue VALUES (:index, :entryid, :playing);"));
    query.bindValue(QStringLiteral(":index"), index);
    query.bindValue(QStringLiteral(":entryid"), entryid);
    query.bindValue(QStringLiteral(":playing"), false);
    Database::instance().execute(query);

    // Make sure that the QueueModel is aware of the changes
    Q_EMIT queueEntryAdded(index, entryid);
}

void DataManager::removeFromQueue(const int &entryid)
{
    if (!entryInQueue(entryid)) {
        return;
    }

    const int index = m_queuemap.indexOf(entryid);
    qCDebug(kastsDataManager) << "Queuemap is now:" << m_queuemap;
    qCDebug(kastsDataManager) << "Queue index of item to be removed" << index;

    // Move to next track if it's currently playing
    if (AudioManager::instance().entry() == getEntry(entryid)) {
        AudioManager::instance().next();
    }

    // Remove the item from the internal data structure
    m_queuemap.removeAt(index);

    // Then make sure that the database Queue table reflects these changes
    QSqlQuery query;
    query.prepare(QStringLiteral("DELETE FROM Queue WHERE entryid=:entryid;"));
    query.bindValue(QStringLiteral(":entryid"), getEntry(entryid)->entryid());
    Database::instance().execute(query);

    // Make sure that the QueueModel is aware of the change so it can update
    Q_EMIT queueEntryRemoved(index, entryid);
}

void DataManager::sortQueue(AbstractEpisodeProxyModel::SortType sortType)
{
    QString columnName;
    QString order;

    switch (sortType) {
    case AbstractEpisodeProxyModel::SortType::DateAscending:
        order = QStringLiteral("ASC");
        columnName = QStringLiteral("updated");
        break;
    case AbstractEpisodeProxyModel::SortType::DateDescending:
        order = QStringLiteral("DESC");
        columnName = QStringLiteral("updated");
        break;
    }

    QList<int> newQueuemap;

    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT * FROM Queue INNER JOIN Entries ON Queue.entryid = Entries.entryid ORDER BY %1 %2;").arg(columnName, order));
    Database::instance().execute(query);

    while (query.next()) {
        qCDebug(kastsDataManager) << "new queue order:" << query.value(QStringLiteral("entryid")).toInt();
        newQueuemap += query.value(QStringLiteral("entryid")).toInt();
    }

    Database::instance().transaction();
    for (int i = 0; i < m_queuemap.length(); i++) {
        query.prepare(QStringLiteral("UPDATE Queue SET listnr=:listnr WHERE entryid=:entryid;"));
        query.bindValue(QStringLiteral(":entryid"), getEntry(newQueuemap[i])->entryid());
        query.bindValue(QStringLiteral(":listnr"), i);
        Database::instance().execute(query);
    }
    Database::instance().commit();

    m_queuemap.clear();
    m_queuemap = newQueuemap;

    Q_EMIT queueSorted();
}

int DataManager::lastPlayingEntry()
{
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT entryid FROM Queue WHERE playing=:playing;"));
    query.bindValue(QStringLiteral(":playing"), true);
    Database::instance().execute(query);
    if (!query.next())
        return -1;
    return query.value(QStringLiteral("entryid")).toInt();
}

void DataManager::setLastPlayingEntry(const int &entryid)
{
    QSqlQuery query;
    // First set playing to false for all Queue items
    query.prepare(QStringLiteral("UPDATE Queue SET playing=:playing;"));
    query.bindValue(QStringLiteral(":playing"), false);
    Database::instance().execute(query);
    // Now set the correct track to playing=true
    query.prepare(QStringLiteral("UPDATE Queue SET playing=:playing WHERE entryid=:entryid;"));
    query.bindValue(QStringLiteral(":playing"), true);
    query.bindValue(QStringLiteral(":entryid"), entryid);
    Database::instance().execute(query);
}

void DataManager::deletePlayedEnclosures()
{
    QSqlQuery query;
    query.prepare(
        QStringLiteral("SELECT * FROM Enclosures INNER JOIN Entries ON Enclosures.entryid = Entries.entryid WHERE"
                       "(downloaded=:downloaded OR downloaded=:partiallydownloaded) AND (read=:read);"));
    query.bindValue(QStringLiteral(":downloaded"), Enclosure::statusToDb(Enclosure::Downloaded));
    query.bindValue(QStringLiteral(":partiallydownloaded"), Enclosure::statusToDb(Enclosure::PartiallyDownloaded));
    query.bindValue(QStringLiteral(":read"), true);
    Database::instance().execute(query);
    while (query.next()) {
        int entryid = query.value(QStringLiteral("entryid")).toInt();
        qCDebug(kastsDataManager) << "Found entry which has been downloaded and is marked as played; deleting now, entryid is:" << entryid;
        QString id = query.value(QStringLiteral("id")).toString();
        Entry *entry = getEntry(entryid);
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
        addFeeds(urls);
    }
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
        xmlWriter.writeEmptyElement(QStringLiteral("head"));
        xmlWriter.writeStartElement(QStringLiteral("body"));
        xmlWriter.writeAttribute(QStringLiteral("version"), QStringLiteral("1.0"));
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
}

void DataManager::loadFeed(const int &feedid) const
{
    if (m_feeds[feedid]) {
        // nothing to do if Feed object already exists
        return;
    }

    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT feedid FROM Feeds WHERE feedid=:feedid;"));
    query.bindValue(QStringLiteral(":feedid"), feedid);
    Database::instance().execute(query);
    if (!query.next()) {
        qWarning() << "Failed to load feed" << feedid;
    } else {
        m_feeds[feedid] = new Feed(feedid);
    }
}

void DataManager::loadEntry(const int &entryid) const
{
    // First find the feed that this entry belongs to
    Feed *feed = nullptr;
    QHashIterator<int, QList<int>> i(m_entrymap);
    while (i.hasNext()) {
        i.next();
        if (i.value().contains(entryid))
            feed = getFeed(i.key());
    }
    if (!feed) {
        qCDebug(kastsDataManager) << "Failed to find feed belonging to entry" << entryid;
        return;
    }
    m_entries[entryid] = new Entry(feed, entryid);
}

bool DataManager::feedExists(const QString &url)
{
    // using cleanUrl to do "fuzzy" check on the podcast URL
    QString cleanedUrl = cleanUrl(url);

    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT url FROM Feeds;"));
    Database::instance().execute(query);
    while (query.next()) {
        if (cleanedUrl == cleanUrl(query.value(QStringLiteral("url")).toString())) {
            return true;
        }
    }
    return false;
}

void DataManager::updateQueueListnrs() const
{
    QSqlQuery query;
    query.prepare(QStringLiteral("UPDATE Queue SET listnr=:i WHERE entryid=:entryid;"));
    for (int i = 0; i < m_queuemap.count(); i++) {
        query.bindValue(QStringLiteral(":i"), i);
        query.bindValue(QStringLiteral(":entryid"), m_queuemap[i]);
        Database::instance().execute(query);
    }
}

void DataManager::bulkMarkReadByIndex(bool state, const QModelIndexList &list)
{
    bulkMarkRead(state, getIdsFromModelIndexList(list));
}

void DataManager::bulkMarkRead(bool state, const QList<int> &list)
{
    Database::instance().transaction();

    if (state) { // Mark as read
        // This needs special attention as the DB operations are very intensive.
        // Reversing the loop is much faster
        for (int i = list.count() - 1; i >= 0; i--) {
            getEntry(list[i])->setReadInternal(state);
        }
        updateQueueListnrs(); // update queue after modification
    } else { // Mark as unread
        for (const int &entryid : list) {
            getEntry(entryid)->setReadInternal(state);
        }
    }
    Database::instance().commit();

    Q_EMIT bulkReadStatusActionFinished();

    // if settings allow, upload these changes immediately to sync servers
    if (state) {
        Sync::instance().doQuickSync();
    }
}

void DataManager::bulkMarkNewByIndex(bool state, const QModelIndexList &list)
{
    bulkMarkNew(state, getIdsFromModelIndexList(list));
}

void DataManager::bulkMarkNew(bool state, const QList<int> &list)
{
    Database::instance().transaction();
    for (const int &entryid : list) {
        getEntry(entryid)->setNewInternal(state);
    }
    Database::instance().commit();

    Q_EMIT bulkNewStatusActionFinished();
}

void DataManager::bulkMarkFavoriteByIndex(bool state, const QModelIndexList &list)
{
    bulkMarkFavorite(state, getIdsFromModelIndexList(list));
}

void DataManager::bulkMarkFavorite(bool state, const QList<int> &list)
{
    Database::instance().transaction();
    for (const int &entryid : list) {
        getEntry(entryid)->setFavoriteInternal(state);
    }
    Database::instance().commit();

    Q_EMIT bulkFavoriteStatusActionFinished();
}

void DataManager::bulkQueueStatusByIndex(bool state, const QModelIndexList &list)
{
    bulkQueueStatus(state, getIdsFromModelIndexList(list));
}

void DataManager::bulkQueueStatus(bool state, const QList<int> &list)
{
    Database::instance().transaction();
    if (state) { // i.e. add to queue
        for (const int &entryid : list) {
            getEntry(entryid)->setQueueStatusInternal(state);
        }
    } else { // i.e. remove from queue
        // This needs special attention as the DB operations are very intensive.
        // Reversing the loop is much faster.
        for (int i = list.count() - 1; i >= 0; i--) {
            qCDebug(kastsDataManager) << "getting entry" << getEntry(list[i])->id();
            getEntry(list[i])->setQueueStatusInternal(state);
        }
        updateQueueListnrs();
    }
    Database::instance().commit();

    Q_EMIT bulkReadStatusActionFinished();
    Q_EMIT bulkNewStatusActionFinished();
}

void DataManager::bulkDownloadEnclosuresByIndex(const QModelIndexList &list)
{
    bulkDownloadEnclosures(getIdsFromModelIndexList(list));
}

void DataManager::bulkDownloadEnclosures(const QList<int> &list)
{
    bulkQueueStatus(true, list);
    for (const int &entryid : list) {
        if (getEntry(entryid)->hasEnclosure()) {
            getEntry(entryid)->enclosure()->download();
        }
    }
}

void DataManager::bulkDeleteEnclosuresByIndex(const QModelIndexList &list)
{
    bulkDeleteEnclosures(getIdsFromModelIndexList(list));
}

void DataManager::bulkDeleteEnclosures(const QList<int> &list)
{
    Database::instance().transaction();
    for (const int &entryid : list) {
        if (getEntry(entryid)->hasEnclosure()) {
            getEntry(entryid)->enclosure()->deleteFile();
        }
    }
    Database::instance().commit();
}

QList<int> DataManager::getIdsFromModelIndexList(const QModelIndexList &list) const
{
    QList<int> entryids;
    for (QModelIndex index : list) {
        entryids += index.data(EpisodeModel::Roles::EntryIdRole).value<int>();
    }
    qCDebug(kastsDataManager) << "Ids of selection:" << entryids;
    return entryids;
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
