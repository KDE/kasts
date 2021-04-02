/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
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
    connect(&Fetcher::instance(), &Fetcher::feedUpdated, this, [this](const QString &url) {
        // TODO: make DataManager rescan entries
        Q_EMIT feedEntriesUpdated(url);
    });

    // Only read unique feedurls and entry ids from the database.
    // The feed and entry datastructures will be loaded lazily.
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT url FROM Feeds;"));
    Database::instance().execute(query);
    while (query.next()) {
        m_feedmap += query.value(QStringLiteral("url")).toString();
    }
    qDebug() << m_feedmap;

    for (auto &feedurl : m_feedmap) {
        query.prepare(QStringLiteral("SELECT id FROM Entries WHERE feed=:feed ORDER BY updated;"));
        query.bindValue(QStringLiteral(":feed"), feedurl);
        Database::instance().execute(query);
        while (query.next()) {
            m_entrymap[feedurl] += query.value(QStringLiteral("id")).toString();
            qDebug() << m_entrymap[feedurl];
        }
    }
    qDebug() << m_entrymap;
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

void DataManager::removeFeed(const Feed* feed)
{
    removeFeed(m_feedmap.indexOf(feed->url()));
}

void DataManager::removeFeed(const int &index)
{
    // Get feed pointer
    Feed* feed = m_feeds[m_feedmap[index]];

    // First delete everything from the database

    // Delete Authors
    QSqlQuery query;
    query.prepare(QStringLiteral("DELETE FROM Authors WHERE feed=:feed;"));
    query.bindValue(QStringLiteral(":feed"), feed->url());
    Database::instance().execute(query);

    // Delete Entries
    query.prepare(QStringLiteral("DELETE FROM Entries WHERE feed=:feed;"));
    query.bindValue(QStringLiteral(":feed"), feed->url());
    Database::instance().execute(query);

    // Delete Enclosures
    query.prepare(QStringLiteral("DELETE FROM Enclosures WHERE feed=:feed;"));
    query.bindValue(QStringLiteral(":feed"), feed->url());
    Database::instance().execute(query);

    // Delete Feed
    query.prepare(QStringLiteral("DELETE FROM Feeds WHERE url=:url;"));
    query.bindValue(QStringLiteral(":url"), feed->url());
    Database::instance().execute(query);

    // Then delete the instances and mappings
    for (auto& id : m_entrymap[feed->url()]) {
        delete m_entries[id]; // delete pointer
        m_entries.remove(id); // delete the hash key
    }
    m_entrymap.remove(feed->url()); // remove all the entry mappings belonging to the feed

    delete feed; // remove the pointer
    m_feeds.remove(m_feedmap[index]); // remove from m_feeds
    m_feedmap.removeAt(index); // remove from m_feedmap
    Q_EMIT(feedRemoved(index));
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
