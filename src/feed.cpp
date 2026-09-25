/*
 * SPDX-FileCopyrightText: 2020 Tobias Fella <tobias.fella@kde.org>
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include <QVariant>

#include <KLocalizedString>
#include <qtpreprocessorsupport.h>

#include "database.h"
#include "datamanager.h"
#include "feed.h"
#include "fetcher.h"
#include "objectslogging.h"
#include "utils/entryutils.h"

Feed::Feed(const qint64 feeduid, QObject *parent)
    : QObject(parent)
    , m_feeduid(feeduid)
    , m_isSubscribed(true)
{
    qCDebug(kastsObjects) << "Feed object" << m_feeduid << "constructed";

    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT * FROM Feeds WHERE feeduid=:feeduid;"));
    query.bindValue(QStringLiteral(":feeduid"), feeduid);
    Database::instance().execute(query);
    if (!query.next())
        qWarning() << "Failed to load feed" << feeduid;

    m_feeduid = query.value(QStringLiteral("feeduid")).toLongLong();
    m_name = query.value(QStringLiteral("name")).toString();
    m_url = query.value(QStringLiteral("url")).toString();
    m_image = query.value(QStringLiteral("image")).toString();
    m_link = query.value(QStringLiteral("link")).toString();
    m_description = query.value(QStringLiteral("description")).toString();
    m_subscribed.setSecsSinceEpoch(query.value(QStringLiteral("subscribed")).toLongLong());
    m_lastUpdated.setSecsSinceEpoch(query.value(QStringLiteral("lastUpdated")).toLongLong());
    m_dirname = query.value(QStringLiteral("dirname")).toString();

    QStringList authors;
    query.prepare(QStringLiteral("SELECT name FROM FeedAuthors WHERE feeduid=:feeduid"));
    query.bindValue(QStringLiteral(":feeduid"), m_feeduid);
    Database::instance().execute(query);
    while (query.next()) {
        authors += query.value(QStringLiteral("name")).toString();
    }
    m_authors = EntryUtils::combineAuthors(authors);

    connect(&Fetcher::instance(), &Fetcher::feedUpdated, this, [this](const qint64 feeduid) {
        if (feeduid == m_feeduid) {
            Q_EMIT entryCountChanged();
            Q_EMIT unreadCountChanged();
            Q_EMIT newCountChanged();
        }
    });
    connect(&Fetcher::instance(), &Fetcher::feedDetailsUpdated, this, [this](const qint64 feeduid) {
        if (feeduid == m_feeduid) {
            updateFeed();
        }
    });
    connect(&DataManager::instance(), &DataManager::unreadEntryCountChanged, this, [this](const qint64 feeduid) {
        if (feeduid == m_feeduid) {
            Q_EMIT unreadCountChanged();
        }
    });
    connect(&DataManager::instance(), &DataManager::newEntryCountChanged, this, [this](const qint64 feeduid) {
        if (feeduid == m_feeduid) {
            Q_EMIT newCountChanged();
        }
    });
    connect(&DataManager::instance(), &DataManager::favoriteEntryCountChanged, this, [this](const qint64 feeduid) {
        if (feeduid == m_feeduid) {
            Q_EMIT favoriteCountChanged();
        }
    });
    connect(&Fetcher::instance(), &Fetcher::feedUpdateStatusChanged, this, [this](const qint64 feeduid, bool status) {
        if (feeduid == m_feeduid) {
            m_refreshing = status;
            Q_EMIT refreshingChanged(m_refreshing);
        }
    });
}

Feed::Feed(const QString &url,
           const QString &name,
           const QString &image,
           const QString &link,
           const QString &description,
           const QString &authors,
           const QDateTime &lastUpdated,
           QObject *parent)
    : QObject(parent)
    , m_feeduid(0)
    , m_url(url)
    , m_name(name)
    , m_image(image)
    , m_link(link)
    , m_description(description)
    , m_authors(authors)
    , m_lastUpdated(lastUpdated)
    , m_isSubscribed(false)
{
    qCDebug(kastsObjects) << "Feed object" << m_name << "constructed (non-subscribed)";
}

Feed::~Feed()
{
    if (m_isSubscribed) {
        qCDebug(kastsObjects) << "Feed object" << m_feeduid << "destructed";
    } else {
        qCDebug(kastsObjects) << "Feed object" << m_name << "destructed (non-subscribed)";
    }
}

void Feed::updateFeed()
{
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT * FROM Feeds WHERE feeduid=:feeduid;"));
    query.bindValue(QStringLiteral(":feeduid"), m_feeduid);
    Database::instance().execute(query);
    if (!query.next())
        qWarning() << "Failed to load feed" << m_feeduid;

    QString name = query.value(QStringLiteral("name")).toString();
    QString url = query.value(QStringLiteral("url")).toString();
    QString image = query.value(QStringLiteral("image")).toString();
    QString link = query.value(QStringLiteral("link")).toString();
    QString description = query.value(QStringLiteral("description")).toString();
    QDateTime subscribed = QDateTime::fromSecsSinceEpoch(query.value(QStringLiteral("subscribed")).toLongLong());
    QDateTime lastUpdated = QDateTime::fromSecsSinceEpoch(query.value(QStringLiteral("lastUpdated")).toLongLong());
    QString dirname = query.value(QStringLiteral("dirname")).toString();

    QStringList authorList;
    query.prepare(QStringLiteral("SELECT name FROM FeedAuthors WHERE feeduid=:feeduid"));
    query.bindValue(QStringLiteral(":feeduid"), m_feeduid);
    Database::instance().execute(query);
    while (query.next()) {
        authorList += query.value(QStringLiteral("name")).toString();
    }
    QString authors = EntryUtils::combineAuthors(authorList);

    // send signals if needed
    if (name != m_name) {
        m_name = name;
        Q_EMIT nameChanged(m_name);
    }
    if (url != m_url) {
        m_url = url;
        Q_EMIT urlChanged(m_url);
    }
    if (image != m_image) {
        m_image = image;
        Q_EMIT imageChanged(m_image);
    }
    if (link != m_link) {
        m_link = link;
        Q_EMIT linkChanged(m_link);
    }
    if (description != m_description) {
        m_description = description;
        Q_EMIT descriptionChanged(m_description);
    }
    if (subscribed != m_subscribed) {
        m_subscribed = subscribed;
    }
    if (lastUpdated != m_lastUpdated) {
        m_lastUpdated = lastUpdated;
        Q_EMIT lastUpdatedChanged(m_lastUpdated);
    }
    if (dirname != m_dirname) {
        m_dirname = dirname;
        Q_EMIT dirnameChanged(m_dirname);
    }
    if (authors != m_authors) {
        m_authors = authors;
        Q_EMIT authorsChanged(m_authors);
    }
}

qint64 Feed::entryCount() const
{
    if (m_isSubscribed) {
        QSqlQuery query;
        query.prepare(QStringLiteral("SELECT COUNT (id) FROM Entries WHERE feeduid=:feeduid;"));
        query.bindValue(QStringLiteral(":feeduid"), m_feeduid);
        Database::instance().execute(query);
        if (!query.next())
            return -1;
        return query.value(0).toInt();
    } else {
        return 0;
    }
}

qint64 Feed::unreadCount() const
{
    if (m_isSubscribed) {
        QSqlQuery query;
        query.prepare(QStringLiteral("SELECT COUNT (id) FROM Entries WHERE feeduid=:feeduid AND read=0;"));
        query.bindValue(QStringLiteral(":feeduid"), m_feeduid);
        Database::instance().execute(query);
        if (!query.next())
            return -1;
        return query.value(0).toInt();
    } else {
        return 0;
    }
}

qint64 Feed::newCount() const
{
    if (m_isSubscribed) {
        QSqlQuery query;
        query.prepare(QStringLiteral("SELECT COUNT (id) FROM Entries where feeduid=:feeduid AND new=1;"));
        query.bindValue(QStringLiteral(":feeduid"), m_feeduid);
        Database::instance().execute(query);
        if (!query.next())
            return -1;
        return query.value(0).toInt();
    } else {
        return 0;
    }
}

qint64 Feed::favoriteCount() const
{
    if (m_isSubscribed) {
        QSqlQuery query;
        query.prepare(QStringLiteral("SELECT COUNT (id) FROM Entries where feeduid=:feeduid AND favorite=1;"));
        query.bindValue(QStringLiteral(":feeduid"), m_feeduid);
        Database::instance().execute(query);
        if (!query.next())
            return -1;
        return query.value(0).toInt();
    } else {
        return 0;
    }
}
