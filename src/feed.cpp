/*
 * Copyright 2020 Tobias Fella <fella@posteo.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <QVariant>

#include "database.h"
#include "feed.h"
#include "fetcher.h"

Feed::Feed(QString url,
           QString name,
           QString image,
           QString link,
           QString description,
           QVector<Author *> authors,
           int deleteAfterCount,
           int deleteAfterType,
           QDateTime subscribed,
           QDateTime lastUpdated,
           int autoUpdateCount,
           int autoUpdateType,
           bool notify,
           QObject *parent)
    : QObject(parent)
    , m_url(url)
    , m_name(name)
    , m_image(image)
    , m_link(link)
    , m_description(description)
    , m_authors(authors)
    , m_deleteAfterCount(deleteAfterCount)
    , m_deleteAfterType(deleteAfterType)
    , m_subscribed(subscribed)
    , m_lastUpdated(lastUpdated)
    , m_autoUpdateCount(autoUpdateCount)
    , m_autoUpdateType(autoUpdateType)
    , m_notify(notify)
{
    connect(&Fetcher::instance(), &Fetcher::startedFetchingFeed, this, [this](QString url) {
        if (url == m_url) {
            setRefreshing(true);
        }
    });
    connect(&Fetcher::instance(), &Fetcher::feedUpdated, this, [this](QString url) {
        if (url == m_url) {
            setRefreshing(false);
            emit entryCountChanged();
            emit unreadEntryCountChanged();
        }
    });
}

Feed::~Feed()
{
}

QString Feed::url() const
{
    return m_url;
}

QString Feed::name() const
{
    return m_name;
}

QString Feed::image() const
{
    return m_image;
}

QString Feed::link() const
{
    return m_link;
}

QString Feed::description() const
{
    return m_description;
}

QVector<Author *> Feed::authors() const
{
    return m_authors;
}

int Feed::deleteAfterCount() const
{
    return m_deleteAfterCount;
}

int Feed::deleteAfterType() const
{
    return m_deleteAfterType;
}

QDateTime Feed::subscribed() const
{
    return m_subscribed;
}

QDateTime Feed::lastUpdated() const
{
    return m_lastUpdated;
}

int Feed::autoUpdateCount() const
{
    return m_autoUpdateCount;
}

int Feed::autoUpdateType() const
{
    return m_autoUpdateType;
}

bool Feed::notify() const
{
    return m_notify;
}

int Feed::entryCount() const
{
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT COUNT (id) FROM Entries where feed=:feed;"));
    query.bindValue(QStringLiteral(":feed"), m_url);
    Database::instance().execute(query);
    if (!query.next())
        return -1;
    return query.value(0).toInt();
}

int Feed::unreadEntryCount() const
{
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT COUNT (id) FROM Entries where feed=:feed AND read=false;"));
    query.bindValue(QStringLiteral(":feed"), m_url);
    Database::instance().execute(query);
    if (!query.next())
        return -1;
    return query.value(0).toInt();
}

bool Feed::refreshing() const
{
    return m_refreshing;
}

void Feed::setName(QString name)
{
    m_name = name;
    Q_EMIT nameChanged(m_name);
}

void Feed::setImage(QString image)
{
    m_image = image;
    Q_EMIT imageChanged(m_image);
}

void Feed::setLink(QString link)
{
    m_link = link;
    Q_EMIT linkChanged(m_link);
}

void Feed::setDescription(QString description)
{
    m_description = description;
    Q_EMIT descriptionChanged(m_description);
}

void Feed::setAuthors(QVector<Author *> authors)
{
    m_authors = authors;
    Q_EMIT authorsChanged(m_authors);
}

void Feed::setDeleteAfterCount(int count)
{
    m_deleteAfterCount = count;
    Q_EMIT deleteAfterCountChanged(m_deleteAfterCount);
}

void Feed::setDeleteAfterType(int type)
{
    m_deleteAfterType = type;
    Q_EMIT deleteAfterTypeChanged(m_deleteAfterType);
}

void Feed::setLastUpdated(QDateTime lastUpdated)
{
    m_lastUpdated = lastUpdated;
    Q_EMIT lastUpdatedChanged(m_lastUpdated);
}

void Feed::setAutoUpdateCount(int count)
{
    m_autoUpdateCount = count;
    Q_EMIT autoUpdateCountChanged(m_autoUpdateCount);
}

void Feed::setAutoUpdateType(int type)
{
    m_autoUpdateType = type;
    Q_EMIT autoUpdateTypeChanged(m_autoUpdateType);
}

void Feed::setNotify(bool notify)
{
    m_notify = notify;
    Q_EMIT notifyChanged(m_notify);
}

void Feed::setRefreshing(bool refreshing)
{
    m_refreshing = refreshing;
    Q_EMIT refreshingChanged(m_refreshing);
}

void Feed::refresh()
{
    Fetcher::instance().fetch(m_url);
}

void Feed::remove()
{
    // Delete Authors
    QSqlQuery query;
    query.prepare(QStringLiteral("DELETE FROM Authors WHERE feed=:feed;"));
    query.bindValue(QStringLiteral(":feed"), m_url);
    Database::instance().execute(query);

    // Delete Entries
    query.prepare(QStringLiteral("DELETE FROM Entries WHERE feed=:feed;"));
    query.bindValue(QStringLiteral(":feed"), m_url);
    Database::instance().execute(query);

    // TODO Delete Enclosures

    // Delete Feed
    query.prepare(QStringLiteral("DELETE FROM Feeds WHERE url=:url;"));
    query.bindValue(QStringLiteral(":url"), m_url);
    Database::instance().execute(query);
}
