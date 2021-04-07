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

DataManager::DataManager()
{
    // connect signals to lambda slots
    connect(&Fetcher::instance(), &Fetcher::feedDetailsUpdated, this, [this](const QString &url, const QString &name, const QString &image, const QString &link, const QString &description, const QDateTime &lastUpdated) {
        m_feeds[url]->setName(name);
        m_feeds[url]->setImage(image);
        m_feeds[url]->setLink(link);
        m_feeds[url]->setDescription(description);
        m_feeds[url]->setLastUpdated(lastUpdated);
        // TODO: signal feedmodel: Q_EMIT dataChanged(createIndex(i, 0), createIndex(i, 0));
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
        Q_EMIT feedEntriesUpdated(feedurl);
    });

    // Only read unique feedurls and entry ids from the database.
    // The feed and entry datastructures will be loaded lazily.
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT url FROM Feeds;"));
    Database::instance().execute(query);
    while (query.next()) {
        m_feedmap += query.value(QStringLiteral("url")).toString();
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

void DataManager::removeFeed(const Feed* feed)
{
    removeFeed(m_feedmap.indexOf(feed->url()));
}

void DataManager::removeFeed(const int &index)
{
    // Get feed pointer
    Feed* feed = m_feeds[m_feedmap[index]];
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
        delete m_entries[id]; // delete pointer
        m_entries.remove(id); // delete the hash key
    }
    m_entrymap.remove(feedurl); // remove all the entry mappings belonging to the feed

    qDebug() << "Remove image" << feed->image() << "for feed" << feedurl;
    if (!feed->image().isEmpty()) Fetcher::instance().removeImage(feed->image());
    delete feed; // remove the pointer
    m_feeds.remove(m_feedmap[index]); // remove from m_feeds
    m_feedmap.removeAt(index); // remove from m_feedmap

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
    qDebug() << "Adding feed";
    if (feedExists(url)) {
        qDebug() << "Feed already exists";
        return;
    }
    qDebug() << "Feed does not yet exist";

    QUrl urlFromInput = QUrl::fromUserInput(url);
    QSqlQuery query;
    query.prepare(QStringLiteral("INSERT INTO Feeds VALUES (:name, :url, :image, :link, :description, :deleteAfterCount, :deleteAfterType, :subscribed, :lastUpdated, :notify);"));
    query.bindValue(QStringLiteral(":name"), urlFromInput.toString());
    query.bindValue(QStringLiteral(":url"), urlFromInput.toString());
    query.bindValue(QStringLiteral(":image"), QLatin1String(""));
    query.bindValue(QStringLiteral(":link"), QLatin1String(""));
    query.bindValue(QStringLiteral(":description"), QLatin1String(""));
    query.bindValue(QStringLiteral(":deleteAfterCount"), 0);
    query.bindValue(QStringLiteral(":deleteAfterType"), 0);
    query.bindValue(QStringLiteral(":subscribed"), QDateTime::currentDateTime().toSecsSinceEpoch());
    query.bindValue(QStringLiteral(":lastUpdated"), 0);
    query.bindValue(QStringLiteral(":notify"), false);
    Database::instance().execute(query);

    m_feeds[urlFromInput.toString()] = new Feed(urlFromInput.toString());
    m_feedmap.append(urlFromInput.toString());

    Q_EMIT feedAdded(urlFromInput.toString());

    Fetcher::instance().fetch(urlFromInput.toString());
}

Entry* DataManager::getQueueEntry(int const &index) const
{
    return getEntry(m_queuemap[index]);
}

int DataManager::queueCount() const
{
    return m_queuemap.count();
}

void DataManager::addtoQueue(const QString &feedurl, const QString &id)
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
    query.prepare(QStringLiteral("INSERT INTO Queue VALUES (:index, :feedurl, :id);"));
    query.bindValue(QStringLiteral(":index"), index);
    query.bindValue(QStringLiteral(":feedurl"), feedurl);
    query.bindValue(QStringLiteral(":id"), id);
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
    // First remove the item from the internal data structure
    const QString id = m_queuemap[index];
    m_queuemap.removeAt(index);
    //qDebug() << m_queuemap;

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

void DataManager::importFeeds(const QString &path)
{
    QUrl url(path);
    QFile file(url.isLocalFile() ? url.toLocalFile() : url.toString());

    file.open(QIODevice::ReadOnly);

    QXmlStreamReader xmlReader(&file);
    while(!xmlReader.atEnd()) {
        xmlReader.readNext();
        if(xmlReader.tokenType() == 4 &&  xmlReader.attributes().hasAttribute(QStringLiteral("xmlUrl"))) {
            addFeed(xmlReader.attributes().value(QStringLiteral("xmlUrl")).toString());
        }
    }
    Fetcher::instance().fetchAll();
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
