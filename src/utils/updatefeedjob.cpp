/**
 * SPDX-FileCopyrightText: 2021-2022 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "updatefeedjob.h"

#include <QCryptographicHash>
#include <QDir>
#include <QDomElement>
#include <QMultiMap>
#include <QNetworkReply>
#include <QSqlError>
#include <QSqlQuery>
#include <QTextDocumentFragment>
#include <QTimer>

#include <KLocalizedString>
#include <ThreadWeaver/Thread>

#include "database.h"
#include "datatypes.h"
#include "enclosure.h"
#include "error.h"
#include "fetcher.h"
#include "fetcherlogging.h"
#include "settingsmanager.h"
#include "storagemanager.h"

using namespace ThreadWeaver;
using namespace DataTypes;

UpdateFeedJob::UpdateFeedJob(const QByteArray &data, const FeedDetails &oldFeedDetails, QObject *parent)
    : QObject(parent)
    , m_data(data)
    , m_oldFeedDetails(oldFeedDetails)
{
    m_url = oldFeedDetails.url;

    // connect to signals in Fetcher such that GUI can pick up the changes
    // TODO: Clean out the unnecessary signals
    connect(this, &UpdateFeedJob::feedDetailsUpdated, &Fetcher::instance(), &Fetcher::feedDetailsUpdated);
    connect(this, &UpdateFeedJob::feedUpdated, &Fetcher::instance(), &Fetcher::feedUpdated);
    connect(this, &UpdateFeedJob::entryAdded, &Fetcher::instance(), &Fetcher::entryAdded);
    connect(this, &UpdateFeedJob::entryUpdated, &Fetcher::instance(), &Fetcher::entryUpdated);
    connect(this, &UpdateFeedJob::error, &Fetcher::instance(), &Fetcher::error);
}

void UpdateFeedJob::run(JobPointer, Thread *)
{
    if (m_abort) {
        Q_EMIT finished();
        return;
    }

    Database::openDatabase(m_url);

    Syndication::DocumentSource document(m_data, m_url);
    Syndication::FeedPtr feed = Syndication::parserCollection()->parse(document, QStringLiteral("Atom"));
    processFeed(feed);

    Database::closeDatabase(m_url);
    Q_EMIT finished();
    Q_EMIT resultsAvailable(m_feedDetails);
}

void UpdateFeedJob::processFeed(Syndication::FeedPtr feed)
{
    qCDebug(kastsFetcher) << "start process feed" << feed;

    if (feed.isNull())
        return;

    // First check if this is a newly added feed and get current name and dirname
    if (m_oldFeedDetails.isNew) {
        qCDebug(kastsFetcher) << "New feed" << feed->title();
    }

    m_markUnreadOnNewFeed = !(SettingsManager::self()->markUnreadOnNewFeed() == 2);
    QDateTime current = QDateTime::currentDateTime();

    m_feedDetails = std::as_const(m_oldFeedDetails); // start from current feed details in database

    m_feedDetails.name = feed->title();
    m_feedDetails.link = feed->link();
    m_feedDetails.description = feed->description();
    m_feedDetails.lastUpdated = current.toSecsSinceEpoch();
    m_feedDetails.lastHash = QString::fromLatin1(QCryptographicHash::hash(m_data, QCryptographicHash::Sha256).toHex());

    // Retrieve "other" fields; this will include the "itunes" tags
    QMultiMap<QString, QDomElement> otherItems = feed->additionalProperties();

    // First try the itunes tags, if not, fall back to regular image tag
    if (otherItems.value(QStringLiteral("http://www.itunes.com/dtds/podcast-1.0.dtdimage")).hasAttribute(QStringLiteral("href"))) {
        m_feedDetails.image = otherItems.value(QStringLiteral("http://www.itunes.com/dtds/podcast-1.0.dtdimage")).attribute(QStringLiteral("href"));
    } else {
        m_feedDetails.image = feed->image()->url();
    }

    if (m_feedDetails.image.startsWith(QStringLiteral("/"))) {
        m_feedDetails.image = QUrl(m_url).adjusted(QUrl::RemovePath).toString() + m_feedDetails.image;
    }

    // if the title has changed, we need to rename the corresponding enclosure
    // download directory name and move the files
    if (m_oldFeedDetails.name != m_feedDetails.name || m_oldFeedDetails.dirname.isEmpty() || m_oldFeedDetails.isNew) {
        QString generatedDirname = generateFeedDirname(m_feedDetails.name);
        if (generatedDirname != m_oldFeedDetails.dirname) {
            m_feedDetails.dirname = generatedDirname;
            QString enclosurePath = StorageManager::instance().enclosureDirPath();
            if (QDir(enclosurePath + m_oldFeedDetails.dirname).exists()) {
                QDir().rename(enclosurePath + m_oldFeedDetails.dirname, enclosurePath + m_feedDetails.dirname);
            } else {
                QDir().mkpath(enclosurePath + m_feedDetails.dirname);
            }
        }
    }

    QSqlQuery query(QSqlDatabase::database(m_url));
    query.prepare(QStringLiteral(
        "UPDATE Feeds SET name=:name, image=:image, link=:link, description=:description, lastUpdated=:lastUpdated, dirname=:dirname WHERE url=:url;"));
    query.bindValue(QStringLiteral(":name"), m_feedDetails.name);
    query.bindValue(QStringLiteral(":url"), m_feedDetails.url);
    query.bindValue(QStringLiteral(":link"), m_feedDetails.link);
    query.bindValue(QStringLiteral(":description"), m_feedDetails.description);
    query.bindValue(QStringLiteral(":lastUpdated"), m_feedDetails.lastUpdated);
    query.bindValue(QStringLiteral(":image"), m_feedDetails.image);
    query.bindValue(QStringLiteral(":dirname"), m_feedDetails.dirname);
    // we only write the new lastHash to the database after entries etc. have
    // all been updated!
    dbExecute(query);
    query.clear(); // make sure this query is not blocking anything anymore

    bool authorsChanged = false;
    // Process feed authors
    if (feed->authors().count() > 0) {
        for (const auto &author : feed->authors()) {
            processAuthor(author->name(), author->email(), m_feedDetails.authors);
        }
    } else {
        // Try to find itunes fields if plain author doesn't exist
        QString authorname, authoremail;
        // First try the "itunes:owner" tag, if that doesn't succeed, then try the "itunes:author" tag
        if (otherItems.value(QStringLiteral("http://www.itunes.com/dtds/podcast-1.0.dtdowner")).hasChildNodes()) {
            QDomNodeList nodelist = otherItems.value(QStringLiteral("http://www.itunes.com/dtds/podcast-1.0.dtdowner")).childNodes();
            for (int i = 0; i < nodelist.length(); i++) {
                if (nodelist.item(i).nodeName() == QStringLiteral("itunes:name")) {
                    authorname = nodelist.item(i).toElement().text();
                } else if (nodelist.item(i).nodeName() == QStringLiteral("itunes:email")) {
                    authoremail = nodelist.item(i).toElement().text();
                }
            }
        } else {
            authorname = otherItems.value(QStringLiteral("http://www.itunes.com/dtds/podcast-1.0.dtdauthor")).text();
            qCDebug(kastsFetcher) << "authorname" << authorname;
        }
        if (!authorname.isEmpty()) {
            processAuthor(authorname, authoremail, m_feedDetails.authors);
        }
    }

    qCDebug(kastsFetcher) << "Updated feed details:" << feed->title();

    // check if any field has changed and only emit signal if there are changes
    if (m_feedDetails.name != m_oldFeedDetails.name || m_feedDetails.link != m_oldFeedDetails.link || m_feedDetails.image != m_oldFeedDetails.image
        || m_feedDetails.description != m_oldFeedDetails.description || authorsChanged) {
        m_feedDetails.state = DataTypes::RecordState::Modified;
        Q_EMIT feedDetailsUpdated(m_url,
                                  m_feedDetails.name,
                                  m_feedDetails.image,
                                  m_feedDetails.link,
                                  m_feedDetails.description,
                                  current,
                                  m_feedDetails.dirname);
    }

    if (m_abort)
        return;

    // Now deal with the entries, enclosures, entry authors and chapter marks
    const auto items = feed->items();
    for (const auto &feedEntry : items) {
        if (m_abort)
            return;

        processEntry(feedEntry, m_feedDetails.entries);
    }

    qCDebug(kastsFetcher) << "done processing feed" << feed;
}

void UpdateFeedJob::processEntry(const Syndication::ItemPtr entry, QHash<QString, DataTypes::EntryDetails> &entries)
{
    qCDebug(kastsFetcher) << "Processing" << entry->title();

    // Retrieve "other" fields; this will include the "itunes" tags
    QMultiMap<QString, QDomElement> otherItems = entry->additionalProperties();

    const auto uniqueKeys = otherItems.uniqueKeys();
    for (const QString &key : uniqueKeys) {
        qCDebug(kastsFetcher) << "other elements";
        qCDebug(kastsFetcher) << key << otherItems.value(key).tagName();
    }

    QString id = entry->id();
    QString title = QTextDocumentFragment::fromHtml(entry->title()).toPlainText();
    int created = static_cast<int>(entry->datePublished());
    int updated = static_cast<int>(entry->dateUpdated());
    QString link = entry->link();
    bool hasEnclosure = (entry->enclosures().length() > 0);
    bool read = m_oldFeedDetails.isNew ? m_markUnreadOnNewFeed : false; // if new feed, then check settings
    bool isNew = !m_oldFeedDetails.isNew; // if new feed, then mark none as new
    QString content;
    QString image;

    // This is needed to be able to rename enclosures of the entry title changes
    QString oldTitle = title;

    // Take the longest text, either content or description
    if (entry->content().length() > entry->description().length()) {
        content = entry->content();
    } else {
        content = entry->description();
    }

    // Look for image in itunes tags
    if (otherItems.value(QStringLiteral("http://www.itunes.com/dtds/podcast-1.0.dtdimage")).hasAttribute(QStringLiteral("href"))) {
        image = otherItems.value(QStringLiteral("http://www.itunes.com/dtds/podcast-1.0.dtdimage")).attribute(QStringLiteral("href"));
    } else if (otherItems.contains(QStringLiteral("http://search.yahoo.com/mrss/thumbnail"))) {
        image = otherItems.value(QStringLiteral("http://search.yahoo.com/mrss/thumbnail")).attribute(QStringLiteral("url"));
    }
    if (image.startsWith(QStringLiteral("/"))) {
        image = QUrl(m_url).adjusted(QUrl::RemovePath).toString() + image;
    }
    qCDebug(kastsFetcher) << "Entry image found" << image;

    // now we start updating the datastructure
    if (entries.contains(id)) {
        // stop here if doFullUpdate is set to false and this is an existing entry
        if (!SettingsManager::self()->doFullUpdate()) {
            return;
        }
        if ((entries[id].title != title) || (entries[id].content != content) || (entries[id].created != created) || (entries[id].updated != updated)
            || (entries[id].link != link) || (entries[id].hasEnclosure != hasEnclosure) || (entries[id].image != image)) {
            oldTitle = entries[id].title; // save old title for later on
            entries[id].title = title;
            entries[id].content = content;
            entries[id].created = created;
            entries[id].updated = updated;
            entries[id].link = link;
            entries[id].hasEnclosure = hasEnclosure;
            entries[id].image = image;
            entries[id].state = DataTypes::RecordState::Modified;
            qCDebug(kastsFetcher) << "episode details have been updated:" << id;
        } else {
            entries[id].state = DataTypes::RecordState::Unmodified;
            qCDebug(kastsFetcher) << "episode details are unchanged:" << id;
        }
    } else {
        EntryDetails newEntry;
        entries[id].feed = m_url;
        entries[id].id = id;
        entries[id].title = title;
        entries[id].content = content;
        entries[id].created = created;
        entries[id].updated = updated;
        entries[id].link = link;
        entries[id].read = read;
        entries[id].isNew = isNew;
        entries[id].hasEnclosure = hasEnclosure;
        entries[id].image = image;
        entries[id].state = DataTypes::RecordState::New;
        entries[id] = newEntry;
        qCDebug(kastsFetcher) << "new episode details added:" << id;
    }

    // Process authors
    if (entry->authors().count() > 0) {
        const auto authors = entry->authors();
        for (const auto &author : authors) {
            processAuthor(author->name(), author->email(), entries[id].authors);
        }
    } else {
        // As fallback, check if there is itunes "author" information
        QString authorName = otherItems.value(QStringLiteral("http://www.itunes.com/dtds/podcast-1.0.dtdauthor")).text();
        if (!authorName.isEmpty())
            processAuthor(authorName, QLatin1String(""), entries[id].authors);
    }

    // Process chapters
    if (otherItems.value(QStringLiteral("http://podlove.org/simple-chapterschapters")).hasChildNodes()) {
        QDomNodeList nodelist = otherItems.value(QStringLiteral("http://podlove.org/simple-chapterschapters")).childNodes();
        for (int i = 0; i < nodelist.length(); i++) {
            if (nodelist.item(i).nodeName() == QStringLiteral("psc:chapter")) {
                QDomElement element = nodelist.at(i).toElement();
                QString title = element.attribute(QStringLiteral("title"));
                QString start = element.attribute(QStringLiteral("start"));
                QStringList startParts = start.split(QStringLiteral(":"));
                // Some podcasts use colon for milliseconds as well
                while (startParts.count() > 3) {
                    startParts.removeLast();
                }
                int startInt = 0;
                for (const QString &part : std::as_const(startParts)) {
                    // strip off decimal point if it's present
                    startInt = part.split(QStringLiteral("."))[0].toInt() + startInt * 60;
                }
                qCDebug(kastsFetcher) << "Found chapter mark:" << start << "; in seconds:" << startInt;
                QString images = element.attribute(QStringLiteral("image"));
                processChapter(startInt, title, entry->link(), images, entries[id].chapters);
            }
        }
    }

    // Process enclosures
    // only process first enclosure if there are multiple (e.g. mp3 and ogg);
    // the first one is probably the podcast author's preferred version
    // TODO: handle more than one enclosure?
    for (auto &enclosure : entry->enclosures()) {
        processEnclosure(enclosure, oldTitle, title, entries[id].enclosures);
    }
}

void UpdateFeedJob::processAuthor(const QString &name, const QString &email, QHash<QString, DataTypes::AuthorDetails> &authors)
{
    if (authors.contains(name)) {
        if (authors[name].email == email) {
            authors[name].state = DataTypes::RecordState::Unmodified;
            qCDebug(kastsFetcher) << "author details are unchanged:" << name << email;
        } else {
            authors[name].email = email;
            authors[name].state = DataTypes::RecordState::Modified;
            qCDebug(kastsFetcher) << "author details have been updated for:" << name << email;
        }
    } else {
        AuthorDetails newAuthor;
        newAuthor.name = name;
        newAuthor.email = email;
        newAuthor.state = DataTypes::RecordState::New;
        authors[name] = newAuthor;
        qCDebug(kastsFetcher) << "this is a new author:" << name << email;
    }
}

void UpdateFeedJob::processEnclosure(const Syndication::EnclosurePtr enclosure,
                                     const QString &oldEntryTitle,
                                     const QString &newEntryTitle,
                                     QHash<QString, DataTypes::EnclosureDetails> &enclosures)
{
    int duration = enclosure->duration();
    int size = enclosure->length();
    QString title = enclosure->title();
    QString type = enclosure->type();
    QString url = enclosure->url();
    int playPosition = 0;
    Enclosure::Status downloaded = Enclosure::Downloadable;

    if (enclosures.contains(url)) {
        if (title != enclosures[url].title || (type != enclosures[url].type)) {
            enclosures[url].title = title;
            enclosures[url].type = type;
            enclosures[url].state = DataTypes::RecordState::Modified;
            qCDebug(kastsFetcher) << "enclosure details have been updated for:" << url;
        } else {
            enclosures[url].state = DataTypes::RecordState::Unmodified;
            qCDebug(kastsFetcher) << "enclosure details are unchanged:" << url;
        }
        // TODO: Move to place where the actual database changes are stored; this avoids the paths getting out of sync with the DB
        // Check if entry title or enclosure URL has changed
        if (newEntryTitle != oldEntryTitle) {
            QString oldFilename = StorageManager::instance().enclosurePath(oldEntryTitle, url, m_feedDetails.dirname);
            QString newFilename = StorageManager::instance().enclosurePath(newEntryTitle, url, m_feedDetails.dirname);

            if (oldFilename != newFilename) {
                // If entry title has changed but URL is still the same, the existing enclosure needs to be renamed
                QFile::rename(oldFilename, newFilename);
            }
        }
    } else {
        EnclosureDetails newEnclosure;
        newEnclosure.duration = duration;
        newEnclosure.size = size;
        newEnclosure.title = title;
        newEnclosure.type = type;
        newEnclosure.url = url;
        newEnclosure.playPosition = playPosition;
        newEnclosure.downloaded = downloaded;
        newEnclosure.state = DataTypes::RecordState::New;
        enclosures[url] = newEnclosure;
        qCDebug(kastsFetcher) << "this is a new enclosure:" << url;
    }
}

void UpdateFeedJob::processChapter(const int &start, const QString &title, const QString &link, const QString &image, QHash<int, ChapterDetails> &chapters)
{
    if (chapters.contains(start)) {
        if ((chapters[start].title != title) || (chapters[start].link != link) || (chapters[start].image != image)) {
            chapters[start].title = title;
            chapters[start].link = link;
            chapters[start].image = image;
            chapters[start].state = DataTypes::RecordState::Modified;
            qCDebug(kastsFetcher) << "chapter details have been updated for:" << start;
        } else {
            chapters[start].state = DataTypes::RecordState::Unmodified;
            qCDebug(kastsFetcher) << "chapter details are unchanged:" << start;
        }
    } else {
        ChapterDetails newChapter;
        newChapter.start = start;
        newChapter.title = title;
        newChapter.link = link;
        newChapter.image = image;
        newChapter.state = DataTypes::RecordState::New;
        chapters[start] = newChapter;
        qCDebug(kastsFetcher) << "this is a new chapter:" << start;
    }
}

// void UpdateFeedJob::writeToDatabase()
// {
//     QSqlQuery writeQuery(QSqlDatabase::database(m_url));
//
//     dbTransaction();
//
//     // new entries
//     writeQuery.prepare(
//         QStringLiteral("INSERT INTO Entries VALUES (:feed, :id, :title, :content, :created, :updated, :link, :read, :new, :hasEnclosure, :image,
//         :favorite);"));
//     for (const EntryDetails &entryDetails : std::as_const(m_newEntries)) {
//         writeQuery.bindValue(QStringLiteral(":feed"), entryDetails.feed);
//         writeQuery.bindValue(QStringLiteral(":id"), entryDetails.id);
//         writeQuery.bindValue(QStringLiteral(":title"), entryDetails.title);
//         writeQuery.bindValue(QStringLiteral(":content"), entryDetails.content);
//         writeQuery.bindValue(QStringLiteral(":created"), entryDetails.created);
//         writeQuery.bindValue(QStringLiteral(":updated"), entryDetails.updated);
//         writeQuery.bindValue(QStringLiteral(":link"), entryDetails.link);
//         writeQuery.bindValue(QStringLiteral(":hasEnclosure"), entryDetails.hasEnclosure);
//         writeQuery.bindValue(QStringLiteral(":read"), entryDetails.read);
//         writeQuery.bindValue(QStringLiteral(":new"), entryDetails.isNew);
//         writeQuery.bindValue(QStringLiteral(":image"), entryDetails.image);
//         writeQuery.bindValue(QStringLiteral(":favorite"), false);
//         dbExecute(writeQuery);
//     }
//     writeQuery.clear();
//
//     // update entries
//     writeQuery.prepare(
//         QStringLiteral("UPDATE Entries SET title=:title, content=:content, created=:created, updated=:updated, link=:link, hasEnclosure=:hasEnclosure, "
//                        "image=:image WHERE id=:id AND feed=:feed;"));
//     for (const EntryDetails &entryDetails : std::as_const(m_updateEntries)) {
//         writeQuery.bindValue(QStringLiteral(":feed"), entryDetails.feed);
//         writeQuery.bindValue(QStringLiteral(":id"), entryDetails.id);
//         writeQuery.bindValue(QStringLiteral(":title"), entryDetails.title);
//         writeQuery.bindValue(QStringLiteral(":content"), entryDetails.content);
//         writeQuery.bindValue(QStringLiteral(":created"), entryDetails.created);
//         writeQuery.bindValue(QStringLiteral(":updated"), entryDetails.updated);
//         writeQuery.bindValue(QStringLiteral(":link"), entryDetails.link);
//         writeQuery.bindValue(QStringLiteral(":hasEnclosure"), entryDetails.hasEnclosure);
//         writeQuery.bindValue(QStringLiteral(":image"), entryDetails.image);
//         dbExecute(writeQuery);
//     }
//     writeQuery.clear();
//
//     // new authors
//     writeQuery.prepare(QStringLiteral("INSERT INTO Authors VALUES(:feed, :id, :name, :uri, :email);"));
//     for (const AuthorDetails &authorDetails : std::as_const(m_newAuthors)) {
//         writeQuery.bindValue(QStringLiteral(":feed"), authorDetails.feed);
//         writeQuery.bindValue(QStringLiteral(":id"), authorDetails.id);
//         writeQuery.bindValue(QStringLiteral(":name"), authorDetails.name);
//         writeQuery.bindValue(QStringLiteral(":uri"), authorDetails.uri);
//         writeQuery.bindValue(QStringLiteral(":email"), authorDetails.email);
//         dbExecute(writeQuery);
//     }
//     writeQuery.clear();
//
//     // update authors
//     writeQuery.prepare(QStringLiteral("UPDATE Authors SET uri=:uri, email=:email WHERE feed=:feed AND id=:id AND name=:name;"));
//     for (const AuthorDetails &authorDetails : std::as_const(m_updateAuthors)) {
//         writeQuery.bindValue(QStringLiteral(":feed"), authorDetails.feed);
//         writeQuery.bindValue(QStringLiteral(":id"), authorDetails.id);
//         writeQuery.bindValue(QStringLiteral(":name"), authorDetails.name);
//         writeQuery.bindValue(QStringLiteral(":uri"), authorDetails.uri);
//         writeQuery.bindValue(QStringLiteral(":email"), authorDetails.email);
//         dbExecute(writeQuery);
//     }
//     writeQuery.clear();
//
//     // new enclosures
//     writeQuery.prepare(QStringLiteral("INSERT INTO Enclosures VALUES (:feed, :id, :duration, :size, :title, :type, :url, :playposition, :downloaded);"));
//     for (const EnclosureDetails &enclosureDetails : std::as_const(m_newEnclosures)) {
//         writeQuery.bindValue(QStringLiteral(":feed"), enclosureDetails.feed);
//         writeQuery.bindValue(QStringLiteral(":id"), enclosureDetails.id);
//         writeQuery.bindValue(QStringLiteral(":duration"), enclosureDetails.duration);
//         writeQuery.bindValue(QStringLiteral(":size"), enclosureDetails.size);
//         writeQuery.bindValue(QStringLiteral(":title"), enclosureDetails.title);
//         writeQuery.bindValue(QStringLiteral(":type"), enclosureDetails.type);
//         writeQuery.bindValue(QStringLiteral(":url"), enclosureDetails.url);
//         writeQuery.bindValue(QStringLiteral(":playposition"), enclosureDetails.playPosition);
//         writeQuery.bindValue(QStringLiteral(":downloaded"), Enclosure::statusToDb(enclosureDetails.downloaded));
//         dbExecute(writeQuery);
//     }
//     writeQuery.clear();
//
//     // update enclosures
//     writeQuery.prepare(QStringLiteral("UPDATE Enclosures SET duration=:duration, size=:size, title=:title, type=:type, url=:url WHERE feed=:feed AND
//     id=:id;")); for (const EnclosureDetails &enclosureDetails : std::as_const(m_updateEnclosures)) {
//         writeQuery.bindValue(QStringLiteral(":feed"), enclosureDetails.feed);
//         writeQuery.bindValue(QStringLiteral(":id"), enclosureDetails.id);
//         writeQuery.bindValue(QStringLiteral(":duration"), enclosureDetails.duration);
//         writeQuery.bindValue(QStringLiteral(":size"), enclosureDetails.size);
//         writeQuery.bindValue(QStringLiteral(":title"), enclosureDetails.title);
//         writeQuery.bindValue(QStringLiteral(":type"), enclosureDetails.type);
//         writeQuery.bindValue(QStringLiteral(":url"), enclosureDetails.url);
//         dbExecute(writeQuery);
//     }
//     writeQuery.clear();
//
//     // new chapters
//     writeQuery.prepare(QStringLiteral("INSERT INTO Chapters VALUES(:feed, :id, :start, :title, :link, :image);"));
//     for (const ChapterDetails &chapterDetails : std::as_const(m_newChapters)) {
//         writeQuery.bindValue(QStringLiteral(":feed"), chapterDetails.feed);
//         writeQuery.bindValue(QStringLiteral(":id"), chapterDetails.id);
//         writeQuery.bindValue(QStringLiteral(":start"), chapterDetails.start);
//         writeQuery.bindValue(QStringLiteral(":title"), chapterDetails.title);
//         writeQuery.bindValue(QStringLiteral(":link"), chapterDetails.link);
//         writeQuery.bindValue(QStringLiteral(":image"), chapterDetails.image);
//         dbExecute(writeQuery);
//     }
//     writeQuery.clear();
//
//     // update chapters
//     writeQuery.prepare(QStringLiteral("UPDATE Chapters SET title=:title, link=:link, image=:image WHERE feed=:feed AND id=:id AND start=:start;"));
//     for (const ChapterDetails &chapterDetails : std::as_const(m_updateChapters)) {
//         writeQuery.bindValue(QStringLiteral(":feed"), chapterDetails.feed);
//         writeQuery.bindValue(QStringLiteral(":id"), chapterDetails.id);
//         writeQuery.bindValue(QStringLiteral(":start"), chapterDetails.start);
//         writeQuery.bindValue(QStringLiteral(":title"), chapterDetails.title);
//         writeQuery.bindValue(QStringLiteral(":link"), chapterDetails.link);
//         writeQuery.bindValue(QStringLiteral(":image"), chapterDetails.image);
//         dbExecute(writeQuery);
//     }
//     writeQuery.clear();
//
//     // set custom amount of episodes to unread/new if required
//     if (m_oldFeedDetails.isNew && (SettingsManager::self()->markUnreadOnNewFeed() == 1) && (SettingsManager::self()->markUnreadOnNewFeedCustomAmount() > 0))
//     {
//         writeQuery.prepare(QStringLiteral(
//             "UPDATE Entries SET read=:read, new=:new WHERE id in (SELECT id FROM Entries WHERE feed =:feed ORDER BY updated DESC LIMIT :recentUnread);"));
//         writeQuery.bindValue(QStringLiteral(":feed"), m_url);
//         writeQuery.bindValue(QStringLiteral(":read"), false);
//         writeQuery.bindValue(QStringLiteral(":new"), true);
//         writeQuery.bindValue(QStringLiteral(":recentUnread"), SettingsManager::self()->markUnreadOnNewFeedCustomAmount());
//         dbExecute(writeQuery);
//         writeQuery.clear();
//     }
//
//     if (m_oldFeedDetails.isNew) {
//         // Finally, reset the new flag to false now that the new feed has been
//         // fully processed.  If we would reset the flag sooner, then too many
//         // episodes will get flagged as new if the initial import gets
//         // interrupted somehow.
//         writeQuery.prepare(QStringLiteral("UPDATE Feeds SET new=:new WHERE url=:url;"));
//         writeQuery.bindValue(QStringLiteral(":url"), m_url);
//         writeQuery.bindValue(QStringLiteral(":new"), false);
//         dbExecute(writeQuery);
//         writeQuery.clear();
//     }
//
//     if (m_oldFeedDetails.lastHash != m_feedDetails.lastHash) {
//         writeQuery.prepare(QStringLiteral("UPDATE Feeds SET lastHash=:lastHash WHERE url=:url;"));
//         writeQuery.bindValue(QStringLiteral(":url"), m_url);
//         writeQuery.bindValue(QStringLiteral(":lastHash"), m_feedDetails.lastHash);
//         dbExecute(writeQuery);
//         writeQuery.clear();
//     }
//
//     if (dbCommit()) {
//         QStringList newIds, updateIds;
//
//         // emit signals for new entries
//         for (const EntryDetails &entryDetails : std::as_const(m_newEntries)) {
//             if (!newIds.contains(entryDetails.id)) {
//                 newIds += entryDetails.id;
//             }
//         }
//
//         for (const QString &id : std::as_const(newIds)) {
//             Q_EMIT entryAdded(m_url, id);
//         }
//
//         // emit signals for updated entries or entries with new/updated authors,
//         // enclosures or chapters
//         for (const EntryDetails &entryDetails : std::as_const(m_updateEntries)) {
//             if (!updateIds.contains(entryDetails.id) && !newIds.contains(entryDetails.id)) {
//                 updateIds += entryDetails.id;
//             }
//         }
//         const auto allEnclosures = m_newEnclosures + m_updateEnclosures;
//         for (const EnclosureDetails &enclosureDetails : allEnclosures) {
//             if (!updateIds.contains(enclosureDetails.id) && !newIds.contains(enclosureDetails.id)) {
//                 updateIds += enclosureDetails.id;
//             }
//         }
//         const auto allAuthors = m_newAuthors + m_updateAuthors;
//         for (const AuthorDetails &authorDetails : allAuthors) {
//             if (!updateIds.contains(authorDetails.id) && !newIds.contains(authorDetails.id)) {
//                 updateIds += authorDetails.id;
//             }
//         }
//         const auto allChapters = m_newChapters + m_updateChapters;
//         for (const ChapterDetails &chapterDetails : allChapters) {
//             if (!updateIds.contains(chapterDetails.id) && !newIds.contains(chapterDetails.id)) {
//                 updateIds += chapterDetails.id;
//             }
//         }
//
//         for (const QString &id : std::as_const(updateIds)) {
//             qCDebug(kastsFetcher) << "updated episode" << id;
//             Q_EMIT entryUpdated(m_url, id);
//         }
//     }
// }

// TODO: Remove all the database stuff
bool UpdateFeedJob::dbExecute(QSqlQuery &query)
{
    bool state = Database::executeThread(query);

    if (!state) {
        Q_EMIT error(Error::Type::Database, QString(), QString(), query.lastError().type(), query.lastQuery(), query.lastError().text());
    }

    return state;
}

bool UpdateFeedJob::dbTransaction()
{
    // use raw sqlite query to benefit from automatic retries on execute
    QSqlQuery query(QSqlDatabase::database(m_url));
    query.prepare(QStringLiteral("BEGIN TRANSACTION;"));
    return dbExecute(query);
}

bool UpdateFeedJob::dbCommit()
{
    // use raw sqlite query to benefit from automatic retries on execute
    QSqlQuery query(QSqlDatabase::database(m_url));
    query.prepare(QStringLiteral("COMMIT TRANSACTION;"));
    return dbExecute(query);
}

QString UpdateFeedJob::generateFeedDirname(const QString &name)
{
    // Generate directory name for enclosures based on feed name
    // NOTE: Any changes here require a database migration!
    QString dirBaseName = StorageManager::instance().sanitizedFilePath(name);
    QString dirName = dirBaseName;

    QStringList dirNameList;
    QSqlQuery query(QSqlDatabase::database(m_url));
    query.prepare(QStringLiteral("SELECT name FROM Feeds;"));
    dbExecute(query);
    while (query.next()) {
        dirNameList << query.value(QStringLiteral("name")).toString();
    }

    // Check for duplicate names in database and on filesystem
    int numDups = 1; // Minimum to append is " (1)" if file already exists
    while (dirNameList.contains(dirName) || QDir(StorageManager::instance().enclosureDirPath() + dirName).exists()) {
        dirName = QStringLiteral("%1 (%2)").arg(dirBaseName, QString::number(numDups));
        numDups++;
    }
    return dirName;
}

void UpdateFeedJob::abort()
{
    m_abort = true;
    Q_EMIT aborting();
}
