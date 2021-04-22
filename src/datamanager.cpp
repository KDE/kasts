/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include <QDateTime>
#include <QDir>
#include <QSqlDatabase>
#include <QSqlError>
#include <QStandardPaths>
#include <QUrl>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include "datamanager.h"
#include "fetcher.h"
#include "database.h"
#include "settingsmanager.h"

DataManager::DataManager()
{
    // connect signals to lambda slots
    connect(&Fetcher::instance(), &Fetcher::feedDetailsUpdated, this, [this](const QString &url, const QString &name, const QString &image, const QString &link, const QString &description, const QDateTime &lastUpdated) {
        //qDebug() << "Start updating feed details" << m_feeds;
        Feed* feed = getFeed(url);
        if (feed != nullptr) {
            feed->setName(name);
            feed->setImage(image);
            feed->setLink(link);
            feed->setDescription(description);
            feed->setLastUpdated(lastUpdated);
            //qDebug() << "Retrieving authors";
            feed->updateAuthors();
            // TODO: signal feedmodel: Q_EMIT dataChanged(createIndex(i, 0), createIndex(i, 0));
            // quite sure that this is actually not needed
        }
    });
    connect(&Fetcher::instance(), &Fetcher::entryAdded, this, [this](const QString &feedurl, const QString &id) {
        // Only add the new entry to m_entries
        // we will repopulate m_entrymap once all new entries have been added,
        // such that m_entrymap will show all new entries in the correct order
        m_entries[id] = nullptr;
        Q_EMIT entryAdded(feedurl, id);
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
            //qDebug() << m_entrymap[feedurl];
        }

        // Check for "new" entries
        if (SettingsManager::self()->autoQueue()) {
            query.prepare(QStringLiteral("SELECT id FROM Entries WHERE feed=:feed AND new=:new;"));
            query.bindValue(QStringLiteral(":feed"), feedurl);
            query.bindValue(QStringLiteral(":new"), true);
            Database::instance().execute(query);
            while (query.next()) {
                QString const id = query.value(QStringLiteral("id")).toString();
                addToQueue(feedurl, id);
                if (SettingsManager::self()->autoDownload()) {
                    if (getEntry(id)->hasEnclosure()) {
                        qDebug() << "Start downloading" << getEntry(id)->title();
                        getEntry(id)->enclosure()->download();
                    }
                }
            }

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
    //qDebug() << m_feedmap;

    for (auto &feedurl : m_feedmap) {
        query.prepare(QStringLiteral("SELECT id FROM Entries WHERE feed=:feed ORDER BY updated DESC;"));
        query.bindValue(QStringLiteral(":feed"), feedurl);
        Database::instance().execute(query);
        while (query.next()) {
            m_entrymap[feedurl] += query.value(QStringLiteral("id")).toString();
            m_entries[query.value(QStringLiteral("id")).toString()] = nullptr;
            //qDebug() << m_entrymap[feedurl];
        }
    }
    //qDebug() << m_entrymap;

    query.prepare(QStringLiteral("SELECT id FROM Queue ORDER BY listnr;"));
    Database::instance().execute(query);
    while (query.next()) {
        m_queuemap += query.value(QStringLiteral("id")).toString();
    }
    //qDebug() << m_queuemap;
}

Feed* DataManager::getFeed(int const index) const
{
    return getFeed(m_feedmap[index]);
}

Feed* DataManager::getFeed(QString const feedurl) const
{
    if (m_feeds[feedurl] == nullptr)
        loadFeed(feedurl);
    return m_feeds[feedurl];
}


Entry* DataManager::getEntry(int const feed_index, int const entry_index) const
{
    return getEntry(m_entrymap[m_feedmap[feed_index]][entry_index]);
}

Entry* DataManager::getEntry(const Feed* feed, int const entry_index) const
{
    return getEntry(m_entrymap[feed->url()][entry_index]);
}

Entry* DataManager::getEntry(QString id) const
{
    if (m_entries[id] == nullptr)
        loadEntry(id);
    return m_entries[id];
}

Entry* DataManager::getEntry(const EpisodeModel::Type type, const int entry_index) const
{
    QSqlQuery entryQuery;
    if (type == EpisodeModel::All || type == EpisodeModel::New || type == EpisodeModel::Unread || type == EpisodeModel::Downloaded) {

        if (type == EpisodeModel::New) {
            entryQuery.prepare(QStringLiteral("SELECT id FROM Entries WHERE new=:new ORDER BY updated DESC LIMIT 1 OFFSET :index;"));
            entryQuery.bindValue(QStringLiteral(":new"), true);
        } else if (type == EpisodeModel::Unread) {
            entryQuery.prepare(QStringLiteral("SELECT id FROM Entries WHERE read=:read ORDER BY updated DESC LIMIT 1 OFFSET :index;"));
            entryQuery.bindValue(QStringLiteral(":read"), false);
        } else if (type == EpisodeModel::All) {
            entryQuery.prepare(QStringLiteral("SELECT id FROM Entries ORDER BY updated DESC LIMIT 1 OFFSET :index;"));
        } else { // i.e. EpisodeModel::Downloaded
            entryQuery.prepare(QStringLiteral("SELECT * FROM Enclosures INNER JOIN Entries ON Enclosures.id = Entries.id WHERE downloaded=:downloaded ORDER BY updated DESC LIMIT 1 OFFSET :index;"));
            entryQuery.bindValue(QStringLiteral(":downloaded"), true);
        }
        entryQuery.bindValue(QStringLiteral(":index"), entry_index);
        Database::instance().execute(entryQuery);
        if (!entryQuery.next()) {
            qWarning() << "No element with index" << entry_index << "found";
            return nullptr;
        }
        QString id = entryQuery.value(QStringLiteral("id")).toString();
        return getEntry(id);

    }
    return nullptr;
}

int DataManager::feedCount() const
{
    return m_feedmap.count();
}

int DataManager::entryCount(const int feed_index) const
{
    return m_entrymap[m_feedmap[feed_index]].count();
}

int DataManager::entryCount(const Feed* feed) const
{
    return m_entrymap[feed->url()].count();
}

int DataManager::entryCount(const EpisodeModel::Type type) const
{
    QSqlQuery query;
    if (type == EpisodeModel::All || type == EpisodeModel::New || type == EpisodeModel::Unread || type == EpisodeModel::Downloaded) {
        if (type == EpisodeModel::New) {
            query.prepare(QStringLiteral("SELECT COUNT (id) FROM Entries WHERE new=:new;"));
            query.bindValue(QStringLiteral(":new"), true);
        } else if (type == EpisodeModel::Unread) {
            query.prepare(QStringLiteral("SELECT COUNT (id) FROM Entries WHERE read=:read;"));
            query.bindValue(QStringLiteral(":read"), false);
        } else if (type == EpisodeModel::All) {
            query.prepare(QStringLiteral("SELECT COUNT (id) FROM Entries;"));
        } else { // i.e. EpisodeModel::Downloaded
            query.prepare(QStringLiteral("SELECT COUNT (id) FROM Enclosures WHERE downloaded=:downloaded;"));
            query.bindValue(QStringLiteral(":downloaded"), true);
        }
        Database::instance().execute(query);
        if (!query.next())
            return -1;
        return query.value(0).toInt();
    }
    return -1;
}

int DataManager::unreadEntryCount(const Feed* feed) const
{
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT COUNT (id) FROM Entries where feed=:feed AND read=0;"));
    query.bindValue(QStringLiteral(":feed"), feed->url());
    Database::instance().execute(query);
    if (!query.next())
        return -1;
    return query.value(0).toInt();
}

int DataManager::newEntryCount(const Feed* feed) const
{
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT COUNT (id) FROM Entries where feed=:feed AND new=1;"));
    query.bindValue(QStringLiteral(":feed"), feed->url());
    Database::instance().execute(query);
    if (!query.next())
        return -1;
    return query.value(0).toInt();
}

void DataManager::removeFeed(Feed* feed)
{
    qDebug() << feed->url();
    qDebug() << "deleting feed with index" << m_feedmap.indexOf(feed->url());
    removeFeed(m_feedmap.indexOf(feed->url()));
}

void DataManager::removeFeed(const int &index)
{
    // Get feed pointer
    Feed* feed = getFeed(m_feedmap[index]);
    const QString feedurl = feed->url();

    // Delete the object instances and mappings
    // First delete entries in Queue
    qDebug() << "delete queueentries of" << feedurl;
    for (auto& id : m_queuemap) {
        if (getEntry(id)->feed()->url() == feedurl) {
            removeQueueItem(id);
        }
    }

    // Delete entries themselves
    qDebug() << "delete entries of" << feedurl;
    for (auto& id : m_entrymap[feedurl]) {
        if (getEntry(id)->hasEnclosure()) getEntry(id)->enclosure()->deleteFile(); // delete enclosure (if it exists)
        if (!getEntry(id)->image().isEmpty()) Fetcher::instance().removeImage(getEntry(id)->image()); // delete entry images
        delete m_entries[id]; // delete pointer
        m_entries.remove(id); // delete the hash key
    }
    m_entrymap.remove(feedurl); // remove all the entry mappings belonging to the feed

    qDebug() << "Remove feed image" << feed->image() << "for feed" << feedurl;
    if (!feed->image().isEmpty()) Fetcher::instance().removeImage(feed->image());
    m_feeds.remove(m_feedmap[index]); // remove from m_feeds
    m_feedmap.removeAt(index); // remove from m_feedmap
    delete feed; // remove the pointer

    // Then delete everything from the database
    qDebug() << "delete database part of" << feedurl;

    // Delete Authors
    QSqlQuery query;
    query.prepare(QStringLiteral("DELETE FROM Authors WHERE feed=:feed;"));
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

    Q_EMIT feedRemoved(index);
}

void DataManager::addFeed(const QString &url)
{
    addFeed(url, true);
}

void DataManager::addFeed(const QString &url, const bool fetch)
{
    // This method will add the relevant internal data structures, and then add
    // a preliminary entry into the database.  Those details (as well as entries,
    // authors and enclosures) will be updated by calling Fetcher::fetch() which
    // will trigger a full update of the feed and all related items.
    qDebug() << "Adding feed";
    if (feedExists(url)) {
        qDebug() << "Feed already exists";
        return;
    }
    qDebug() << "Feed does not yet exist";

    QUrl urlFromInput = QUrl::fromUserInput(url);
    QSqlQuery query;
    query.prepare(QStringLiteral("INSERT INTO Feeds VALUES (:name, :url, :image, :link, :description, :deleteAfterCount, :deleteAfterType, :subscribed, :lastUpdated, :new, :notify);"));
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

    Q_EMIT feedAdded(urlFromInput.toString());

    if (fetch) Fetcher::instance().fetch(urlFromInput.toString());
}

void DataManager::addFeeds(const QStringList &urls)
{
    if (urls.count() == 0) return;

    for (int i=0; i<urls.count(); i++) {
        addFeed(urls[i], false);  // add preliminary feed entries, but do not fetch yet
    }
    Fetcher::instance().fetch(urls);
}

Entry* DataManager::getQueueEntry(int const &index) const
{
    return getEntry(m_queuemap[index]);
}

int DataManager::queueCount() const
{
    return m_queuemap.count();
}

QStringList DataManager::getQueue() const
{
    return m_queuemap;
}

bool DataManager::entryInQueue(const Entry* entry)
{
    return entryInQueue(entry->feed()->url(), entry->id());
}

bool DataManager::entryInQueue(const QString &feedurl, const QString &id) const
{
    Q_UNUSED(feedurl);
    return m_queuemap.contains(id);
}

void DataManager::addToQueue(const Entry* entry)
{
    if (entry != nullptr) {
        return addToQueue(entry->feed()->url(), entry->id());
    }
}

void DataManager::addToQueue(const QString &feedurl, const QString &id)
{
    // If item is already in queue, then stop here
    if (m_queuemap.contains(id)) return;

    // Add to internal queuemap data structure
    m_queuemap += id;
    //qDebug() << m_queuemap;

    // Get index of this entry
    const int index = m_queuemap.indexOf(id); // add new entry to end of queue

    // Add to Queue database
    QSqlQuery query;
    query.prepare(QStringLiteral("INSERT INTO Queue VALUES (:index, :feedurl, :id, :playing);"));
    query.bindValue(QStringLiteral(":index"), index);
    query.bindValue(QStringLiteral(":feedurl"), feedurl);
    query.bindValue(QStringLiteral(":id"), id);
    query.bindValue(QStringLiteral(":playing"), false);
    Database::instance().execute(query);

    // Make sure that the QueueModel is aware of the changes
    Q_EMIT queueEntryAdded(index, id);
}

void DataManager::moveQueueItem(const int &from, const int &to)
{
    // First move the items in the internal data structure
    m_queuemap.move(from, to);

    // Then make sure that the database Queue table reflects these changes
    updateQueueListnrs();

    // Make sure that the QueueModel is aware of the changes so it can update
    Q_EMIT queueEntryMoved(from, to);
}

void DataManager::removeQueueItem(const int &index)
{
    //qDebug() << m_queuemap;
    // Unset "new" state
    getEntry(m_queuemap[index])->setNew(false);
    // TODO: Make sure to unset the pointer in the Audio class once it's been
    // ported to c++

    // Remove the item from the internal data structure
    const QString id = m_queuemap[index];
    m_queuemap.removeAt(index);

    // Then make sure that the database Queue table reflects these changes
    QSqlQuery query;
    query.prepare(QStringLiteral("DELETE FROM Queue WHERE listnr=:listnr;"));
    query.bindValue(QStringLiteral(":listnr"), index);
    Database::instance().execute(query);
    // ... and update all other listnrs in Queue table
    updateQueueListnrs();

    // Make sure that the QueueModel is aware of the change so it can update
    Q_EMIT queueEntryRemoved(index, id);
}

void DataManager::removeQueueItem(const QString id)
{
    removeQueueItem(m_queuemap.indexOf(id));
}

void DataManager::removeQueueItem(Entry* entry)
{
    removeQueueItem(m_queuemap.indexOf(entry->id()));
}

QString DataManager::lastPlayingEntry()
{
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT id FROM Queue WHERE playing=:playing;"));
    query.bindValue(QStringLiteral(":playing"), true);
    Database::instance().execute(query);
    if (!query.next()) return QStringLiteral("none");
    return query.value(QStringLiteral("id")).toString();
}

void DataManager::setLastPlayingEntry(const QString& id)
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

void DataManager::importFeeds(const QString &path)
{
    QUrl url(path);
    QFile file(url.isLocalFile() ? url.toLocalFile() : url.toString());

    file.open(QIODevice::ReadOnly);

    QStringList urls;
    QXmlStreamReader xmlReader(&file);
    while(!xmlReader.atEnd()) {
        xmlReader.readNext();
        if(xmlReader.tokenType() == 4 &&  xmlReader.attributes().hasAttribute(QStringLiteral("xmlUrl"))) {
            urls += xmlReader.attributes().value(QStringLiteral("xmlUrl")).toString();
        }
    }
    qDebug() << urls;
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
    while(query.next()) {
        xmlWriter.writeEmptyElement(QStringLiteral("outline"));
        xmlWriter.writeAttribute(QStringLiteral("xmlUrl"), query.value(0).toString());
        xmlWriter.writeAttribute(QStringLiteral("title"), query.value(1).toString());
    }
    xmlWriter.writeEndElement();
    xmlWriter.writeEndElement();
    xmlWriter.writeEndDocument();

}

void DataManager::loadFeed(const QString feedurl) const
{
    m_feeds[feedurl] = new Feed(feedurl);
}

void DataManager::loadEntry(const QString id) const
{
    // First find the feed that this entry belongs to
    Feed* feed = nullptr;
    QHashIterator<QString, QStringList> i(m_entrymap);
    while (i.hasNext()) {
        i.next();
        if (i.value().contains(id))
            feed = getFeed(i.key());
    }
    if (feed == nullptr) {
        qDebug() << "Failed to find feed belonging to entry" << id;
        return;
    }
    m_entries[id] = new Entry(feed, id);
}

bool DataManager::feedExists(const QString &url)
{
    return m_feeds.contains(url);
}

void DataManager::updateQueueListnrs() const
{
    QSqlQuery query;
    for (int i=0; i<m_queuemap.count(); i++) {
        query.prepare(QStringLiteral("UPDATE Queue SET listnr=:i WHERE id=:id;"));
        query.bindValue(QStringLiteral(":i"), i);
        query.bindValue(QStringLiteral(":id"), m_queuemap[i]);
        Database::instance().execute(query);
    }
}
