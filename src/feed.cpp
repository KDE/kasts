/*
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include <QVariant>

#include "author.h"
#include "database.h"
#include "datamanager.h"
#include "error.h"
#include "feed.h"
#include "feedlogging.h"
#include "fetcher.h"

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

    m_errorId = 0;
    m_errorString = QLatin1String("");

    updateAuthors();
    updateUnreadEntryCountFromDB();

    connect(&Fetcher::instance(), &Fetcher::feedUpdateStatusChanged, this, [this](const QString &url, bool status) {
        if (url == m_url) {
            setRefreshing(status);
        }
    });
    connect(&DataManager::instance(), &DataManager::feedEntriesUpdated, this, [this](const QString &url) {
        if (url == m_url) {
            Q_EMIT entryCountChanged();
            updateUnreadEntryCountFromDB();
            Q_EMIT unreadEntryCountChanged();
            setErrorId(0);
            setErrorString(QLatin1String(""));
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
}

Feed::~Feed()
{
}

void Feed::updateAuthors()
{
    QVector<Author *> newAuthors;
    bool haveAuthorsChanged = false;

    QSqlQuery authorQuery;
    authorQuery.prepare(QStringLiteral("SELECT * FROM Authors WHERE id='' AND feed=:feed"));
    authorQuery.bindValue(QStringLiteral(":feed"), m_url);
    Database::instance().execute(authorQuery);
    while (authorQuery.next()) {
        // check if author already exists, if so, then reuse
        bool existingAuthor = false;
        QString name = authorQuery.value(QStringLiteral("name")).toString();
        QString email = authorQuery.value(QStringLiteral("email")).toString();
        QString url = authorQuery.value(QStringLiteral("uri")).toString();
        qCDebug(kastsFeed) << name << email << url;
        for (int i = 0; i < m_authors.count(); i++) {
            qCDebug(kastsFeed) << "old authors" << m_authors[i]->name() << m_authors[i]->email() << m_authors[i]->url();
            if (m_authors[i] && m_authors[i]->name() == name && m_authors[i]->email() == email && m_authors[i]->url() == url) {
                existingAuthor = true;
                newAuthors += m_authors[i];
            }
        }
        if (!existingAuthor) {
            newAuthors += new Author(name, email, url, nullptr);
            haveAuthorsChanged = true;
        }
    }

    // Finally check whether m_authors and newAuthors are identical
    // if not, then delete the authors that were removed
    for (int i = 0; i < m_authors.count(); i++) {
        if (!newAuthors.contains(m_authors[i])) {
            delete m_authors[i];
            haveAuthorsChanged = true;
        }
    }

    m_authors = newAuthors;

    if (haveAuthorsChanged)
        Q_EMIT authorsChanged(m_authors);
    qCDebug(kastsFeed) << "feed" << m_name << "authors have changed?" << haveAuthorsChanged;
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

bool Feed::notify() const
{
    return m_notify;
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
    return DataManager::instance().newEntryCount(this);
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

void Feed::setAuthors(const QVector<Author *> &authors)
{
    for (auto &author : m_authors) {
        delete author;
    }
    m_authors.clear();
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
