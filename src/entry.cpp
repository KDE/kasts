/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 * SPDX-FileCopyrightText: 2021-2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "entry.h"
#include "entrylogging.h"

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
    , m_id(id)
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
    connect(&Fetcher::instance(), &Fetcher::entryUpdated, this, [this](const QString &url, const QString &id) {
        if ((m_feed->url() == url) && (m_id == id)) {
            updateFromDb();
        }
    });

    updateFromDb(false);
}

Entry::~Entry()
{
}

void Entry::updateFromDb(bool emitSignals)
{
    QSqlQuery entryQuery;
    entryQuery.prepare(QStringLiteral("SELECT * FROM Entries WHERE feed=:feed AND id=:id;"));
    entryQuery.bindValue(QStringLiteral(":feed"), m_feed->url());
    entryQuery.bindValue(QStringLiteral(":id"), m_id);
    Database::instance().execute(entryQuery);
    if (!entryQuery.next()) {
        qWarning() << "No element with index" << m_id << "found in feed" << m_feed->url();
        return;
    }

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

    setHasEnclosure(entryQuery.value(QStringLiteral("hasEnclosure")).toBool(), emitSignals);
    setImage(entryQuery.value(QStringLiteral("image")).toString(), emitSignals);

    updateAuthors(emitSignals);
}

void Entry::updateAuthors(bool emitSignals)
{
    QVector<Author *> newAuthors;
    bool haveAuthorsChanged = false;

    QSqlQuery authorQuery;
    authorQuery.prepare(QStringLiteral("SELECT * FROM Authors WHERE id=:id AND feed=:feed;"));
    authorQuery.bindValue(QStringLiteral(":id"), m_id);
    authorQuery.bindValue(QStringLiteral(":feed"), m_feed->url());
    Database::instance().execute(authorQuery);
    while (authorQuery.next()) {
        // check if author already exists, if so, then reuse
        bool existingAuthor = false;
        QString name = authorQuery.value(QStringLiteral("name")).toString();
        QString email = authorQuery.value(QStringLiteral("email")).toString();
        QString url = authorQuery.value(QStringLiteral("uri")).toString();
        qCDebug(kastsEntry) << name << email << url;
        for (Author *author : m_authors) {
            if (author)
                qCDebug(kastsEntry) << "old authors" << author->name() << author->email() << author->url();
            if (author && author->name() == name && author->email() == email && author->url() == url) {
                existingAuthor = true;
                newAuthors += author;
            }
        }
        if (!existingAuthor) {
            newAuthors += new Author(name, email, url, this);
            haveAuthorsChanged = true;
        }
    }

    // Finally check whether m_authors and newAuthors are identical
    // if not, then delete the authors that were removed
    for (Author *author : m_authors) {
        if (!newAuthors.contains(author)) {
            delete author;
            haveAuthorsChanged = true;
        }
    }

    m_authors = newAuthors;

    if (haveAuthorsChanged && emitSignals) {
        Q_EMIT authorsChanged(m_authors);
        qCDebug(kastsEntry) << "entry" << m_id << "authors have changed?" << haveAuthorsChanged;
    }
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
        // if there is already an enclosure, it will be updated through separate
        // signals if required
        if (!m_enclosure) {
            m_enclosure = new Enclosure(this);
        }
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
        }
        ret.replace(match.captured(), imgTag);
    }

    ret.replace(QStringLiteral("<img"), QStringLiteral("<br /> <img"));

    // Replace strings that look like timestamps into clickable links with scheme
    // "timestamp://".  We will pick these up in the GUI to work like chapter marks

    QRegularExpression imgRegexDate(QStringLiteral("\\d{1,2}(:\\d{2})+"));

    i = imgRegexDate.globalMatch(ret);
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
    if (!m_image.isEmpty()) {
        return m_image;
    } else if (m_hasenclosure && !m_enclosure->cachedEmbeddedImage().isEmpty()) {
        // use embedded image if available
        return m_enclosure->cachedEmbeddedImage();
    } else {
        // else fall back to feed image
        return m_feed->image();
    }
}

QString Entry::cachedImage() const
{
    // First check for the feed image, fall back if needed
    QString image = m_image;
    if (image.isEmpty()) {
        if (m_hasenclosure && !m_enclosure->cachedEmbeddedImage().isEmpty()) {
            // use embedded image if available
            return m_enclosure->cachedEmbeddedImage();
        } else {
            // else fall back to feed image
            image = m_feed->image();
        }
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
