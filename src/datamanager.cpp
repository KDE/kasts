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
#include "settingsmanager.h"
#include "storagemanager.h"
#include "sync/sync.h"

DataManager::DataManager()
{
    connect(
        &Fetcher::instance(),
        &Fetcher::feedDetailsUpdated,
        this,
        [this](const QString &url, const QString &name, const QString &image, const QString &link, const QString &description, const QDateTime &lastUpdated) {
            qCDebug(kastsDataManager) << "Start updating feed details for" << url;
            Feed *feed = getFeed(url);
            if (feed != nullptr) {
                feed->setName(name);
                feed->setImage(image);
                feed->setLink(link);
                feed->setDescription(description);
                feed->setLastUpdated(lastUpdated);
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

        // Check for "new" entries
        if (SettingsManager::self()->autoQueue()) {
            // start an immediate transaction since this non-blocking read query
            // can change into a blocking write query if the entry needs to be
            // queued; this can create a deadlock with other concurrent write
            // operations
            Database::instance().transaction();
            query.prepare(QStringLiteral("SELECT id FROM Entries WHERE feed=:feed AND new=:new ORDER BY updated ASC;"));
            query.bindValue(QStringLiteral(":feed"), feedurl);
            query.bindValue(QStringLiteral(":new"), true);
            Database::instance().execute(query);
            while (query.next()) {
                QString id = query.value(QStringLiteral("id")).toString();
                getEntry(id)->setQueueStatusInternal(true);
                if (SettingsManager::self()->autoDownload()) {
                    if (getEntry(id) && getEntry(id)->hasEnclosure() && getEntry(id)->enclosure()) {
                        qCDebug(kastsDataManager) << "Start downloading" << getEntry(id)->title();
                        getEntry(id)->enclosure()->download();
                    }
                }
            }
            Database::instance().commit();
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
    return getFeed(m_feedmap[index]);
}

Feed *DataManager::getFeed(const QString &feedurl) const
{
    if (m_feeds[feedurl] == nullptr)
        loadFeed(feedurl);
    return m_feeds[feedurl];
}

Entry *DataManager::getEntry(const int feed_index, const int entry_index) const
{
    return getEntry(m_entrymap[m_feedmap[feed_index]][entry_index]);
}

Entry *DataManager::getEntry(const Feed *feed, const int entry_index) const
{
    return getEntry(m_entrymap[feed->url()][entry_index]);
}

Entry *DataManager::getEntry(const QString &id) const
{
    if (m_entries[id] == nullptr)
        loadEntry(id);
    return m_entries[id];
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

int DataManager::newEntryCount(const Feed *feed) const
{
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT COUNT (id) FROM Entries where feed=:feed AND new=1;"));
    query.bindValue(QStringLiteral(":feed"), feed->url());
    Database::instance().execute(query);
    if (!query.next())
        return -1;
    return query.value(0).toInt();
}

int DataManager::favoriteEntryCount(const Feed *feed) const
{
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT COUNT (id) FROM Entries where feed=:feed AND favorite=1;"));
    query.bindValue(QStringLiteral(":feed"), feed->url());
    Database::instance().execute(query);
    if (!query.next())
        return -1;
    return query.value(0).toInt();
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
    for (QString feedurl : feedurls) {
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
    for (QVariant feedVariant : feedVariantList) {
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
        if (!url.trimmed().isEmpty()) {
            newUrls << url.trimmed();
        }
    }

    if (newUrls.count() == 0)
        return;

    // This method will add the relevant internal data structures, and then add
    // a preliminary entry into the database.  Those details (as well as entries,
    // authors and enclosures) will be updated by calling Fetcher::fetch() which
    // will trigger a full update of the feed and all related items.
    for (const QString &url : newUrls) {
        qCDebug(kastsDataManager) << "Adding feed";
        if (feedExists(url)) {
            qCDebug(kastsDataManager) << "Feed already exists";
            continue;
        }
        qCDebug(kastsDataManager) << "Feed does not yet exist";

        QUrl urlFromInput = QUrl::fromUserInput(url);
        QSqlQuery query;
        query.prepare(
            QStringLiteral("INSERT INTO Feeds VALUES (:name, :url, :image, :link, :description, :deleteAfterCount, :deleteAfterType, :subscribed, "
                           ":lastUpdated, :new, :notify);"));
        query.bindValue(QStringLiteral(":name"), urlFromInput.toString());
        query.bindValue(QStringLiteral(":url"), urlFromInput.toString());
        query.bindValue(QStringLiteral(":image"), QLatin1String(""));
        query.bindValue(QStringLiteral(":link"), QLatin1String(""));
        query.bindValue(QStringLiteral(":description"), QLatin1String(""));
        query.bindValue(QStringLiteral(":deleteAfterCount"), 0);
        query.bindValue(QStringLiteral(":deleteAfterType"), 0);
        query.bindValue(QStringLiteral(":subscribed"), QDateTime::currentDateTime().toSecsSinceEpoch());
        query.bindValue(QStringLiteral(":lastUpdated"), 0);
        query.bindValue(QStringLiteral(":new"), true);
        query.bindValue(QStringLiteral(":notify"), false);
        Database::instance().execute(query);

        m_feeds[urlFromInput.toString()] = nullptr;
        m_feedmap.append(urlFromInput.toString());

        // Save this action to the database (including timestamp) in order to be
        // able to sync with remote services
        Sync::instance().storeAddFeedAction(urlFromInput.toString());

        Q_EMIT feedAdded(urlFromInput.toString());
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
        QString feed = query.value(QStringLiteral("feed")).toString();
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

void DataManager::loadFeed(const QString &feedurl) const
{
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
    for (QString listUrl : m_feedmap) {
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

void DataManager::bulkMarkReadByIndex(bool state, QModelIndexList list)
{
    bulkMarkRead(state, getIdsFromModelIndexList(list));
}

void DataManager::bulkMarkRead(bool state, QStringList list)
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
        for (QString id : list) {
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

void DataManager::bulkMarkNewByIndex(bool state, QModelIndexList list)
{
    bulkMarkNew(state, getIdsFromModelIndexList(list));
}

void DataManager::bulkMarkNew(bool state, QStringList list)
{
    Database::instance().transaction();
    for (QString id : list) {
        getEntry(id)->setNewInternal(state);
    }
    Database::instance().commit();

    Q_EMIT bulkNewStatusActionFinished();
}

void DataManager::bulkMarkFavoriteByIndex(bool state, QModelIndexList list)
{
    bulkMarkFavorite(state, getIdsFromModelIndexList(list));
}

void DataManager::bulkMarkFavorite(bool state, QStringList list)
{
    Database::instance().transaction();
    for (QString id : list) {
        getEntry(id)->setFavoriteInternal(state);
    }
    Database::instance().commit();

    Q_EMIT bulkFavoriteStatusActionFinished();
}

void DataManager::bulkQueueStatusByIndex(bool state, QModelIndexList list)
{
    bulkQueueStatus(state, getIdsFromModelIndexList(list));
}

void DataManager::bulkQueueStatus(bool state, QStringList list)
{
    Database::instance().transaction();
    if (state) { // i.e. add to queue
        for (QString id : list) {
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

void DataManager::bulkDownloadEnclosuresByIndex(QModelIndexList list)
{
    bulkDownloadEnclosures(getIdsFromModelIndexList(list));
}

void DataManager::bulkDownloadEnclosures(QStringList list)
{
    bulkQueueStatus(true, list);
    for (QString id : list) {
        if (getEntry(id)->hasEnclosure()) {
            getEntry(id)->enclosure()->download();
        }
    }
}

void DataManager::bulkDeleteEnclosuresByIndex(QModelIndexList list)
{
    bulkDeleteEnclosures(getIdsFromModelIndexList(list));
}

void DataManager::bulkDeleteEnclosures(QStringList list)
{
    Database::instance().transaction();
    for (QString id : list) {
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
        + (QUrl(url).hasQuery() ? QStringLiteral("?") + QUrl(url).query(QUrl::FullyDecoded) : QStringLiteral(""));
}
