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
            [this](const QString &url,
                   const QString &name,
                   const QString &image,
                   const QString &link,
                   const QString &description,
                   const QDateTime &lastUpdated,
                   const QString &dirname) {
                qCDebug(kastsDataManager) << "Start updating feed details for" << url;
                Feed *feed = getFeed(url);
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
    connect(&Fetcher::instance(), &Fetcher::entryAdded, this, [this](const QString &feedurl, const QString &id) {
        Q_UNUSED(feedurl)
        // Only add the new entry to m_entries
        // we will repopulate m_entrymap once all new entries have been added,
        // such that m_entrymap will show all new entries in the correct order
        m_entries[id] = nullptr;
    });
    connect(&Fetcher::instance(), &Fetcher::feedUpdated, this, [this](const QString &feedurl) {
        // Update m_entrymap for feedurl, such that the new and old entries show
        // up in the correct order
        // TODO: put this code into a separate method and re-use this in the constructor
        QSqlQuery query;
        m_entrymap[feedurl].clear();
        query.prepare(QStringLiteral("SELECT id FROM Entries WHERE feed=:feed ORDER BY updated DESC;"));
        query.bindValue(QStringLiteral(":feed"), feedurl);
        Database::instance().execute(query);
        while (query.next()) {
            m_entrymap[feedurl] += query.value(QStringLiteral("id")).toString();
        }

        Q_EMIT feedEntriesUpdated(feedurl);
    });

    // Only read unique feedurls and entry ids from the database.
    // The feed and entry datastructures will be loaded lazily.
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT url FROM Feeds;"));
    Database::instance().execute(query);
    while (query.next()) {
        m_feedmap += query.value(QStringLiteral("url")).toString();
        m_feeds[query.value(QStringLiteral("url")).toString()] = nullptr;
    }

    for (auto &feedurl : m_feedmap) {
        query.prepare(QStringLiteral("SELECT id FROM Entries WHERE feed=:feed ORDER BY updated DESC;"));
        query.bindValue(QStringLiteral(":feed"), feedurl);
        Database::instance().execute(query);
        while (query.next()) {
            m_entrymap[feedurl] += query.value(QStringLiteral("id")).toString();
            m_entries[query.value(QStringLiteral("id")).toString()] = nullptr;
        }
    }
    // qCDebug(kastsDataManager) << "entrymap contains:" << m_entrymap;

    query.prepare(QStringLiteral("SELECT id FROM Queue ORDER BY listnr;"));
    Database::instance().execute(query);
    while (query.next()) {
        m_queuemap += query.value(QStringLiteral("id")).toString();
    }
    qCDebug(kastsDataManager) << "Queuemap contains:" << m_queuemap;
}

Feed *DataManager::getFeed(const int index) const
{
    if (index >= 0 && index < m_feedmap.size()) {
        return getFeed(m_feedmap[index]);
    }
    return nullptr;
}

Feed *DataManager::getFeed(const QString &feedurl) const
{
    if (m_feeds.contains(feedurl)) {
        if (m_feeds[feedurl] == nullptr) {
            loadFeed(feedurl);
        }
        return m_feeds[feedurl];
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
    if (feed && entry_index < m_entrymap[feed->url()].size()) {
        return getEntry(m_entrymap[feed->url()][entry_index]);
    }
    return nullptr;
}

Entry *DataManager::getEntry(const QString &id) const
{
    if (m_entries.contains(id)) {
        if (m_entries[id] == nullptr)
            loadEntry(id);
        return m_entries[id];
    }
    return nullptr;
}

int DataManager::feedCount() const
{
    return m_feedmap.count();
}

QStringList DataManager::getIdList(const Feed *feed) const
{
    return m_entrymap[feed->url()];
}

int DataManager::entryCount(const int feed_index) const
{
    return m_entrymap[m_feedmap[feed_index]].count();
}

int DataManager::entryCount(const Feed *feed) const
{
    return m_entrymap[feed->url()].count();
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
            const QString feedurl = feed->url();
            int index = m_feedmap.indexOf(feedurl);

            qCDebug(kastsDataManager) << "deleting feed" << feedurl << "with index" << index;

            // Delete the object instances and mappings
            // First delete entries in Queue
            qCDebug(kastsDataManager) << "delete queueentries of" << feedurl;
            QStringList removeFromQueueList;
            for (auto &id : m_queuemap) {
                if (getEntry(id)->feed()->url() == feedurl) {
                    if (AudioManager::instance().entry() == getEntry(id)) {
                        AudioManager::instance().next();
                    }
                    removeFromQueueList += id;
                }
            }
            bulkQueueStatus(false, removeFromQueueList);

            // Delete entries themselves
            qCDebug(kastsDataManager) << "delete entries of" << feedurl;
            for (auto &id : m_entrymap[feedurl]) {
                if (getEntry(id)->hasEnclosure())
                    getEntry(id)->enclosure()->deleteFile(); // delete enclosure (if it exists)
                if (!getEntry(id)->image().isEmpty())
                    StorageManager::instance().removeImage(getEntry(id)->image()); // delete entry images
                delete m_entries[id]; // delete pointer
                m_entries.remove(id); // delete the hash key
            }
            m_entrymap.remove(feedurl); // remove all the entry mappings belonging to the feed

            qCDebug(kastsDataManager) << "Remove feed image" << feed->image() << "for feed" << feedurl;
            qCDebug(kastsDataManager) << "Remove feed enclosure download directory" << feed->dirname() << "for feed" << feedurl;
            QDir enclosureDir = QDir(StorageManager::instance().enclosureDirPath() + feed->dirname());
            if (!feed->dirname().isEmpty() && enclosureDir.exists()) {
                enclosureDir.removeRecursively();
            }
            if (!feed->image().isEmpty())
                StorageManager::instance().removeImage(feed->image());
            m_feeds.remove(m_feedmap[index]); // remove from m_feeds
            m_feedmap.removeAt(index); // remove from m_feedmap
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
            Sync::instance().storeRemoveFeedAction(feedurl);

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
            QStringLiteral("INSERT INTO Feeds VALUES (:name, :url, :image, :link, :description, :deleteAfterCount, :deleteAfterType, :subscribed, "
                           ":lastUpdated, :new, :notify, :dirname, :lastHash, :filterType, :sortType);"));
        query.bindValue(QStringLiteral(":name"), urlFromInput);
        query.bindValue(QStringLiteral(":url"), urlFromInput);
        query.bindValue(QStringLiteral(":image"), QLatin1String(""));
        query.bindValue(QStringLiteral(":link"), QLatin1String(""));
        query.bindValue(QStringLiteral(":description"), QLatin1String(""));
        query.bindValue(QStringLiteral(":deleteAfterCount"), 0);
        query.bindValue(QStringLiteral(":deleteAfterType"), 0);
        query.bindValue(QStringLiteral(":subscribed"), QDateTime::currentSecsSinceEpoch());
        query.bindValue(QStringLiteral(":lastUpdated"), 0);
        query.bindValue(QStringLiteral(":new"), true);
        query.bindValue(QStringLiteral(":notify"), false);
        query.bindValue(QStringLiteral(":dirname"), QLatin1String(""));
        query.bindValue(QStringLiteral(":lastHash"), QLatin1String(""));
        query.bindValue(QStringLiteral(":filterType"), 0);
        query.bindValue(QStringLiteral(":sortType"), 0);
        Database::instance().execute(query);

        // TODO: check whether the entry in the database happened correctly?

        m_feeds[urlFromInput] = new Feed(urlFromInput);
        m_feedmap.append(urlFromInput);

        // Save this action to the database (including timestamp) in order to be
        // able to sync with remote services
        Sync::instance().storeAddFeedAction(urlFromInput);

        Q_EMIT feedAdded(urlFromInput);
    }

    if (fetch) {
        Fetcher::instance().fetch(urls);
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

QStringList DataManager::queue() const
{
    return m_queuemap;
}

bool DataManager::entryInQueue(const Entry *entry)
{
    return entryInQueue(entry->id());
}

bool DataManager::entryInQueue(const QString &id) const
{
    return m_queuemap.contains(id);
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

void DataManager::addToQueue(const QString &id)
{
    // If item is already in queue, then stop here
    if (m_queuemap.contains(id))
        return;

    // Add to internal queuemap data structure
    m_queuemap += id;
    qCDebug(kastsDataManager) << "Queue mapping is now:" << m_queuemap;

    // Get index of this entry
    const int index = m_queuemap.indexOf(id); // add new entry to end of queue

    // Add to Queue database
    QSqlQuery query;
    query.prepare(QStringLiteral("INSERT INTO Queue VALUES (:index, :feedurl, :id, :playing);"));
    query.bindValue(QStringLiteral(":index"), index);
    query.bindValue(QStringLiteral(":feedurl"), getEntry(id)->feed()->url());
    query.bindValue(QStringLiteral(":id"), id);
    query.bindValue(QStringLiteral(":playing"), false);
    Database::instance().execute(query);

    // Make sure that the QueueModel is aware of the changes
    Q_EMIT queueEntryAdded(index, id);
}

void DataManager::removeFromQueue(const QString &id)
{
    if (!entryInQueue(id)) {
        return;
    }

    const int index = m_queuemap.indexOf(id);
    qCDebug(kastsDataManager) << "Queuemap is now:" << m_queuemap;
    qCDebug(kastsDataManager) << "Queue index of item to be removed" << index;

    // Move to next track if it's currently playing
    if (AudioManager::instance().entry() == getEntry(id)) {
        AudioManager::instance().next();
    }

    // Remove the item from the internal data structure
    m_queuemap.removeAt(index);

    // Then make sure that the database Queue table reflects these changes
    QSqlQuery query;
    query.prepare(QStringLiteral("DELETE FROM Queue WHERE id=:id;"));
    query.bindValue(QStringLiteral(":id"), id);
    Database::instance().execute(query);

    // Make sure that the QueueModel is aware of the change so it can update
    Q_EMIT queueEntryRemoved(index, id);
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

    QStringList newQueuemap;

    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT * FROM Queue INNER JOIN Entries ON Queue.id = Entries.id ORDER BY %1 %2;").arg(columnName, order));
    Database::instance().execute(query);

    while (query.next()) {
        qCDebug(kastsDataManager) << "new queue order:" << query.value(QStringLiteral("id")).toString();
        newQueuemap += query.value(QStringLiteral("id")).toString();
    }

    Database::instance().transaction();
    for (int i = 0; i < m_queuemap.length(); i++) {
        query.prepare(QStringLiteral("UPDATE Queue SET listnr=:listnr WHERE id=:id;"));
        query.bindValue(QStringLiteral(":id"), newQueuemap[i]);
        query.bindValue(QStringLiteral(":listnr"), i);
        Database::instance().execute(query);
    }
    Database::instance().commit();

    m_queuemap.clear();
    m_queuemap = newQueuemap;

    Q_EMIT queueSorted();
}

QString DataManager::lastPlayingEntry()
{
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT id FROM Queue WHERE playing=:playing;"));
    query.bindValue(QStringLiteral(":playing"), true);
    Database::instance().execute(query);
    if (!query.next())
        return QStringLiteral("none");
    return query.value(QStringLiteral("id")).toString();
}

void DataManager::setLastPlayingEntry(const QString &id)
{
    QSqlQuery query;
    // First set playing to false for all Queue items
    query.prepare(QStringLiteral("UPDATE Queue SET playing=:playing;"));
    query.bindValue(QStringLiteral(":playing"), false);
    Database::instance().execute(query);
    // Now set the correct track to playing=true
    query.prepare(QStringLiteral("UPDATE Queue SET playing=:playing WHERE id=:id;"));
    query.bindValue(QStringLiteral(":playing"), true);
    query.bindValue(QStringLiteral(":id"), id);
    Database::instance().execute(query);
}

void DataManager::deletePlayedEnclosures()
{
    QSqlQuery query;
    query.prepare(
        QStringLiteral("SELECT * FROM Enclosures INNER JOIN Entries ON Enclosures.id = Entries.id WHERE"
                       "(downloaded=:downloaded OR downloaded=:partiallydownloaded) AND (read=:read);"));
    query.bindValue(QStringLiteral(":downloaded"), Enclosure::statusToDb(Enclosure::Downloaded));
    query.bindValue(QStringLiteral(":partiallydownloaded"), Enclosure::statusToDb(Enclosure::PartiallyDownloaded));
    query.bindValue(QStringLiteral(":read"), true);
    Database::instance().execute(query);
    while (query.next()) {
        QString id = query.value(QStringLiteral("id")).toString();
        qCDebug(kastsDataManager) << "Found entry which has been downloaded and is marked as played; deleting now:" << id;
        Entry *entry = getEntry(id);
        if (entry->hasEnclosure()) {
            entry->enclosure()->deleteFile();
        }
    }
}

void DataManager::importFeeds(const QString &path)
{
    QUrl url(path);
    QFile file(url.isLocalFile() ? url.toLocalFile() : url.toString());

    file.open(QIODevice::ReadOnly);

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

void DataManager::exportFeeds(const QString &path)
{
    QUrl url(path);
    QFile file(url.isLocalFile() ? url.toLocalFile() : url.toString());
    file.open(QIODevice::WriteOnly);

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

void DataManager::loadFeed(const QString &feedurl) const
{
    if (m_feeds[feedurl]) {
        // nothing to do if Feed object already exists
        return;
    }

    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT url FROM Feeds WHERE url=:feedurl;"));
    query.bindValue(QStringLiteral(":feedurl"), feedurl);
    Database::instance().execute(query);
    if (!query.next()) {
        qWarning() << "Failed to load feed" << feedurl;
    } else {
        m_feeds[feedurl] = new Feed(feedurl);
    }
}

void DataManager::loadEntry(const QString id) const
{
    // First find the feed that this entry belongs to
    Feed *feed = nullptr;
    QHashIterator<QString, QStringList> i(m_entrymap);
    while (i.hasNext()) {
        i.next();
        if (i.value().contains(id))
            feed = getFeed(i.key());
    }
    if (!feed) {
        qCDebug(kastsDataManager) << "Failed to find feed belonging to entry" << id;
        return;
    }
    m_entries[id] = new Entry(feed, id);
}

bool DataManager::feedExists(const QString &url)
{
    // using cleanUrl to do "fuzzy" check on the podcast URL
    QString cleanedUrl = cleanUrl(url);
    for (const QString &listUrl : std::as_const(m_feedmap)) {
        if (cleanedUrl == cleanUrl(listUrl)) {
            return true;
        }
    }
    return false;
}

void DataManager::updateQueueListnrs() const
{
    QSqlQuery query;
    query.prepare(QStringLiteral("UPDATE Queue SET listnr=:i WHERE id=:id;"));
    for (int i = 0; i < m_queuemap.count(); i++) {
        query.bindValue(QStringLiteral(":i"), i);
        query.bindValue(QStringLiteral(":id"), m_queuemap[i]);
        Database::instance().execute(query);
    }
}

void DataManager::bulkMarkReadByIndex(bool state, const QModelIndexList &list)
{
    bulkMarkRead(state, getIdsFromModelIndexList(list));
}

void DataManager::bulkMarkRead(bool state, const QStringList &list)
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
        for (const QString &id : list) {
            getEntry(id)->setReadInternal(state);
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

void DataManager::bulkMarkNew(bool state, const QStringList &list)
{
    Database::instance().transaction();
    for (const QString &id : list) {
        getEntry(id)->setNewInternal(state);
    }
    Database::instance().commit();

    Q_EMIT bulkNewStatusActionFinished();
}

void DataManager::bulkMarkFavoriteByIndex(bool state, const QModelIndexList &list)
{
    bulkMarkFavorite(state, getIdsFromModelIndexList(list));
}

void DataManager::bulkMarkFavorite(bool state, const QStringList &list)
{
    Database::instance().transaction();
    for (const QString &id : list) {
        getEntry(id)->setFavoriteInternal(state);
    }
    Database::instance().commit();

    Q_EMIT bulkFavoriteStatusActionFinished();
}

void DataManager::bulkQueueStatusByIndex(bool state, const QModelIndexList &list)
{
    bulkQueueStatus(state, getIdsFromModelIndexList(list));
}

void DataManager::bulkQueueStatus(bool state, const QStringList &list)
{
    Database::instance().transaction();
    if (state) { // i.e. add to queue
        for (const QString &id : list) {
            getEntry(id)->setQueueStatusInternal(state);
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

void DataManager::bulkDownloadEnclosures(const QStringList &list)
{
    bulkQueueStatus(true, list);
    for (const QString &id : list) {
        if (getEntry(id)->hasEnclosure()) {
            getEntry(id)->enclosure()->download();
        }
    }
}

void DataManager::bulkDeleteEnclosuresByIndex(const QModelIndexList &list)
{
    bulkDeleteEnclosures(getIdsFromModelIndexList(list));
}

void DataManager::bulkDeleteEnclosures(const QStringList &list)
{
    Database::instance().transaction();
    for (const QString &id : list) {
        if (getEntry(id)->hasEnclosure()) {
            getEntry(id)->enclosure()->deleteFile();
        }
    }
    Database::instance().commit();
}

QStringList DataManager::getIdsFromModelIndexList(const QModelIndexList &list) const
{
    QStringList ids;
    for (QModelIndex index : list) {
        ids += index.data(EpisodeModel::Roles::IdRole).value<QString>();
    }
    qCDebug(kastsDataManager) << "Ids of selection:" << ids;
    return ids;
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
