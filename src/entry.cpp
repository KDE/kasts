/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <tobias.fella@kde.org>
 * SPDX-FileCopyrightText: 2021-2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "entry.h"
#include "entrylogging.h"

#include <QRegularExpression>
#include <QSqlQuery>
#include <QUrl>

#include <KLocalizedString>

#include "database.h"
#include "datamanager.h"
#include "feed.h"
#include "fetcher.h"
#include "settingsmanager.h"
#include "sync/sync.h"

Entry::Entry(const qint64 entryuid, QObject *parent)
    : QObject(parent)
    , m_entryuid(entryuid)
{
    parent = &DataManager::instance(); // TODO: check whether we can have these autodestroyed to save memory

    connect(&Fetcher::instance(), &Fetcher::downloadFinished, this, [this](QString url) {
        if (url == m_image) {
            Q_EMIT imageChanged(url);
            Q_EMIT cachedImageChanged(cachedImage());
        } else if (m_image.isEmpty() && url == m_feed->image()) {
            Q_EMIT imageChanged(url);
            Q_EMIT cachedImageChanged(cachedImage());
        }
    });
    connect(&Fetcher::instance(), &Fetcher::entryUpdated, this, [this](const QString &url, const QString &id) {
        if ((m_feed->url() == url) && (m_id == id)) {
            updateFromDb();
        }
    });

    updateFromDb(false);
}

Entry::Entry(Feed *feed, const QString &id, QObject *parent)
    : QObject(parent)
    , m_feed(feed)
    , m_id(id)
{
    parent = &DataManager::instance(); // TODO: check whether we can have these autodestroyed to save memory
    connect(&Fetcher::instance(), &Fetcher::downloadFinished, this, [this](QString url) {
        if (url == m_image) {
            Q_EMIT imageChanged(url);
            Q_EMIT cachedImageChanged(cachedImage());
        } else if (m_image.isEmpty() && url == m_feed->image()) {
            Q_EMIT imageChanged(url);
            Q_EMIT cachedImageChanged(cachedImage());
        }
    });
    connect(&Fetcher::instance(), &Fetcher::entryUpdated, this, [this](const QString &url, const QString &id) {
        if ((m_feed->url() == url) && (m_id == id)) {
            updateFromDb();
        }
    });

    QSqlQuery entryQuery;
    entryQuery.prepare(QStringLiteral("SELECT entryuid FROM Entries WHERE id=:id;"));
    entryQuery.bindValue(QStringLiteral(":id"), m_id);
    Database::instance().execute(entryQuery);
    if (!entryQuery.next()) {
        qWarning() << "No element with entryuid" << m_id;
        return;
    }
    m_entryuid = entryQuery.value(QStringLiteral("entryuid")).toLongLong();

    updateFromDb(false);
}

void Entry::updateFromDb(bool emitSignals)
{
    QSqlQuery entryQuery;
    entryQuery.prepare(QStringLiteral("SELECT * FROM Entries WHERE entryuid=:entryuid;"));
    entryQuery.bindValue(QStringLiteral(":entryuid"), m_entryuid);
    Database::instance().execute(entryQuery);
    if (!entryQuery.next()) {
        qWarning() << "No element with entryuid" << m_entryuid;
        return;
    }

    m_feeduid = entryQuery.value(QStringLiteral("feeduid")).toLongLong();
    // TODO: can we get rid of the feed pointer?
    if (m_feed == nullptr) {
        m_feed = DataManager::instance().getFeed(m_feeduid);
    }

    m_id = entryQuery.value(QStringLiteral("id")).toString();
    setCreated(QDateTime::fromSecsSinceEpoch(entryQuery.value(QStringLiteral("created")).toInt()), emitSignals);
    setUpdated(QDateTime::fromSecsSinceEpoch(entryQuery.value(QStringLiteral("updated")).toInt()), emitSignals);
    setTitle(entryQuery.value(QStringLiteral("title")).toString(), emitSignals);
    setContent(entryQuery.value(QStringLiteral("content")).toString(), emitSignals);
    setLink(entryQuery.value(QStringLiteral("link")).toString(), emitSignals);

    if (m_read != entryQuery.value(QStringLiteral("read")).toBool()) {
        m_read = entryQuery.value(QStringLiteral("read")).toBool();
        Q_EMIT readChanged(m_read);
    }
    if (m_new != entryQuery.value(QStringLiteral("new")).toBool()) {
        m_new = entryQuery.value(QStringLiteral("new")).toBool();
        Q_EMIT newChanged(m_new);
    }
    if (m_favorite != entryQuery.value(QStringLiteral("favorite")).toBool()) {
        m_favorite = entryQuery.value(QStringLiteral("favorite")).toBool();
        Q_EMIT favoriteChanged(m_favorite);
    }

    setHasEnclosure(entryQuery.value(QStringLiteral("hasEnclosure")).toBool(), emitSignals);
    setImage(entryQuery.value(QStringLiteral("image")).toString(), emitSignals);

    updateAuthors();
}

void Entry::updateAuthors()
{
    QStringList authors;

    QSqlQuery authorQuery;
    authorQuery.prepare(QStringLiteral("SELECT name FROM EntryAuthors WHERE entryuid=:entryuid;"));
    authorQuery.bindValue(QStringLiteral(":entryuid"), m_entryuid);
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

qint64 Entry::entryuid() const
{
    return m_entryuid;
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

QString Entry::authors() const
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

bool Entry::favorite() const
{
    return m_favorite;
}

QString Entry::baseUrl() const
{
    return QUrl(m_link).adjusted(QUrl::RemovePath).toString();
}

void Entry::setTitle(const QString &title, bool emitSignal)
{
    if (m_title != title) {
        m_title = title;
        if (emitSignal) {
            Q_EMIT titleChanged(m_title);
        }
    }
}

void Entry::setContent(const QString &content, bool emitSignal)
{
    if (m_content != content) {
        m_content = content;
        if (emitSignal) {
            Q_EMIT contentChanged(m_content);
        }
    }
}

void Entry::setCreated(const QDateTime &created, bool emitSignal)
{
    if (m_created != created) {
        m_created = created;
        if (emitSignal) {
            Q_EMIT createdChanged(m_created);
        }
    }
}

void Entry::setUpdated(const QDateTime &updated, bool emitSignal)
{
    if (m_updated != updated) {
        m_updated = updated;
        if (emitSignal) {
            Q_EMIT updatedChanged(m_updated);
        }
    }
}

void Entry::setLink(const QString &link, bool emitSignal)
{
    if (m_link != link) {
        m_link = link;
        if (emitSignal) {
            Q_EMIT linkChanged(m_link);
            Q_EMIT baseUrlChanged(baseUrl());
        }
    }
}

void Entry::setHasEnclosure(bool hasEnclosure, bool emitSignal)
{
    if (hasEnclosure) {
        // refresh enclosure anyway since we don't know whether it's been updated.
        if (m_enclosure) {
            delete m_enclosure;
        }
        m_enclosure = new Enclosure(this);
    } else {
        delete m_enclosure;
        m_enclosure = nullptr;
    }
    if (m_hasenclosure != hasEnclosure) {
        m_hasenclosure = hasEnclosure;
        if (emitSignal) {
            Q_EMIT hasEnclosureChanged(m_hasenclosure);
        }
    }
}

void Entry::setImage(const QString &image, bool emitSignal)
{
    if (m_image != image) {
        m_image = image;
        if (emitSignal) {
            Q_EMIT imageChanged(m_image);
            Q_EMIT cachedImageChanged(cachedImage());
        }
    }
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
        query.prepare(QStringLiteral("UPDATE Entries SET read=:read WHERE entryuid=:entryuid;"));
        query.bindValue(QStringLiteral(":entryuid"), m_entryuid);
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

                // 4) Stop download if it's currently running
                Q_EMIT m_enclosure->cancelDownload();

                // 5) Delete episode if that setting is set
                if (SettingsManager::self()->autoDeleteOnPlayed() == 1) {
                    m_enclosure->deleteFile();
                }
            }
            // 6) Log a sync action to sync this state with (gpodder) server
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
        query.prepare(QStringLiteral("UPDATE Entries SET new=:new WHERE entryuid=:entryuid;"));
        query.bindValue(QStringLiteral(":entryuid"), m_entryuid);
        query.bindValue(QStringLiteral(":new"), m_new);
        Database::instance().execute(query);

        // Q_EMIT m_feed->newEntryCountChanged();  // TODO: signal and slots to be implemented
        Q_EMIT DataManager::instance().newEntryCountChanged(m_feed->url());
    }
}

void Entry::setFavorite(bool favorite)
{
    if (favorite != m_favorite) {
        // Making a detour through DataManager to make bulk operations more
        // performant.  DataManager will call setFavoriteInternal on every item to
        // be marked new/not new.  So implement features there.
        DataManager::instance().bulkMarkFavorite(favorite, QStringList(m_id));
    }
}

void Entry::setFavoriteInternal(bool favorite)
{
    if (favorite != m_favorite) {
        // Make sure that operations done here can be wrapped inside an sqlite
        // transaction.  I.e. no calls that trigger a SELECT operation.
        m_favorite = favorite;
        Q_EMIT favoriteChanged(m_favorite);

        QSqlQuery query;
        query.prepare(QStringLiteral("UPDATE Entries SET favorite=:favorite WHERE entryuid=:entryuid;"));
        query.bindValue(QStringLiteral(":entryuid"), m_entryuid);
        query.bindValue(QStringLiteral(":favorite"), m_favorite);
        Database::instance().execute(query);

        Q_EMIT DataManager::instance().favoriteEntryCountChanged(m_feed->url());
    }
}

QString Entry::adjustedContent(int width, int fontSize)
{
    static QRegularExpression imgRegex(QStringLiteral("<img ((?!width=\"[0-9]+(px)?\").)*(width=\"([0-9]+)(px)?\")?[^>]*>"));
    static QRegularExpression imgHeightRegex(QStringLiteral("height=\"([0-9]+)(px)?\""));

    QString ret(m_content);

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
                imgTag.replace(imgHeightRegex, QString());
            }
        }
        ret.replace(match.captured(), imgTag);
    }

    ret.replace(QStringLiteral("<img"), QStringLiteral("<br /> <img"));

    // Replace strings that look like timestamps into clickable links with scheme
    // "timestamp://".  We will pick these up in the GUI to work like chapter marks

    static QRegularExpression dateRegex(QStringLiteral("\\d{1,2}(:\\d{2})+"));

    i = dateRegex.globalMatch(ret);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        QString timeStamp(match.captured());
        QStringList timeFragments(timeStamp.split(QStringLiteral(":")));
        int timeUnit = 1;
        qint64 time = 0;
        for (QList<QString>::const_reverse_iterator iter = timeFragments.crbegin(); iter != timeFragments.crend(); iter++) {
            time += (*iter).toInt() * 1000 * timeUnit;
            timeUnit *= 60;
        }
        timeStamp = QStringLiteral("<a href=\"timestamp://%1\">%2</a>").arg(time).arg(timeStamp);
        ret.replace(match.captured(), timeStamp);
    }

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
    if (m_hasenclosure && !m_enclosure->cachedEmbeddedImage().isEmpty()) {
        // use embedded image if available
        return m_enclosure->cachedEmbeddedImage();
    } else if (!m_image.isEmpty()) {
        return m_image;
    } else {
        // else fall back to feed image
        return m_feed->image();
    }
}

QString Entry::cachedImage() const
{
    // First check for an image in the downloaded file
    if (m_hasenclosure && !m_enclosure->cachedEmbeddedImage().isEmpty()) {
        // use embedded image if available
        return m_enclosure->cachedEmbeddedImage();
    }

    // Then check for the entry image, fall back if needed to feed image
    QString image = m_image;
    if (image.isEmpty()) {
        // else fall back to feed image
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

Feed *Entry::feed() const
{
    return m_feed;
}
