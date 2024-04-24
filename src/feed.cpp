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
#include "error.h"
#include "feed.h"
#include "feedlogging.h"
#include "fetcher.h"
#include "models/abstractepisodeproxymodel.h"

Feed::Feed(const QString &feedurl)
    : QObject(&DataManager::instance())
{
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT * FROM Feeds WHERE url=:feedurl;"));
    query.bindValue(QStringLiteral(":feedurl"), feedurl);
    Database::instance().execute(query);
    if (!query.next())
        qWarning() << "Failed to load feed" << feedurl;

    m_subscribed.setSecsSinceEpoch(query.value(QStringLiteral("subscribed")).toInt());

    m_lastUpdated.setSecsSinceEpoch(query.value(QStringLiteral("lastUpdated")).toInt());

    m_url = query.value(QStringLiteral("url")).toString();
    m_name = query.value(QStringLiteral("name")).toString();
    m_image = query.value(QStringLiteral("image")).toString();
    m_link = query.value(QStringLiteral("link")).toString();
    m_description = query.value(QStringLiteral("description")).toString();
    m_deleteAfterCount = query.value(QStringLiteral("deleteAfterCount")).toInt();
    m_deleteAfterType = query.value(QStringLiteral("deleteAfterType")).toInt();
    m_notify = query.value(QStringLiteral("notify")).toBool();
    int filterTypeValue = query.value(QStringLiteral("filterType")).toInt();
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
    connect(&DataManager::instance(), &DataManager::feedEntriesUpdated, this, [this](const QString &url) {
        if (url == m_url) {
            Q_EMIT entryCountChanged();
            updateUnreadEntryCountFromDB();
            Q_EMIT DataManager::instance().unreadEntryCountChanged(m_url);
            Q_EMIT unreadEntryCountChanged();
            Q_EMIT DataManager::instance().newEntryCountChanged(m_url);
            Q_EMIT newEntryCountChanged();
            setErrorId(0);
            setErrorString(QLatin1String(""));
        }
    });
    connect(&DataManager::instance(), &DataManager::newEntryCountChanged, this, [this](const QString &url) {
        if (url == m_url) {
            updateNewEntryCountFromDB();
            Q_EMIT newEntryCountChanged();
        }
    });
    connect(&DataManager::instance(), &DataManager::favoriteEntryCountChanged, this, [this](const QString &url) {
        if (url == m_url) {
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

    m_entries = new EntriesProxyModel(this);

    // restore saved filter
    AbstractEpisodeProxyModel::FilterType filterType = AbstractEpisodeProxyModel::FilterType(filterTypeValue);
    if (filterType != m_entries->filterType()) {
        m_entries->setFilterType(filterType);
    }

    // save filter to db when changed
    connect(m_entries, &EntriesProxyModel::filterTypeChanged, this, [this]() {
        int filterTypeValue = static_cast<int>(m_entries->filterType());

        QSqlQuery writeQuery;
        writeQuery.prepare(QStringLiteral("UPDATE Feeds SET filterType=:filterType WHERE url=:feedurl;"));
        writeQuery.bindValue(QStringLiteral(":feedurl"), m_url);
        writeQuery.bindValue(QStringLiteral(":filterType"), filterTypeValue);
        Database::instance().execute(writeQuery);
    });
}

void Feed::updateAuthors()
{
    QStringList authors;

    QSqlQuery authorQuery;
    authorQuery.prepare(QStringLiteral("SELECT name FROM Authors WHERE id='' AND feed=:feed"));
    authorQuery.bindValue(QStringLiteral(":feed"), m_url);
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
    query.prepare(QStringLiteral("SELECT COUNT (id) FROM Entries where feed=:feed AND read=0;"));
    query.bindValue(QStringLiteral(":feed"), m_url);
    Database::instance().execute(query);
    if (!query.next())
        m_unreadEntryCount = -1;
    m_unreadEntryCount = query.value(0).toInt();
}

void Feed::updateNewEntryCountFromDB()
{
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT COUNT (id) FROM Entries where feed=:feed AND new=1;"));
    query.bindValue(QStringLiteral(":feed"), m_url);
    Database::instance().execute(query);
    if (!query.next())
        m_newEntryCount = -1;
    m_newEntryCount = query.value(0).toInt();
}

void Feed::updateFavoriteEntryCountFromDB()
{
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT COUNT (id) FROM Entries where feed=:feed AND favorite=1;"));
    query.bindValue(QStringLiteral(":feed"), m_url);
    Database::instance().execute(query);
    if (!query.next())
        m_favoriteEntryCount = -1;
    m_favoriteEntryCount = query.value(0).toInt();
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

bool Feed::notify() const
{
    return m_notify;
}

QString Feed::dirname() const
{
    return m_dirname;
}

int Feed::entryCount() const
{
    return DataManager::instance().entryCount(this);
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

void Feed::setLastUpdated(const QDateTime &lastUpdated)
{
    if (lastUpdated != m_lastUpdated) {
        m_lastUpdated = lastUpdated;
        Q_EMIT lastUpdatedChanged(m_lastUpdated);
    }
}

void Feed::setNotify(bool notify)
{
    if (notify != m_notify) {
        m_notify = notify;
        Q_EMIT notifyChanged(m_notify);
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
        Q_EMIT DataManager::instance().unreadEntryCountChanged(m_url);
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
