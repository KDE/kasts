/*
 * SPDX-FileCopyrightText: 2020 Tobias Fella <tobias.fella@kde.org>
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include <QVariant>

#include <KLocalizedString>

#include "database.h"
#include "datamanager.h"
#include "feed.h"
#include "fetcher.h"
#include "models/abstractepisodeproxymodel.h"
#include "models/unsubscribedentriesproxymodel.h"
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

    connectSignals();
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
    qCDebug(kastsObjects) << "Feed object" << m_name << "constructed (non-subscribed)" << this;

    // if we subscribe to this feed, then we need to switch to the data in the database
    connect(&Fetcher::instance(), &Fetcher::feedDetailsUpdated, this, [this](const qint64 feeduid, const QString &url) {
        if (url == m_url) {
            if (feeduid > 0) {
                m_feeduid = feeduid;

                // disconnect the signals before connecting the signals for a subscribed feed
                disconnect(&Fetcher::instance(), nullptr, this, nullptr);

                // now update everything to what's in the database
                connectSignals();
                updateFeed();
                m_isSubscribed = true;

                // replace the entriesProxyModel
                if (m_entriesProxyModel) {
                    delete m_entriesProxyModel;
                    m_entriesProxyModel = entriesProxyModel();
                }

                Q_EMIT isSubscribedChanged(m_isSubscribed);
                Q_EMIT feeduidChanged(feeduid);
                Q_EMIT entriesProxyModelChanged(m_entriesProxyModel);
            }
        }
    });
}

Feed::~Feed()
{
    if (m_isSubscribed) {
        qCDebug(kastsObjects) << "Feed object" << m_feeduid << "destructed" << this;
    } else {
        qCDebug(kastsObjects) << "Feed object" << m_name << "destructed (non-subscribed)" << this;
    }
}

void Feed::connectSignals()
{
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
        return m_entryCount;
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

AbstractEpisodeProxyModel *Feed::entriesProxyModel()
{
    if (!m_entriesProxyModel) {
        if (m_isSubscribed) {
            m_entriesProxyModel = new EntriesProxyModel(m_feeduid, this);
        } else {
            UnsubscribedEntriesProxyModel *entriesProxyModel = new UnsubscribedEntriesProxyModel(m_url, this);
            m_entriesProxyModel = entriesProxyModel;

            // when the full feed has been retrieved and parsed, we can update the
            // data from there; it will contain more accurate data than what's retrieved
            // from the search APIs
            UnsubscribedEntriesModel *entriesModel = qobject_cast<UnsubscribedEntriesModel *>(m_entriesProxyModel->sourceModel());
            if (entriesModel) {
                connect(entriesModel,
                        &UnsubscribedEntriesModel::feedDetailsRetrieved,
                        this,
                        [this](const QString &url, const DataTypes::FeedUpdateDetails &feedDetails) {
                            if (url == m_url) {
                                if (m_url != feedDetails.url) {
                                    m_url = feedDetails.url;
                                    Q_EMIT urlChanged(m_url);
                                }
                                if (m_name != feedDetails.name) {
                                    m_name = feedDetails.name;
                                    Q_EMIT nameChanged(m_name);
                                }
                                if (m_image != feedDetails.image) {
                                    m_image = feedDetails.image;
                                    Q_EMIT imageChanged(m_image);
                                }
                                if (m_link != feedDetails.link) {
                                    m_link = feedDetails.link;
                                    Q_EMIT linkChanged(m_link);
                                }
                                if (m_description != feedDetails.description) {
                                    m_description = feedDetails.description;
                                    Q_EMIT descriptionChanged(m_description);
                                }
                                if (m_authors != EntryUtils::combineAuthors(feedDetails.authors.keys())) {
                                    m_authors = EntryUtils::combineAuthors(feedDetails.authors.keys());
                                    Q_EMIT authorsChanged(m_authors);
                                }
                                if (m_lastUpdated != QDateTime::fromSecsSinceEpoch(feedDetails.lastUpdated)) {
                                    m_lastUpdated = QDateTime::fromSecsSinceEpoch(feedDetails.lastUpdated);
                                    Q_EMIT lastUpdatedChanged(m_lastUpdated);
                                }
                                if (m_entryCount != feedDetails.entryOrder.count()) {
                                    m_entryCount = feedDetails.entryOrder.count();
                                    Q_EMIT entryCountChanged();
                                }
                            }
                        });
            }
        }
    } else {
        m_entriesProxyModel = nullptr;
    }
    return m_entriesProxyModel;
}
