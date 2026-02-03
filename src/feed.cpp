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
#include "error.h"
#include "feed.h"
#include "feedlogging.h"
#include "fetcher.h"
#include "models/abstractepisodeproxymodel.h"
#include "objectslogging.h"

Feed::Feed(const qint64 feeduid, QObject *parent)
    : QObject(&DataManager::instance()) // TODO: remove explicit parenting after refactor
    , m_feeduid(feeduid)
{
    Q_UNUSED(parent)

    qCDebug(kastsObjects) << "Feed object" << m_feeduid << "constructed";

    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT * FROM Feeds WHERE feeduid=:feeduid;"));
    query.bindValue(QStringLiteral(":feeduid"), feeduid);
    Database::instance().execute(query);
    if (!query.next())
        qWarning() << "Failed to load feed" << feeduid;

    m_subscribed.setSecsSinceEpoch(query.value(QStringLiteral("subscribed")).toInt());

    m_lastUpdated.setSecsSinceEpoch(query.value(QStringLiteral("lastUpdated")).toInt());

    m_feeduid = query.value(QStringLiteral("feeduid")).toLongLong();
    m_url = query.value(QStringLiteral("url")).toString();
    m_name = query.value(QStringLiteral("name")).toString();
    m_image = query.value(QStringLiteral("image")).toString();
    m_link = query.value(QStringLiteral("link")).toString();
    m_description = query.value(QStringLiteral("description")).toString();
    int filterTypeValue = query.value(QStringLiteral("filterType")).toInt();
    int sortTypeValue = query.value(QStringLiteral("sortType")).toInt();
    m_dirname = query.value(QStringLiteral("dirname")).toString();

    m_errorId = 0;
    m_errorString = QLatin1String("");

    updateAuthors();
    updateUnreadEntryCountFromDB();
    updateNewEntryCountFromDB();
    updateFavoriteEntryCountFromDB();

    connect(&Fetcher::instance(), &Fetcher::feedUpdateStatusChanged, this, [this](const QString &url, bool status) {
        if (url == m_url) {
            setRefreshing(status);
        }
    });
    connect(&DataManager::instance(), &DataManager::feedEntriesUpdated, this, [this](const qint64 feeduid) {
        if (feeduid == m_feeduid) {
            Q_EMIT entryCountChanged();
            updateUnreadEntryCountFromDB();
            Q_EMIT DataManager::instance().unreadEntryCountChanged(m_feeduid);
            Q_EMIT unreadEntryCountChanged();
            Q_EMIT DataManager::instance().newEntryCountChanged(m_feeduid);
            Q_EMIT newEntryCountChanged();
            setErrorId(0);
            setErrorString(QLatin1String(""));
        }
    });
    connect(&DataManager::instance(), &DataManager::newEntryCountChanged, this, [this](const qint64 feeduid) {
        if (feeduid == m_feeduid) {
            updateNewEntryCountFromDB();
            Q_EMIT newEntryCountChanged();
        }
    });
    connect(&DataManager::instance(), &DataManager::favoriteEntryCountChanged, this, [this](const qint64 feeduid) {
        if (feeduid == m_feeduid) {
            updateFavoriteEntryCountFromDB();
            Q_EMIT favoriteEntryCountChanged();
        }
    });
    connect(&Fetcher::instance(),
            &Fetcher::error,
            this,
            [this](const Error::Type type, const QString &url, const QString &id, int errorId, const QString &errorString) {
                Q_UNUSED(type)
                Q_UNUSED(id)
                if (url == m_url) {
                    setErrorId(errorId);
                    setErrorString(errorString);
                    setRefreshing(false);
                }
            });
    connect(&Fetcher::instance(), &Fetcher::downloadFinished, this, [this](QString url) {
        if (url == m_image) {
            Q_EMIT imageChanged(url);
            Q_EMIT cachedImageChanged(cachedImage());
        }
    });

    m_entries = new EntriesProxyModel(m_feeduid, this);

    initFilterType(filterTypeValue);

    QTimer::singleShot(0, this, [this, sortTypeValue]() {
        initSortType(sortTypeValue);
    });
}

Feed::~Feed()
{
    qCDebug(kastsObjects) << "Feed object" << m_feeduid << "destructed";
}

void Feed::updateAuthors()
{
    QStringList authors;

    QSqlQuery authorQuery;
    authorQuery.prepare(QStringLiteral("SELECT name FROM FeedAuthors WHERE feeduid=:feeduid"));
    authorQuery.bindValue(QStringLiteral(":feeduid"), m_feeduid);
    Database::instance().execute(authorQuery);
    while (authorQuery.next()) {
        authors += authorQuery.value(QStringLiteral("name")).toString();
    }

    if (authors.size() == 1) {
        m_authors = authors[0];
    } else if (authors.size() == 2) {
        m_authors = i18nc("<name> and <name>", "%1 and %2", authors.first(), authors.last());
    } else if (authors.size() > 2) {
        auto last = authors.takeLast();
        m_authors = i18nc("<name(s)>, and <name>", "%1, and %2", authors.join(u','), last);
    }
    Q_EMIT authorsChanged(m_authors);
}

void Feed::updateUnreadEntryCountFromDB()
{
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT COUNT (id) FROM Entries WHERE feeduid=:feeduid AND read=0;"));
    query.bindValue(QStringLiteral(":feeduid"), m_feeduid);
    Database::instance().execute(query);
    if (!query.next())
        m_unreadEntryCount = -1;
    m_unreadEntryCount = query.value(0).toInt();
}

void Feed::updateNewEntryCountFromDB()
{
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT COUNT (id) FROM Entries where feeduid=:feeduid AND new=1;"));
    query.bindValue(QStringLiteral(":feeduid"), m_feeduid);
    Database::instance().execute(query);
    if (!query.next())
        m_newEntryCount = -1;
    m_newEntryCount = query.value(0).toInt();
}

void Feed::updateFavoriteEntryCountFromDB()
{
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT COUNT (id) FROM Entries where feeduid=:feeduid AND favorite=1;"));
    query.bindValue(QStringLiteral(":feeduid"), m_feeduid);
    Database::instance().execute(query);
    if (!query.next())
        m_favoriteEntryCount = -1;
    m_favoriteEntryCount = query.value(0).toInt();
}

void Feed::initFilterType(int value)
{
    // restore saved filter
    AbstractEpisodeProxyModel::FilterType filterType = AbstractEpisodeProxyModel::FilterType(value);
    if (filterType != m_entries->filterType()) {
        m_entries->setFilterType(filterType);
    }

    // save filter to db when changed
    connect(m_entries, &EntriesProxyModel::filterTypeChanged, this, [this]() {
        int filterTypeValue = static_cast<int>(m_entries->filterType());

        QSqlQuery writeQuery;
        writeQuery.prepare(QStringLiteral("UPDATE Feeds SET filterType=:filterType WHERE feeduid=:feeduid;"));
        writeQuery.bindValue(QStringLiteral(":feeduid"), m_feeduid);
        writeQuery.bindValue(QStringLiteral(":filterType"), filterTypeValue);
        Database::instance().execute(writeQuery);
    });
}

void Feed::initSortType(int value)
{
    // restore saved sorting
    AbstractEpisodeProxyModel::SortType sortType = AbstractEpisodeProxyModel::SortType(value);
    if (sortType != m_entries->sortType()) {
        m_entries->setSortType(sortType);
    }

    // save sort to db when changed
    connect(m_entries, &EntriesProxyModel::sortTypeChanged, this, [this]() {
        int sortTypeValue = static_cast<int>(m_entries->sortType());

        QSqlQuery writeQuery;
        writeQuery.prepare(QStringLiteral("UPDATE Feeds SET sortType=:sortType WHERE feeduid=:feeduid;"));
        writeQuery.bindValue(QStringLiteral(":feeduid"), m_feeduid);
        writeQuery.bindValue(QStringLiteral(":sortType"), sortTypeValue);
        Database::instance().execute(writeQuery);
    });
}

qint64 Feed::feeduid() const
{
    return m_feeduid;
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

QString Feed::cachedImage() const
{
    return Fetcher::instance().image(m_image);
}

QString Feed::link() const
{
    return m_link;
}

QString Feed::description() const
{
    return m_description;
}

QString Feed::authors() const
{
    return m_authors;
}

QDateTime Feed::subscribed() const
{
    return m_subscribed;
}

QDateTime Feed::lastUpdated() const
{
    return m_lastUpdated;
}

QString Feed::dirname() const
{
    return m_dirname;
}

int Feed::unreadEntryCount() const
{
    return m_unreadEntryCount;
}

int Feed::newEntryCount() const
{
    return m_newEntryCount;
}

int Feed::favoriteEntryCount() const
{
    return m_favoriteEntryCount;
}

bool Feed::refreshing() const
{
    return m_refreshing;
}

int Feed::errorId() const
{
    return m_errorId;
}

QString Feed::errorString() const
{
    return m_errorString;
}

void Feed::setName(const QString &name)
{
    if (name != m_name) {
        m_name = name;
        Q_EMIT nameChanged(m_name);
    }
}

void Feed::setImage(const QString &image)
{
    if (image != m_image) {
        m_image = image;
        Q_EMIT imageChanged(m_image);
        Q_EMIT cachedImageChanged(cachedImage());
    }
}

void Feed::setLink(const QString &link)
{
    if (link != m_link) {
        m_link = link;
        Q_EMIT linkChanged(m_link);
    }
}

void Feed::setDescription(const QString &description)
{
    if (description != m_description) {
        m_description = description;
        Q_EMIT descriptionChanged(m_description);
    }
}

void Feed::setLastUpdated(const QDateTime &lastUpdated)
{
    if (lastUpdated != m_lastUpdated) {
        m_lastUpdated = lastUpdated;
        Q_EMIT lastUpdatedChanged(m_lastUpdated);
    }
}

void Feed::setDirname(const QString &dirname)
{
    if (dirname != m_dirname) {
        m_dirname = dirname;
        Q_EMIT dirnameChanged(m_dirname);
    }
}

void Feed::setUnreadEntryCount(const int count)
{
    if (count != m_unreadEntryCount) {
        m_unreadEntryCount = count;
        Q_EMIT unreadEntryCountChanged();
        Q_EMIT DataManager::instance().unreadEntryCountChanged(m_feeduid);
        // TODO: can one of the two slots be removed??
    }
}

void Feed::setRefreshing(bool refreshing)
{
    if (refreshing != m_refreshing) {
        m_refreshing = refreshing;
        if (!m_refreshing) {
            m_errorId = 0;
            m_errorString = QString();
        }
        Q_EMIT refreshingChanged(m_refreshing);
    }
}

void Feed::setErrorId(int errorId)
{
    if (errorId != m_errorId) {
        m_errorId = errorId;
        Q_EMIT errorIdChanged(m_errorId);
    }
}

void Feed::setErrorString(const QString &errorString)
{
    if (errorString != m_errorString) {
        m_errorString = errorString;
        Q_EMIT errorStringChanged(m_errorString);
    }
}

void Feed::refresh()
{
    Fetcher::instance().fetch(m_url);
}
