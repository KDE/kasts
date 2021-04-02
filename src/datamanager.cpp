/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */


#include "datamanager.h"
#include "fetcher.h"
#include "database.h"


DataManager::DataManager()
{
    // Only read unique feedurls and entry ids from the database.
    // The feed and entry datastructres will be loaded lazily.
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
        }
        qDebug() << m_entrymap[feedurl];
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

void DataManager::loadFeed(QString const feedurl) const
{
    m_feeds[feedurl] = new Feed(feedurl);
}
