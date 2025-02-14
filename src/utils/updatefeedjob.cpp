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
#include "kasts-version.h"
#include "settingsmanager.h"
#include "storagemanager.h"

using namespace ThreadWeaver;
using namespace DataTypes;

UpdateFeedJob::UpdateFeedJob(const QString &url, const QByteArray &data, const FeedDetails &feed, QObject *parent)
    : QObject(parent)
    , m_url(url)
    , m_data(data)
    , m_feed(feed)
{
    // connect to signals in Fetcher such that GUI can pick up the changes
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
}

void UpdateFeedJob::processFeed(Syndication::FeedPtr feed)
{
    qCDebug(kastsFetcher) << "start process feed" << feed;

    if (feed.isNull())
        return;

    // First check if this is a newly added feed and get current name and dirname
    if (m_feed.isNew) {
        qCDebug(kastsFetcher) << "New feed" << feed->title();
    }

    m_markUnreadOnNewFeed = !(SettingsManager::self()->markUnreadOnNewFeed() == 2);
    QDateTime current = QDateTime::currentDateTime();

    m_updateFeed = m_feed; // start from current feed details in database

    m_updateFeed.name = feed->title();
    m_updateFeed.link = feed->link();
    m_updateFeed.description = feed->description();
    m_updateFeed.lastUpdated = current.toSecsSinceEpoch();
    m_updateFeed.lastHash = QString::fromLatin1(QCryptographicHash::hash(m_data, QCryptographicHash::Sha256).toHex());

    // Retrieve "other" fields; this will include the "itunes" tags
    QMultiMap<QString, QDomElement> otherItems = feed->additionalProperties();

    // First try the itunes tags, if not, fall back to regular image tag
    if (otherItems.value(QStringLiteral("http://www.itunes.com/dtds/podcast-1.0.dtdimage")).hasAttribute(QStringLiteral("href"))) {
        m_updateFeed.image = otherItems.value(QStringLiteral("http://www.itunes.com/dtds/podcast-1.0.dtdimage")).attribute(QStringLiteral("href"));
    } else {
        m_updateFeed.image = feed->image()->url();
    }

    if (m_updateFeed.image.startsWith(QStringLiteral("/"))) {
        m_updateFeed.image = QUrl(m_url).adjusted(QUrl::RemovePath).toString() + m_updateFeed.image;
    }

    // if the title has changed, we need to rename the corresponding enclosure
    // download directory name and move the files
    if (m_feed.name != m_updateFeed.name || m_feed.dirname.isEmpty() || m_feed.isNew) {
        QString generatedDirname = generateFeedDirname(m_updateFeed.name);
        if (generatedDirname != m_feed.dirname) {
            m_updateFeed.dirname = generatedDirname;
            QString enclosurePath = StorageManager::instance().enclosureDirPath();
            if (QDir(enclosurePath + m_feed.dirname).exists()) {
                QDir().rename(enclosurePath + m_feed.dirname, enclosurePath + m_updateFeed.dirname);
            } else {
                QDir().mkpath(enclosurePath + m_updateFeed.dirname);
            }
        }
    }

    QSqlQuery query(QSqlDatabase::database(m_url));
    query.prepare(QStringLiteral(
        "UPDATE Feeds SET name=:name, image=:image, link=:link, description=:description, lastUpdated=:lastUpdated, dirname=:dirname WHERE url=:url;"));
    query.bindValue(QStringLiteral(":name"), m_updateFeed.name);
    query.bindValue(QStringLiteral(":url"), m_updateFeed.url);
    query.bindValue(QStringLiteral(":link"), m_updateFeed.link);
    query.bindValue(QStringLiteral(":description"), m_updateFeed.description);
    query.bindValue(QStringLiteral(":lastUpdated"), m_updateFeed.lastUpdated);
    query.bindValue(QStringLiteral(":image"), m_updateFeed.image);
    query.bindValue(QStringLiteral(":dirname"), m_updateFeed.dirname);
    // we only write the new lastHash to the database after entries etc. have
    // all been updated!
    dbExecute(query);
    query.clear(); // make sure this query is not blocking anything anymore

    // Now that we have the feed details, we make vectors of the data that's
    // already in the database relating to this feed
    // NOTE: We will do the feed authors after this step, because otherwise
    // we can't check for duplicates and we'll keep adding more of the same!
    query.prepare(QStringLiteral("SELECT * FROM Entries WHERE feed=:feed;"));
    query.bindValue(QStringLiteral(":feed"), m_url);
    dbExecute(query);
    while (query.next()) {
        EntryDetails entryDetails;
        entryDetails.feed = m_url;
        entryDetails.id = query.value(QStringLiteral("id")).toString();
        entryDetails.title = query.value(QStringLiteral("title")).toString();
        entryDetails.content = query.value(QStringLiteral("content")).toString();
        entryDetails.created = query.value(QStringLiteral("created")).toInt();
        entryDetails.updated = query.value(QStringLiteral("updated")).toInt();
        entryDetails.read = query.value(QStringLiteral("read")).toBool();
        entryDetails.isNew = query.value(QStringLiteral("new")).toBool();
        entryDetails.link = query.value(QStringLiteral("link")).toString();
        entryDetails.hasEnclosure = query.value(QStringLiteral("hasEnclosure")).toBool();
        entryDetails.image = query.value(QStringLiteral("image")).toString();
        m_entries += entryDetails;
    }
    query.clear();

    query.prepare(QStringLiteral("SELECT * FROM Enclosures WHERE feed=:feed;"));
    query.bindValue(QStringLiteral(":feed"), m_url);
    dbExecute(query);
    while (query.next()) {
        EnclosureDetails enclosureDetails;
        enclosureDetails.feed = m_url;
        enclosureDetails.id = query.value(QStringLiteral("id")).toString();
        enclosureDetails.duration = query.value(QStringLiteral("duration")).toInt();
        enclosureDetails.size = query.value(QStringLiteral("size")).toInt();
        enclosureDetails.title = query.value(QStringLiteral("title")).toString();
        enclosureDetails.type = query.value(QStringLiteral("type")).toString();
        enclosureDetails.url = query.value(QStringLiteral("url")).toString();
        enclosureDetails.playPosition = query.value(QStringLiteral("id")).toInt();
        enclosureDetails.downloaded = Enclosure::dbToStatus(query.value(QStringLiteral("downloaded")).toInt());
        m_enclosures += enclosureDetails;
    }
    query.clear();

    query.prepare(QStringLiteral("SELECT * FROM Authors WHERE feed=:feed;"));
    query.bindValue(QStringLiteral(":feed"), m_url);
    dbExecute(query);
    while (query.next()) {
        AuthorDetails authorDetails;
        authorDetails.feed = m_url;
        authorDetails.id = query.value(QStringLiteral("id")).toString();
        authorDetails.name = query.value(QStringLiteral("name")).toString();
        authorDetails.uri = query.value(QStringLiteral("uri")).toString();
        authorDetails.email = query.value(QStringLiteral("email")).toString();
        m_authors += authorDetails;
    }
    query.clear();

    query.prepare(QStringLiteral("SELECT * FROM Chapters WHERE feed=:feed;"));
    query.bindValue(QStringLiteral(":feed"), m_url);
    dbExecute(query);
    while (query.next()) {
        ChapterDetails chapterDetails;
        chapterDetails.feed = m_url;
        chapterDetails.id = query.value(QStringLiteral("id")).toString();
        chapterDetails.start = query.value(QStringLiteral("start")).toInt();
        chapterDetails.title = query.value(QStringLiteral("title")).toString();
        chapterDetails.link = query.value(QStringLiteral("link")).toString();
        chapterDetails.image = query.value(QStringLiteral("image")).toString();
        m_chapters += chapterDetails;
    }
    query.clear();

    bool authorsChanged = false;
    // Process feed authors
    if (feed->authors().count() > 0) {
        for (auto &author : feed->authors()) {
            authorsChanged = authorsChanged || processAuthor(QLatin1String(""), author->name(), QLatin1String(""), QLatin1String(""));
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
            authorsChanged = authorsChanged || processAuthor(QLatin1String(""), authorname, QLatin1String(""), authoremail);
        }
    }

    qCDebug(kastsFetcher) << "Updated feed details:" << feed->title();

    // check if any field has changed and only emit signal if there are changes
    bool hasFeedBeenUpdated = false;
    if (m_updateFeed.name != m_feed.name || m_updateFeed.link != m_feed.link || m_updateFeed.image != m_feed.image
        || m_updateFeed.description != m_feed.description || authorsChanged) {
        hasFeedBeenUpdated = true;
    } else {
        hasFeedBeenUpdated = false;
    }

    if (hasFeedBeenUpdated) {
        Q_EMIT feedDetailsUpdated(m_url, m_updateFeed.name, m_updateFeed.image, m_updateFeed.link, m_updateFeed.description, current, m_updateFeed.dirname);
    }

    if (m_abort)
        return;

    // Now deal with the entries, enclosures, entry authors and chapter marks
    bool updatedEntries = false;
    for (const auto &entry : feed->items()) {
        if (m_abort)
            return;

        bool isNewEntry = processEntry(entry);
        updatedEntries = updatedEntries || isNewEntry;
    }

    writeToDatabase();

    if (updatedEntries || m_feed.isNew) {
        Q_EMIT feedUpdated(m_url);
    }

    qCDebug(kastsFetcher) << "done processing feed" << feed;
}

bool UpdateFeedJob::processEntry(Syndication::ItemPtr entry)
{
    qCDebug(kastsFetcher) << "Processing" << entry->title();
    bool isNewEntry = true;
    bool isUpdateEntry = false;
    bool isUpdateDependencies = false;
    EntryDetails currentEntry;

    // check against existing entries and the list of new entries
    for (const EntryDetails &entryDetails : (m_entries + m_newEntries)) {
        if (entryDetails.id == entry->id()) {
            isNewEntry = false;
            currentEntry = entryDetails;
        }
    }

    // stop here if doFullUpdate is set to false and this is an existing entry
    if (!isNewEntry && !SettingsManager::self()->doFullUpdate()) {
        return false;
    }

    // Retrieve "other" fields; this will include the "itunes" tags
    QMultiMap<QString, QDomElement> otherItems = entry->additionalProperties();

    for (const QString &key : otherItems.uniqueKeys()) {
        qCDebug(kastsFetcher) << "other elements";
        qCDebug(kastsFetcher) << key << otherItems.value(key).tagName();
    }

    EntryDetails entryDetails;
    entryDetails.feed = m_url;
    entryDetails.id = entry->id();
    entryDetails.title = QTextDocumentFragment::fromHtml(entry->title()).toPlainText();
    entryDetails.created = static_cast<int>(entry->datePublished());
    entryDetails.updated = static_cast<int>(entry->dateUpdated());
    entryDetails.link = entry->link();
    entryDetails.hasEnclosure = (entry->enclosures().length() > 0);
    entryDetails.read = m_feed.isNew ? m_markUnreadOnNewFeed : false; // if new feed, then check settings
    entryDetails.isNew = !m_feed.isNew; // if new feed, then mark none as new

    // Take the longest text, either content or description
    if (entry->content().length() > entry->description().length()) {
        entryDetails.content = entry->content();
    } else {
        entryDetails.content = entry->description();
    }

    // Look for image in itunes tags
    if (otherItems.value(QStringLiteral("http://www.itunes.com/dtds/podcast-1.0.dtdimage")).hasAttribute(QStringLiteral("href"))) {
        entryDetails.image = otherItems.value(QStringLiteral("http://www.itunes.com/dtds/podcast-1.0.dtdimage")).attribute(QStringLiteral("href"));
    } else if (otherItems.contains(QStringLiteral("http://search.yahoo.com/mrss/thumbnail"))) {
        entryDetails.image = otherItems.value(QStringLiteral("http://search.yahoo.com/mrss/thumbnail")).attribute(QStringLiteral("url"));
    }
    if (entryDetails.image.startsWith(QStringLiteral("/"))) {
        entryDetails.image = QUrl(m_url).adjusted(QUrl::RemovePath).toString() + entryDetails.image;
    }
    qCDebug(kastsFetcher) << "Entry image found" << entryDetails.image;

    // if this is an existing episode, check if it needs updating
    if (!isNewEntry) {
        if ((currentEntry.title != entryDetails.title) || (currentEntry.content != entryDetails.content) || (currentEntry.created != entryDetails.created)
            || (currentEntry.updated != entryDetails.updated) || (currentEntry.link != entryDetails.link)
            || (currentEntry.hasEnclosure != entryDetails.hasEnclosure) || (currentEntry.image != entryDetails.image)) {
            qCDebug(kastsFetcher) << "episode details have been updated:" << entry->id();
            isUpdateEntry = true;
            m_updateEntries += entryDetails;
        } else {
            qCDebug(kastsFetcher) << "episode details are unchanged:" << entry->id();
        }
    } else {
        qCDebug(kastsFetcher) << "this is a new episode:" << entry->id();
        m_newEntries += entryDetails;
    }

    // Process authors
    if (entry->authors().count() > 0) {
        for (const auto &author : entry->authors()) {
            isUpdateDependencies = isUpdateDependencies | processAuthor(entry->id(), author->name(), author->uri(), author->email());
        }
    } else {
        // As fallback, check if there is itunes "author" information
        QString authorName = otherItems.value(QStringLiteral("http://www.itunes.com/dtds/podcast-1.0.dtdauthor")).text();
        if (!authorName.isEmpty())
            isUpdateDependencies = isUpdateDependencies | processAuthor(entry->id(), authorName, QLatin1String(""), QLatin1String(""));
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
                for (QString part : startParts) {
                    // strip off decimal point if it's present
                    startInt = part.split(QStringLiteral("."))[0].toInt() + startInt * 60;
                }
                qCDebug(kastsFetcher) << "Found chapter mark:" << start << "; in seconds:" << startInt;
                QString images = element.attribute(QStringLiteral("image"));
                isUpdateDependencies = isUpdateDependencies | processChapter(entry->id(), startInt, title, entry->link(), images);
            }
        }
    }

    // Process enclosures
    // only process first enclosure if there are multiple (e.g. mp3 and ogg);
    // the first one is probably the podcast author's preferred version
    // TODO: handle more than one enclosure?
    if (entry->enclosures().count() > 0) {
        isUpdateDependencies = isUpdateDependencies | processEnclosure(entry->enclosures()[0], entryDetails, currentEntry);
    }

    return isNewEntry | isUpdateEntry | isUpdateDependencies; // this is a new or updated entry, or an enclosure, chapter or author has been changed/added
}

bool UpdateFeedJob::processAuthor(const QString &entryId, const QString &authorName, const QString &authorUri, const QString &authorEmail)
{
    bool isNewAuthor = true;
    bool isUpdateAuthor = false;
    AuthorDetails currentAuthor;

    // check against existing authors already in database
    for (const AuthorDetails &authorDetails : (m_authors + m_newAuthors)) {
        if ((authorDetails.id == entryId) && (authorDetails.name == authorName)) {
            isNewAuthor = false;
            currentAuthor = authorDetails;
        }
    }

    AuthorDetails authorDetails;
    authorDetails.feed = m_url;
    authorDetails.id = entryId;
    authorDetails.name = authorName;
    authorDetails.uri = authorUri;
    authorDetails.email = authorEmail;

    if (!isNewAuthor) {
        if ((currentAuthor.uri != authorDetails.uri) || (currentAuthor.email != authorDetails.email)) {
            qCDebug(kastsFetcher) << "author details have been updated for:" << entryId << authorName;
            isUpdateAuthor = true;
            m_updateAuthors += authorDetails;
        } else {
            qCDebug(kastsFetcher) << "author details are unchanged:" << entryId << authorName;
        }
    } else {
        qCDebug(kastsFetcher) << "this is a new author:" << entryId << authorName;
        m_newAuthors += authorDetails;
    }

    return isNewAuthor | isUpdateAuthor;
}

bool UpdateFeedJob::processEnclosure(Syndication::EnclosurePtr enclosure, const EntryDetails &newEntry, const EntryDetails &oldEntry)
{
    bool isNewEnclosure = true;
    bool isUpdateEnclosure = false;
    EnclosureDetails currentEnclosure;

    // check against existing enclosures already in database
    for (const EnclosureDetails &enclosureDetails : (m_enclosures + m_newEnclosures)) {
        if (enclosureDetails.id == newEntry.id) {
            isNewEnclosure = false;
            currentEnclosure = enclosureDetails;
        }
    }

    EnclosureDetails enclosureDetails;
    enclosureDetails.feed = m_url;
    enclosureDetails.id = newEntry.id;
    enclosureDetails.duration = enclosure->duration();
    enclosureDetails.size = enclosure->length();
    enclosureDetails.title = enclosure->title();
    enclosureDetails.type = enclosure->type();
    enclosureDetails.url = enclosure->url();
    enclosureDetails.playPosition = 0;
    enclosureDetails.downloaded = Enclosure::Downloadable;

    if (!isNewEnclosure) {
        if ((currentEnclosure.url != enclosureDetails.url) || (currentEnclosure.title != enclosureDetails.title)
            || (currentEnclosure.type != enclosureDetails.type)) {
            qCDebug(kastsFetcher) << "enclosure details have been updated for:" << newEntry.id;
            isUpdateEnclosure = true;
            m_updateEnclosures += enclosureDetails;
        } else {
            qCDebug(kastsFetcher) << "enclosure details are unchanged:" << newEntry.id;
        }

        // Check if entry title or enclosure URL has changed
        if (newEntry.title != oldEntry.title) {
            QString oldFilename = StorageManager::instance().enclosurePath(oldEntry.title, currentEnclosure.url, m_updateFeed.dirname);
            QString newFilename = StorageManager::instance().enclosurePath(newEntry.title, enclosureDetails.url, m_updateFeed.dirname);

            if (oldFilename != newFilename) {
                if (currentEnclosure.url == enclosureDetails.url) {
                    // If entry title has changed but URL is still the same, the existing enclosure needs to be renamed
                    QFile::rename(oldFilename, newFilename);
                } else {
                    // If enclosure URL has changed, the old enclosure needs to be deleted
                    if (QFile(oldFilename).exists()) {
                        QFile(oldFilename).remove();
                    }
                }
            }
        }
    } else {
        qCDebug(kastsFetcher) << "this is a new enclosure:" << newEntry.id;
        m_newEnclosures += enclosureDetails;
    }

    return isNewEnclosure | isUpdateEnclosure;
}

bool UpdateFeedJob::processChapter(const QString &entryId, const int &start, const QString &chapterTitle, const QString &link, const QString &image)
{
    bool isNewChapter = true;
    bool isUpdateChapter = false;
    ChapterDetails currentChapter;

    // check against existing enclosures already in database
    for (const ChapterDetails &chapterDetails : (m_chapters + m_newChapters)) {
        if ((chapterDetails.id == entryId) && (chapterDetails.start == start)) {
            isNewChapter = false;
            currentChapter = chapterDetails;
        }
    }

    ChapterDetails chapterDetails;
    chapterDetails.feed = m_url;
    chapterDetails.id = entryId;
    chapterDetails.start = start;
    chapterDetails.title = chapterTitle;
    chapterDetails.link = link;
    chapterDetails.image = image;

    if (!isNewChapter) {
        if ((currentChapter.title != chapterDetails.title) || (currentChapter.link != chapterDetails.link) || (currentChapter.image != chapterDetails.image)) {
            qCDebug(kastsFetcher) << "chapter details have been updated for:" << entryId << start;
            isUpdateChapter = true;
            m_updateChapters += chapterDetails;
        } else {
            qCDebug(kastsFetcher) << "chapter details are unchanged:" << entryId << start;
        }
    } else {
        qCDebug(kastsFetcher) << "this is a new chapter:" << entryId << start;
        m_newChapters += chapterDetails;
    }

    return isNewChapter | isUpdateChapter;
}

void UpdateFeedJob::writeToDatabase()
{
    QSqlQuery writeQuery(QSqlDatabase::database(m_url));

    dbTransaction();

    // new entries
    writeQuery.prepare(
        QStringLiteral("INSERT INTO Entries VALUES (:feed, :id, :title, :content, :created, :updated, :link, :read, :new, :hasEnclosure, :image, :favorite);"));
    for (const EntryDetails &entryDetails : m_newEntries) {
        writeQuery.bindValue(QStringLiteral(":feed"), entryDetails.feed);
        writeQuery.bindValue(QStringLiteral(":id"), entryDetails.id);
        writeQuery.bindValue(QStringLiteral(":title"), entryDetails.title);
        writeQuery.bindValue(QStringLiteral(":content"), entryDetails.content);
        writeQuery.bindValue(QStringLiteral(":created"), entryDetails.created);
        writeQuery.bindValue(QStringLiteral(":updated"), entryDetails.updated);
        writeQuery.bindValue(QStringLiteral(":link"), entryDetails.link);
        writeQuery.bindValue(QStringLiteral(":hasEnclosure"), entryDetails.hasEnclosure);
        writeQuery.bindValue(QStringLiteral(":read"), entryDetails.read);
        writeQuery.bindValue(QStringLiteral(":new"), entryDetails.isNew);
        writeQuery.bindValue(QStringLiteral(":image"), entryDetails.image);
        writeQuery.bindValue(QStringLiteral(":favorite"), false);
        dbExecute(writeQuery);
    }
    writeQuery.clear();

    // update entries
    writeQuery.prepare(
        QStringLiteral("UPDATE Entries SET title=:title, content=:content, created=:created, updated=:updated, link=:link, hasEnclosure=:hasEnclosure, "
                       "image=:image WHERE id=:id AND feed=:feed;"));
    for (const EntryDetails &entryDetails : m_updateEntries) {
        writeQuery.bindValue(QStringLiteral(":feed"), entryDetails.feed);
        writeQuery.bindValue(QStringLiteral(":id"), entryDetails.id);
        writeQuery.bindValue(QStringLiteral(":title"), entryDetails.title);
        writeQuery.bindValue(QStringLiteral(":content"), entryDetails.content);
        writeQuery.bindValue(QStringLiteral(":created"), entryDetails.created);
        writeQuery.bindValue(QStringLiteral(":updated"), entryDetails.updated);
        writeQuery.bindValue(QStringLiteral(":link"), entryDetails.link);
        writeQuery.bindValue(QStringLiteral(":hasEnclosure"), entryDetails.hasEnclosure);
        writeQuery.bindValue(QStringLiteral(":image"), entryDetails.image);
        dbExecute(writeQuery);
    }
    writeQuery.clear();

    // new authors
    writeQuery.prepare(QStringLiteral("INSERT INTO Authors VALUES(:feed, :id, :name, :uri, :email);"));
    for (const AuthorDetails &authorDetails : m_newAuthors) {
        writeQuery.bindValue(QStringLiteral(":feed"), authorDetails.feed);
        writeQuery.bindValue(QStringLiteral(":id"), authorDetails.id);
        writeQuery.bindValue(QStringLiteral(":name"), authorDetails.name);
        writeQuery.bindValue(QStringLiteral(":uri"), authorDetails.uri);
        writeQuery.bindValue(QStringLiteral(":email"), authorDetails.email);
        dbExecute(writeQuery);
    }
    writeQuery.clear();

    // update authors
    writeQuery.prepare(QStringLiteral("UPDATE Authors SET uri=:uri, email=:email WHERE feed=:feed AND id=:id AND name=:name;"));
    for (const AuthorDetails &authorDetails : m_updateAuthors) {
        writeQuery.bindValue(QStringLiteral(":feed"), authorDetails.feed);
        writeQuery.bindValue(QStringLiteral(":id"), authorDetails.id);
        writeQuery.bindValue(QStringLiteral(":name"), authorDetails.name);
        writeQuery.bindValue(QStringLiteral(":uri"), authorDetails.uri);
        writeQuery.bindValue(QStringLiteral(":email"), authorDetails.email);
        dbExecute(writeQuery);
    }
    writeQuery.clear();

    // new enclosures
    writeQuery.prepare(QStringLiteral("INSERT INTO Enclosures VALUES (:feed, :id, :duration, :size, :title, :type, :url, :playposition, :downloaded);"));
    for (const EnclosureDetails &enclosureDetails : m_newEnclosures) {
        writeQuery.bindValue(QStringLiteral(":feed"), enclosureDetails.feed);
        writeQuery.bindValue(QStringLiteral(":id"), enclosureDetails.id);
        writeQuery.bindValue(QStringLiteral(":duration"), enclosureDetails.duration);
        writeQuery.bindValue(QStringLiteral(":size"), enclosureDetails.size);
        writeQuery.bindValue(QStringLiteral(":title"), enclosureDetails.title);
        writeQuery.bindValue(QStringLiteral(":type"), enclosureDetails.type);
        writeQuery.bindValue(QStringLiteral(":url"), enclosureDetails.url);
        writeQuery.bindValue(QStringLiteral(":playposition"), enclosureDetails.playPosition);
        writeQuery.bindValue(QStringLiteral(":downloaded"), Enclosure::statusToDb(enclosureDetails.downloaded));
        dbExecute(writeQuery);
    }
    writeQuery.clear();

    // update enclosures
    writeQuery.prepare(QStringLiteral("UPDATE Enclosures SET duration=:duration, size=:size, title=:title, type=:type, url=:url WHERE feed=:feed AND id=:id;"));
    for (const EnclosureDetails &enclosureDetails : m_updateEnclosures) {
        writeQuery.bindValue(QStringLiteral(":feed"), enclosureDetails.feed);
        writeQuery.bindValue(QStringLiteral(":id"), enclosureDetails.id);
        writeQuery.bindValue(QStringLiteral(":duration"), enclosureDetails.duration);
        writeQuery.bindValue(QStringLiteral(":size"), enclosureDetails.size);
        writeQuery.bindValue(QStringLiteral(":title"), enclosureDetails.title);
        writeQuery.bindValue(QStringLiteral(":type"), enclosureDetails.type);
        writeQuery.bindValue(QStringLiteral(":url"), enclosureDetails.url);
        dbExecute(writeQuery);
    }
    writeQuery.clear();

    // new chapters
    writeQuery.prepare(QStringLiteral("INSERT INTO Chapters VALUES(:feed, :id, :start, :title, :link, :image);"));
    for (const ChapterDetails &chapterDetails : m_newChapters) {
        writeQuery.bindValue(QStringLiteral(":feed"), chapterDetails.feed);
        writeQuery.bindValue(QStringLiteral(":id"), chapterDetails.id);
        writeQuery.bindValue(QStringLiteral(":start"), chapterDetails.start);
        writeQuery.bindValue(QStringLiteral(":title"), chapterDetails.title);
        writeQuery.bindValue(QStringLiteral(":link"), chapterDetails.link);
        writeQuery.bindValue(QStringLiteral(":image"), chapterDetails.image);
        dbExecute(writeQuery);
    }
    writeQuery.clear();

    // update chapters
    writeQuery.prepare(QStringLiteral("UPDATE Chapters SET title=:title, link=:link, image=:image WHERE feed=:feed AND id=:id AND start=:start;"));
    for (const ChapterDetails &chapterDetails : m_updateChapters) {
        writeQuery.bindValue(QStringLiteral(":feed"), chapterDetails.feed);
        writeQuery.bindValue(QStringLiteral(":id"), chapterDetails.id);
        writeQuery.bindValue(QStringLiteral(":start"), chapterDetails.start);
        writeQuery.bindValue(QStringLiteral(":title"), chapterDetails.title);
        writeQuery.bindValue(QStringLiteral(":link"), chapterDetails.link);
        writeQuery.bindValue(QStringLiteral(":image"), chapterDetails.image);
        dbExecute(writeQuery);
    }
    writeQuery.clear();

    // set custom amount of episodes to unread/new if required
    if (m_feed.isNew && (SettingsManager::self()->markUnreadOnNewFeed() == 1) && (SettingsManager::self()->markUnreadOnNewFeedCustomAmount() > 0)) {
        writeQuery.prepare(QStringLiteral(
            "UPDATE Entries SET read=:read, new=:new WHERE id in (SELECT id FROM Entries WHERE feed =:feed ORDER BY updated DESC LIMIT :recentUnread);"));
        writeQuery.bindValue(QStringLiteral(":feed"), m_url);
        writeQuery.bindValue(QStringLiteral(":read"), false);
        writeQuery.bindValue(QStringLiteral(":new"), true);
        writeQuery.bindValue(QStringLiteral(":recentUnread"), SettingsManager::self()->markUnreadOnNewFeedCustomAmount());
        dbExecute(writeQuery);
        writeQuery.clear();
    }

    if (m_feed.isNew) {
        // Finally, reset the new flag to false now that the new feed has been
        // fully processed.  If we would reset the flag sooner, then too many
        // episodes will get flagged as new if the initial import gets
        // interrupted somehow.
        writeQuery.prepare(QStringLiteral("UPDATE Feeds SET new=:new WHERE url=:url;"));
        writeQuery.bindValue(QStringLiteral(":url"), m_url);
        writeQuery.bindValue(QStringLiteral(":new"), false);
        dbExecute(writeQuery);
        writeQuery.clear();
    }

    if (m_feed.lastHash != m_updateFeed.lastHash) {
        writeQuery.prepare(QStringLiteral("UPDATE Feeds SET lastHash=:lastHash WHERE url=:url;"));
        writeQuery.bindValue(QStringLiteral(":url"), m_url);
        writeQuery.bindValue(QStringLiteral(":lastHash"), m_updateFeed.lastHash);
        dbExecute(writeQuery);
        writeQuery.clear();
    }

    if (dbCommit()) {
        QStringList newIds, updateIds;

        // emit signals for new entries
        for (const EntryDetails &entryDetails : m_newEntries) {
            if (!newIds.contains(entryDetails.id)) {
                newIds += entryDetails.id;
            }
        }

        for (const QString &id : newIds) {
            Q_EMIT entryAdded(m_url, id);
        }

        // emit signals for updated entries or entries with new/updated authors,
        // enclosures or chapters
        for (const EntryDetails &entryDetails : m_updateEntries) {
            if (!updateIds.contains(entryDetails.id) && !newIds.contains(entryDetails.id)) {
                updateIds += entryDetails.id;
            }
        }
        for (const EnclosureDetails &enclosureDetails : (m_newEnclosures + m_updateEnclosures)) {
            if (!updateIds.contains(enclosureDetails.id) && !newIds.contains(enclosureDetails.id)) {
                updateIds += enclosureDetails.id;
            }
        }
        for (const AuthorDetails &authorDetails : (m_newAuthors + m_updateAuthors)) {
            if (!updateIds.contains(authorDetails.id) && !newIds.contains(authorDetails.id)) {
                updateIds += authorDetails.id;
            }
        }
        for (const ChapterDetails &chapterDetails : (m_newChapters + m_updateChapters)) {
            if (!updateIds.contains(chapterDetails.id) && !newIds.contains(chapterDetails.id)) {
                updateIds += chapterDetails.id;
            }
        }

        for (const QString &id : updateIds) {
            qCDebug(kastsFetcher) << "updated episode" << id;
            Q_EMIT entryUpdated(m_url, id);
        }
    }
}

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
