/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "entry.h"

#include <QRegularExpression>
#include <QSqlQuery>
#include <QUrl>

#include "database.h"
#include "datamanager.h"
#include "feed.h"
#include "fetcher.h"
#include "settingsmanager.h"
#include "sync/sync.h"

Entry::Entry(Feed *feed, const QString &id)
    : QObject(&DataManager::instance())
    , m_feed(feed)
{
    connect(&Fetcher::instance(), &Fetcher::downloadFinished, this, [this](QString url) {
        if (url == m_image) {
            Q_EMIT imageChanged(url);
            Q_EMIT cachedImageChanged(cachedImage());
        } else if (m_image.isEmpty() && url == m_feed->image()) {
            Q_EMIT imageChanged(url);
            Q_EMIT cachedImageChanged(cachedImage());
        }
    });

    QSqlQuery entryQuery;
    entryQuery.prepare(QStringLiteral("SELECT * FROM Entries WHERE feed=:feed AND id=:id;"));
    entryQuery.bindValue(QStringLiteral(":feed"), m_feed->url());
    entryQuery.bindValue(QStringLiteral(":id"), id);
    Database::instance().execute(entryQuery);
    if (!entryQuery.next())
        qWarning() << "No element with index" << id << "found in feed" << m_feed->url();

    QSqlQuery authorQuery;
    authorQuery.prepare(QStringLiteral("SELECT * FROM Authors WHERE id=:id"));
    authorQuery.bindValue(QStringLiteral(":id"), entryQuery.value(QStringLiteral("id")).toString());
    Database::instance().execute(authorQuery);

    while (authorQuery.next()) {
        m_authors += new Author(authorQuery.value(QStringLiteral("name")).toString(),
                                authorQuery.value(QStringLiteral("email")).toString(),
                                authorQuery.value(QStringLiteral("uri")).toString(),
                                nullptr);
    }

    m_created.setSecsSinceEpoch(entryQuery.value(QStringLiteral("created")).toInt());
    m_updated.setSecsSinceEpoch(entryQuery.value(QStringLiteral("updated")).toInt());

    m_id = entryQuery.value(QStringLiteral("id")).toString();
    m_title = entryQuery.value(QStringLiteral("title")).toString();
    m_content = entryQuery.value(QStringLiteral("content")).toString();
    m_link = entryQuery.value(QStringLiteral("link")).toString();
    m_read = entryQuery.value(QStringLiteral("read")).toBool();
    m_new = entryQuery.value(QStringLiteral("new")).toBool();

    if (entryQuery.value(QStringLiteral("hasEnclosure")).toBool()) {
        m_hasenclosure = true;
        m_enclosure = new Enclosure(this);
    }
    m_image = entryQuery.value(QStringLiteral("image")).toString();
}

Entry::~Entry()
{
    qDeleteAll(m_authors);
}

QString Entry::id() const
{
    return m_id;
}

QString Entry::title() const
{
    return m_title;
}

QString Entry::content() const
{
    return m_content;
}

QVector<Author *> Entry::authors() const
{
    return m_authors;
}

QDateTime Entry::created() const
{
    return m_created;
}

QDateTime Entry::updated() const
{
    return m_updated;
}

QString Entry::link() const
{
    return m_link;
}

bool Entry::read() const
{
    return m_read;
}

bool Entry::getNew() const
{
    return m_new;
}

QString Entry::baseUrl() const
{
    return QUrl(m_link).adjusted(QUrl::RemovePath).toString();
}

void Entry::setRead(bool read)
{
    if (read != m_read) {
        // Making a detour through DataManager to make bulk operations more
        // performant.  DataManager will call setReadInternal on every item to
        // be marked read/unread.  So implement features there.
        DataManager::instance().bulkMarkRead(read, QStringList(m_id));
    }
}

void Entry::setReadInternal(bool read)
{
    if (read != m_read) {
        // Make sure that operations done here can be wrapped inside an sqlite
        // transaction.  I.e. no calls that trigger a SELECT operation.
        m_read = read;
        Q_EMIT readChanged(m_read);

        QSqlQuery query;
        query.prepare(QStringLiteral("UPDATE Entries SET read=:read WHERE id=:id AND feed=:feed"));
        query.bindValue(QStringLiteral(":id"), m_id);
        query.bindValue(QStringLiteral(":feed"), m_feed->url());
        query.bindValue(QStringLiteral(":read"), m_read);
        Database::instance().execute(query);

        m_feed->setUnreadEntryCount(m_feed->unreadEntryCount() + (read ? -1 : 1));

        // Follow up actions
        if (read) {
            // 1) Remove item from queue
            setQueueStatusInternal(false);

            // 2) Remove "new" label
            setNewInternal(false);

            if (hasEnclosure()) {
                // 3) Reset play position
                if (SettingsManager::self()->resetPositionOnPlayed()) {
                    m_enclosure->setPlayPosition(0);
                }

                // 4) Delete episode if that setting is set
                if (SettingsManager::self()->autoDeleteOnPlayed() == 1) {
                    m_enclosure->deleteFile();
                }
            }
            // 5) Log a sync action to sync this state with (gpodder) server
            Sync::instance().storePlayedEpisodeAction(m_id);
        }
    }
}

void Entry::setNew(bool state)
{
    if (state != m_new) {
        // Making a detour through DataManager to make bulk operations more
        // performant.  DataManager will call setNewInternal on every item to
        // be marked new/not new.  So implement features there.
        DataManager::instance().bulkMarkNew(state, QStringList(m_id));
    }
}

void Entry::setNewInternal(bool state)
{
    if (state != m_new) {
        // Make sure that operations done here can be wrapped inside an sqlite
        // transaction.  I.e. no calls that trigger a SELECT operation.
        m_new = state;
        Q_EMIT newChanged(m_new);

        QSqlQuery query;
        query.prepare(QStringLiteral("UPDATE Entries SET new=:new WHERE id=:id;"));
        query.bindValue(QStringLiteral(":id"), m_id);
        query.bindValue(QStringLiteral(":new"), m_new);
        Database::instance().execute(query);

        // Q_EMIT m_feed->newEntryCountChanged();  // TODO: signal and slots to be implemented
        Q_EMIT DataManager::instance().newEntryCountChanged(m_feed->url());
    }
}

QString Entry::adjustedContent(int width, int fontSize)
{
    QString ret(m_content);
    QRegularExpression imgRegex(QStringLiteral("<img ((?!width=\"[0-9]+(px)?\").)*(width=\"([0-9]+)(px)?\")?[^>]*>"));

    QRegularExpressionMatchIterator i = imgRegex.globalMatch(ret);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();

        QString imgTag(match.captured());
        if (imgTag.contains(QStringLiteral("wp-smiley")))
            imgTag.insert(4, QStringLiteral(" width=\"%1\"").arg(fontSize));

        QString widthParameter = match.captured(4);

        if (widthParameter.length() != 0) {
            if (widthParameter.toInt() > width) {
                imgTag.replace(match.captured(3), QStringLiteral("width=\"%1\"").arg(width));
                imgTag.replace(QRegularExpression(QStringLiteral("height=\"([0-9]+)(px)?\"")), QString());
            }
        } else {
            imgTag.insert(4, QStringLiteral(" width=\"%1\"").arg(width));
        }
        ret.replace(match.captured(), imgTag);
    }

    ret.replace(QStringLiteral("<img"), QStringLiteral("<br /> <img"));
    return ret;
}

Enclosure *Entry::enclosure() const
{
    return m_enclosure;
}

bool Entry::hasEnclosure() const
{
    return m_hasenclosure;
}

QString Entry::image() const
{
    if (!m_image.isEmpty()) {
        return m_image;
    } else {
        return m_feed->image();
    }
}

QString Entry::cachedImage() const
{
    // First check for the feed image as fallback
    QString image = m_image;
    if (image.isEmpty()) {
        image = m_feed->image();
    }

    return Fetcher::instance().image(image);
}

bool Entry::queueStatus() const
{
    return DataManager::instance().entryInQueue(this);
}

void Entry::setQueueStatus(bool state)
{
    if (state != DataManager::instance().entryInQueue(this)) {
        // Making a detour through DataManager to make bulk operations more
        // performant.  DataManager will call setQueueStatusInternal on every
        // item to be processed.  So implement features there.
        DataManager::instance().bulkQueueStatus(state, QStringList(m_id));
    }
}

void Entry::setQueueStatusInternal(bool state)
{
    // Make sure that operations done here can be wrapped inside an sqlite
    // transaction.  I.e. no calls that trigger a SELECT operation.
    if (state) {
        DataManager::instance().addToQueue(m_id);
        // Set status to unplayed/unread when adding item to the queue
        setReadInternal(false);
    } else {
        DataManager::instance().removeFromQueue(m_id);
        // Unset "new" state
        setNewInternal(false);
    }

    Q_EMIT queueStatusChanged(state);
}

void Entry::setImage(const QString &image)
{
    m_image = image;
    Q_EMIT imageChanged(m_image);
    Q_EMIT cachedImageChanged(cachedImage());
}

Feed *Entry::feed() const
{
    return m_feed;
}
