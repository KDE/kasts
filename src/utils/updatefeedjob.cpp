/**
 * SPDX-FileCopyrightText: 2021-2022 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "updatefeedjob.h"

#include <QCryptographicHash>
#include <QDir>
#include <QDomElement>
#include <QHash>
#include <QList>
#include <QLoggingCategory>
#include <QMultiHash>
#include <QMultiMap>
#include <QNetworkReply>
#include <QObject>
#include <QSet>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>
#include <QTextDocumentFragment>
#include <QTimer>

#include <KLocalizedString>
#include <ThreadWeaver/Thread>

#include "database.h"
#include "datatypes.h"
#include "enclosure.h"
#include "error.h"
#include "fetcher.h"
#include "settingsmanager.h"
#include "storagemanager.h"
#include "updaterlogging.h"

using namespace ThreadWeaver;
using namespace DataTypes;

UpdateFeedJob::UpdateFeedJob(const QString &url, const QByteArray &data, QObject *parent)
    : QObject(parent)
    , m_url(url)
    , m_data(data)
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
    qCDebug(kastsUpdater) << "get old feed data from DB for" << m_url;

    // First get all the required data from the database and do some basic checks
    QSqlQuery query(QSqlDatabase::database(m_url));
    query.prepare(QStringLiteral("SELECT * FROM Feeds WHERE url=:url;"));
    query.bindValue(QStringLiteral(":url"), m_url);
    if (!dbExecute(query)) {
        return;
    }
    if (query.next()) {
        DataTypes::FeedDetails feedDetail;
        m_feed.feeduid = query.value(QStringLiteral("feeduid")).toLongLong();
        m_feed.name = query.value(QStringLiteral("name")).toString();
        m_feed.url = m_url;
        m_feed.image = query.value(QStringLiteral("image")).toString();
        m_feed.link = query.value(QStringLiteral("link")).toString();
        m_feed.description = query.value(QStringLiteral("description")).toString();
        m_feed.subscribed = query.value(QStringLiteral("subscribed")).toInt();
        m_feed.lastUpdated = query.value(QStringLiteral("lastUpdated")).toInt();
        m_feed.isNew = query.value(QStringLiteral("new")).toBool();
        m_feed.dirname = query.value(QStringLiteral("dirname")).toString();
        m_feed.lastHash = query.value(QStringLiteral("lastHash")).toString();
        m_feed.filterType = query.value(QStringLiteral("filterType")).toInt();
        m_feed.sortType = query.value(QStringLiteral("sortType")).toInt();
        m_feed.state = RecordState::Unmodified;

        // already set the content of all the "old" fields
        m_feed.oldName = m_feed.name;
        m_feed.oldUrl = m_feed.url;
        m_feed.oldImage = m_feed.image;
        m_feed.oldLink = m_feed.link;
        m_feed.oldDescription = m_feed.description;
        m_feed.oldLastUpdated = m_feed.lastUpdated;
        m_feed.oldDirname = m_feed.dirname;
        m_feed.oldLastHash = m_feed.lastHash;
    } else {
        return;
    }
    query.finish(); // release lock on database

    // now we do the feed authors
    query.prepare(QStringLiteral("SELECT name, email FROM FeedAuthors WHERE feeduid=:feed;"));
    query.bindValue(QStringLiteral(":feeduid"), m_feed.feeduid);
    dbExecute(query);
    while (query.next()) {
        AuthorDetails authorDetails;
        authorDetails.name = query.value(QStringLiteral("name")).toString();
        authorDetails.email = query.value(QStringLiteral("email")).toString();
        authorDetails.state = RecordState::Deleted; // will be reset to Unmodified if the author is found in the updated rss feed

        // already set the content of all the "old" fields
        authorDetails.oldEmail = authorDetails.email;

        m_feed.authors[authorDetails.name] = authorDetails;
    }
    query.finish();

    // Now that we have the feed details, we make vectors of the data that's
    // already in the database relating to this feed
    query.prepare(QStringLiteral("SELECT * FROM Entries WHERE feeduid=:feeduid;"));
    query.bindValue(QStringLiteral(":feeduid"), m_feed.feeduid);
    dbExecute(query);
    while (query.next()) {
        EntryDetails entryDetails;
        entryDetails.entryuid = query.value(QStringLiteral("entryuid")).toLongLong();
        entryDetails.feeduid = query.value(QStringLiteral("feeduid")).toLongLong();
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
        entryDetails.state = RecordState::Deleted; // will be set to appropriate value if the entry is found in the updated rss feed

        // already set the content of all the "old" fields
        entryDetails.oldTitle = entryDetails.title;
        entryDetails.oldContent = entryDetails.content;
        entryDetails.oldCreated = entryDetails.created;
        entryDetails.oldUpdated = entryDetails.updated;
        entryDetails.oldLink = entryDetails.link;
        entryDetails.oldHasEnclosure = entryDetails.hasEnclosure;
        entryDetails.oldImage = entryDetails.image;

        m_feed.entries[entryDetails.id] = entryDetails;
    }
    query.finish();

    query.prepare(QStringLiteral("SELECT * FROM Enclosures JOIN Entries ON Entries.entryuid = Enclosures.entryuid WHERE Enclosures.feeduid=:feeduid;"));
    query.bindValue(QStringLiteral(":feeduid"), m_feed.feeduid);
    dbExecute(query);
    while (query.next()) {
        EnclosureDetails enclosureDetails;
        enclosureDetails.enclosureuid = query.value(QStringLiteral("enclosureuid")).toLongLong();
        QString id = query.value(QStringLiteral("id")).toString();
        enclosureDetails.duration = query.value(QStringLiteral("duration")).toInt();
        enclosureDetails.size = query.value(QStringLiteral("size")).toInt();
        enclosureDetails.type = query.value(QStringLiteral("type")).toString();
        enclosureDetails.url = query.value(QStringLiteral("url")).toString();
        enclosureDetails.playPosition = query.value(QStringLiteral("playposition")).toInt();
        enclosureDetails.downloaded = Enclosure::dbToStatus(query.value(QStringLiteral("downloaded")).toInt());
        enclosureDetails.state = RecordState::Deleted; // will be set to appropriate value if the enclosure is found in the updated rss feed

        // already set the content of all the "old" fields
        enclosureDetails.oldDuration = enclosureDetails.duration;
        enclosureDetails.oldSize = enclosureDetails.size;
        enclosureDetails.oldType = enclosureDetails.type;
        enclosureDetails.oldUrl = enclosureDetails.url;

        if (m_feed.entries.contains(id)) {
            m_feed.entries[id].enclosures[enclosureDetails.url] = enclosureDetails;
        }
    }
    query.finish();

    query.prepare(QStringLiteral("SELECT id, name, email FROM EntryAuthors JOIN Entries ON Entries.entryuid = EntryAuthors.entryuid WHERE feeduid=:feeduid;"));
    query.bindValue(QStringLiteral(":feeduid"), m_feed.feeduid);
    dbExecute(query);
    while (query.next()) {
        AuthorDetails authorDetails;
        QString id = query.value(QStringLiteral("id")).toString();
        authorDetails.name = query.value(QStringLiteral("name")).toString();
        authorDetails.email = query.value(QStringLiteral("email")).toString();
        authorDetails.state = RecordState::Deleted; // will be set to appropriate value if the author is found in the updated rss feed

        // already set the content of all the "old" fields
        authorDetails.oldEmail = authorDetails.email;

        if (m_feed.entries.contains(id)) {
            m_feed.entries[id].authors[authorDetails.name] = authorDetails;
        }
    }
    query.finish();

    query.prepare(QStringLiteral("SELECT * FROM Chapters JOIN Entries ON Entries.entryuid = Chapters.entryuid WHERE feeduid=:feeduid;"));
    query.bindValue(QStringLiteral(":feeduid"), m_feed.feeduid);
    dbExecute(query);
    while (query.next()) {
        ChapterDetails chapterDetails;
        QString id = query.value(QStringLiteral("id")).toString();
        chapterDetails.start = query.value(QStringLiteral("start")).toInt();
        chapterDetails.title = query.value(QStringLiteral("title")).toString();
        chapterDetails.link = query.value(QStringLiteral("link")).toString();
        chapterDetails.image = query.value(QStringLiteral("image")).toString();
        chapterDetails.state = RecordState::Deleted; // will be set to appropriate value if the chapter is found in the updated rss feed

        // already set the content of all the "old" fields
        chapterDetails.oldTitle = chapterDetails.title;
        chapterDetails.oldLink = chapterDetails.link;
        chapterDetails.oldImage = chapterDetails.image;

        if (m_feed.entries.contains(id)) {
            m_feed.entries[id].chapters[chapterDetails.start] = chapterDetails;
        }
    }
    query.finish();

    qCDebug(kastsUpdater) << "start process feed" << feed;

    if (feed.isNull())
        return;

    // First check if this is a newly added feed and get current name and dirname
    if (m_feed.isNew) {
        qCDebug(kastsUpdater) << "New feed" << feed->title();
    }

    m_markUnreadOnNewFeed = !(SettingsManager::self()->markUnreadOnNewFeed() == 2);
    QDateTime current = QDateTime::currentDateTime();

    m_feed.name = feed->title();
    m_feed.link = feed->link();
    m_feed.description = feed->description();
    m_feed.lastUpdated = current.toSecsSinceEpoch();
    m_feed.lastHash = QString::fromLatin1(QCryptographicHash::hash(m_data, QCryptographicHash::Sha256).toHex());

    // Retrieve "other" fields; this will include the "itunes" tags
    QMultiMap<QString, QDomElement> otherItems = feed->additionalProperties();

    // First try the itunes tags, if not, fall back to regular image tag
    if (otherItems.value(QStringLiteral("http://www.itunes.com/dtds/podcast-1.0.dtdimage")).hasAttribute(QStringLiteral("href"))) {
        m_feed.image = otherItems.value(QStringLiteral("http://www.itunes.com/dtds/podcast-1.0.dtdimage")).attribute(QStringLiteral("href"));
    } else {
        m_feed.image = feed->image()->url();
    }

    if (m_feed.image.startsWith(QStringLiteral("/"))) {
        m_feed.image = QUrl(m_url).adjusted(QUrl::RemovePath).toString() + m_feed.image;
    }

    // if the title has changed, we need to rename the corresponding enclosure
    // download directory name and move the files
    // TODO: The rename should happen simultaneously with the write to the database rather than here
    if (m_feed.oldName != m_feed.name || m_feed.oldDirname.isEmpty() || m_feed.isNew) {
        QString generatedDirname = generateFeedDirname(m_feed.name);
        if (generatedDirname != m_feed.dirname) {
            m_feed.dirname = generatedDirname;
            QString enclosurePath = StorageManager::instance().enclosureDirPath();
            if (QDir(enclosurePath + m_feed.oldDirname).exists()) {
                QDir().rename(enclosurePath + m_feed.oldDirname, enclosurePath + m_feed.dirname);
            } else {
                QDir().mkpath(enclosurePath + m_feed.dirname);
            }
        }
    }

    bool authorsChanged = false;
    // Process feed authors
    authorsChanged = authorsChanged || processFeedAuthors(feed->authors(), otherItems);

    qCDebug(kastsUpdater) << "Updated feed details:" << feed->title();

    // check if any field has changed and only emit signal if there are changes
    bool hasFeedBeenUpdated = false;
    if (m_feed.name != m_feed.name || m_feed.link != m_feed.link || m_feed.image != m_feed.image || m_feed.description != m_feed.description
        || authorsChanged) {
        hasFeedBeenUpdated = true;
    } else {
        hasFeedBeenUpdated = false;
    }

    if (m_abort)
        return;

    // Now deal with the entries, enclosures, entry authors and chapter marks
    bool updatedEntries = false;
    const auto items = feed->items();
    for (const auto &entry : items) {
        if (m_abort)
            return;

        bool isNewEntry = processEntry(entry);
        updatedEntries = updatedEntries || isNewEntry;
    }

    writeToDatabase();

    if (hasFeedBeenUpdated) {
        Q_EMIT feedDetailsUpdated(m_feed.feeduid, m_url, m_feed.name, m_feed.image, m_feed.link, m_feed.description, current, m_feed.dirname);
    }

    if (updatedEntries || m_feed.isNew) {
        Q_EMIT feedUpdated(m_feed.feeduid);
    }

    qCDebug(kastsUpdater) << "done processing feed" << feed;
}

bool UpdateFeedJob::processFeedAuthors(const QList<Syndication::PersonPtr> &authors, const QMultiMap<QString, QDomElement> &otherItems)
{
    bool isNewOrModified = false;

    if (authors.count() > 0) {
        for (auto &author : authors) {
            isNewOrModified = isNewOrModified || processFeedAuthor(author->name(), QLatin1String(""));
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
            qCDebug(kastsUpdater) << "authorname" << authorname;
        }
        if (!authorname.isEmpty()) {
            isNewOrModified = isNewOrModified || processFeedAuthor(authorname, authoremail);
        }
    }
    return isNewOrModified;
}

bool UpdateFeedJob::processFeedAuthor(const QString &name, const QString &email)
{
    bool isNewOrModified = false;

    // check against existing authors already in database
    if (m_feed.authors.contains(name)) {
        if (m_feed.authors[name].email != email) {
            isNewOrModified = true;
            m_feed.authors[name].email = email;
            m_feed.authors[name].state = RecordState::Modified;
            qCDebug(kastsUpdater) << "author details have been updated for feed:" << m_url << name;
        } else {
            m_feed.authors[name].state = RecordState::Unmodified;
            qCDebug(kastsUpdater) << "author details are unchanged:" << m_url << name;
        }
    } else {
        isNewOrModified = true;
        AuthorDetails authorDetails;
        authorDetails.name = name;
        authorDetails.email = email;
        authorDetails.state = RecordState::New;
        m_feed.authors[name] = authorDetails;
        qCDebug(kastsUpdater) << "this is a new author:" << m_url << name;
    }

    return isNewOrModified;
}

bool UpdateFeedJob::processEntry(const Syndication::ItemPtr &entry)
{
    qCDebug(kastsUpdater) << "Processing" << entry->title();
    bool isNewOrModified = false;
    bool isUpdateDependencies = false;
    QString id = entry->id();

    if (m_feed.entries.contains(id)) {
        isNewOrModified = false;
        m_feed.entries[id].state = RecordState::Unmodified;

        // stop here if doFullUpdate is set to false and this is an existing entry
        if (!SettingsManager::self()->doFullUpdate()) {
            return false;
        }
    } else {
        isNewOrModified = true;
    }

    // Retrieve "other" fields; this will include the "itunes" tags
    QMultiMap<QString, QDomElement> otherItems = entry->additionalProperties();

    const auto uniqueKeys = otherItems.uniqueKeys();
    for (const QString &key : uniqueKeys) {
        qCDebug(kastsUpdater) << "other elements";
        qCDebug(kastsUpdater) << key << otherItems.value(key).tagName();
    }

    QString title = QTextDocumentFragment::fromHtml(entry->title()).toPlainText();
    int created = static_cast<int>(entry->datePublished());
    int updated = static_cast<int>(entry->dateUpdated());
    QString link = entry->link();
    bool hasEnclosure = (entry->enclosures().length() > 0);
    bool read = m_feed.isNew ? m_markUnreadOnNewFeed : false; // if new feed, then check settings
    bool isNew = !m_feed.isNew; // if new feed, then mark none as new
    QString content;
    QString image;

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
    qCDebug(kastsUpdater) << "Entry image found" << image;

    // now we start updating the datastructure
    if (!isNewOrModified) {
        if ((title != m_feed.entries[id].title) || (content != m_feed.entries[id].content) || (created != m_feed.entries[id].created)
            || (updated != m_feed.entries[id].updated) || (link != m_feed.entries[id].link) || (hasEnclosure != m_feed.entries[id].hasEnclosure)
            || (image != m_feed.entries[id].image)) {
            isNewOrModified = true;
            m_feed.entries[id].title = title;
            m_feed.entries[id].content = content;
            m_feed.entries[id].created = created;
            m_feed.entries[id].updated = updated;
            m_feed.entries[id].link = link;
            m_feed.entries[id].hasEnclosure = hasEnclosure;
            m_feed.entries[id].image = image;
            m_feed.entries[id].state = RecordState::Modified;
            qCDebug(kastsUpdater) << "episode details have been updated:" << id;
        } else {
            m_feed.entries[id].state = RecordState::Unmodified;
            qCDebug(kastsUpdater) << "episode details are unchanged:" << id;
        }
    } else {
        isNewOrModified = true;
        EntryDetails entryDetails;
        entryDetails.entryuid = 0; // to be replaced by autoincremented value when added to DB
        entryDetails.feeduid = m_feed.feeduid;
        entryDetails.id = id;
        entryDetails.title = title;
        entryDetails.content = content;
        entryDetails.created = created;
        entryDetails.updated = updated;
        entryDetails.link = link;
        entryDetails.read = read;
        entryDetails.isNew = isNew;
        entryDetails.hasEnclosure = hasEnclosure;
        entryDetails.image = image;
        entryDetails.state = RecordState::New;
        m_feed.entries[id] = entryDetails;
        qCDebug(kastsUpdater) << "this is a new episode:" << id;
    }

    // Process authors
    isUpdateDependencies = isUpdateDependencies | processEntryAuthors(id, entry->authors(), otherItems);

    // Process chapters
    isUpdateDependencies = isUpdateDependencies | processChapters(id, otherItems, entry->link());

    // Process enclosures
    isUpdateDependencies = isUpdateDependencies | processEnclosures(id, entry->enclosures());

    return isNewOrModified | isUpdateDependencies; // this is a new or updated entry, or an enclosure, chapter or author has been changed/added
}

bool UpdateFeedJob::processEntryAuthors(const QString &id, const QList<Syndication::PersonPtr> &authors, const QMultiMap<QString, QDomElement> &otherItems)
{
    bool newOrModifiedAuthors = false;

    if (authors.count() > 0) {
        for (const auto &author : authors) {
            newOrModifiedAuthors = newOrModifiedAuthors | processEntryAuthor(id, author->name(), author->email());
        }
    } else {
        // As fallback, check if there is itunes "author" information
        Syndication::PersonPtr author;
        QString authorName = otherItems.value(QStringLiteral("http://www.itunes.com/dtds/podcast-1.0.dtdauthor")).text();
        if (authorName.isEmpty())
            newOrModifiedAuthors = newOrModifiedAuthors | processEntryAuthor(id, authorName, QLatin1String(""));
    }

    return newOrModifiedAuthors;
}

bool UpdateFeedJob::processEntryAuthor(const QString &id, const QString &name, const QString &email)
{
    bool isNewOrModified = false;

    // check against existing authors already in database
    if (m_feed.entries[id].authors.contains(name)) {
        if (m_feed.entries[id].authors[name].email != email) {
            isNewOrModified = true;
            m_feed.entries[id].authors[name].email = email;
            m_feed.entries[id].authors[name].state = RecordState::Modified;
            qCDebug(kastsUpdater) << "author details have been updated for:" << id << name;
        } else {
            m_feed.entries[id].authors[name].state = RecordState::Unmodified;
            qCDebug(kastsUpdater) << "author details are unchanged:" << id << name;
        }
    } else {
        isNewOrModified = true;
        AuthorDetails authorDetails;
        authorDetails.name = name;
        authorDetails.email = email;
        authorDetails.state = RecordState::New;
        m_feed.entries[id].authors[name] = authorDetails;
        qCDebug(kastsUpdater) << "this is a new author:" << id << name;
    }

    return isNewOrModified;
}

bool UpdateFeedJob::processEnclosures(const QString &id, const QList<Syndication::EnclosurePtr> &enclosures)
{
    bool anyEnclosureUpdated = false;

    for (const Syndication::EnclosurePtr &enclosure : enclosures) {
        bool isNewEnclosure = true;
        bool isUpdateEnclosure = false;

        int duration = enclosure->duration();
        int size = enclosure->length();
        QString type = enclosure->type();
        QString url = enclosure->url();

        if (m_feed.entries[id].enclosures.contains(url)) {
            isNewEnclosure = false;
            if ((m_feed.entries[id].enclosures[url].type != type)) {
                isUpdateEnclosure = true;
                m_feed.entries[id].enclosures[url].type = type;
                m_feed.entries[id].enclosures[url].state = RecordState::Modified;
                qCDebug(kastsUpdater) << "enclosure details have been updated for:" << id << url;
            } else {
                m_feed.entries[id].enclosures[url].state = RecordState::Unmodified;
                qCDebug(kastsUpdater) << "enclosure details are unchanged:" << id << url;
            }

            // Check if entry title or enclosure URL has changed
            if (m_feed.entries[id].title != m_feed.entries[id].oldTitle) {
                QString oldFilename = StorageManager::instance().enclosurePath(m_feed.entries[id].oldTitle, url, m_feed.dirname);
                QString newFilename = StorageManager::instance().enclosurePath(m_feed.entries[id].title, url, m_feed.dirname);
                QFile::rename(oldFilename, newFilename);
            }
        } else {
            EnclosureDetails enclosureDetails;
            enclosureDetails.enclosureuid = 0; // this will be set once the enclosure is written to the database
            enclosureDetails.duration = duration;
            enclosureDetails.size = size;
            enclosureDetails.type = type;
            enclosureDetails.url = url;
            enclosureDetails.playPosition = 0;
            enclosureDetails.downloaded = Enclosure::Downloadable;
            enclosureDetails.state = RecordState::New;
            m_feed.entries[id].enclosures[url] = enclosureDetails;
            qCDebug(kastsUpdater) << "this is a new enclosure:" << id << url;
        }
        anyEnclosureUpdated = anyEnclosureUpdated | isNewEnclosure | isUpdateEnclosure;
    }

    return anyEnclosureUpdated;
}

bool UpdateFeedJob::processChapters(const QString &id, const QMultiMap<QString, QDomElement> &otherItems, const QString &link)
{
    bool newOrModifiedChapters = false;

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
                qCDebug(kastsUpdater) << "Found chapter mark:" << start << "; in seconds:" << startInt;
                QString image = element.attribute(QStringLiteral("image"));

                bool isNewOrModified = false;

                // check against existing enclosures already in database
                if (m_feed.entries[id].chapters.contains(startInt)) {
                    if ((m_feed.entries[id].chapters[startInt].title != title) || (m_feed.entries[id].chapters[startInt].link != link)
                        || (m_feed.entries[id].chapters[startInt].image != image)) {
                        isNewOrModified = true;
                        m_feed.entries[id].chapters[startInt].title = title;
                        m_feed.entries[id].chapters[startInt].link = link;
                        m_feed.entries[id].chapters[startInt].image = image;
                        m_feed.entries[id].chapters[startInt].state = RecordState::Modified;
                        qCDebug(kastsUpdater) << "chapter details have been updated for:" << id << start;
                    } else {
                        m_feed.entries[id].chapters[startInt].state = RecordState::Unmodified;
                        qCDebug(kastsUpdater) << "chapter details are unchanged:" << id << start;
                    }
                } else {
                    isNewOrModified = true;
                    ChapterDetails chapterDetails;
                    chapterDetails.start = startInt;
                    chapterDetails.link = link;
                    chapterDetails.image = image;
                    chapterDetails.state = RecordState::New;
                    m_feed.entries[id].chapters[startInt] = chapterDetails;
                    qCDebug(kastsUpdater) << "this is a new chapter:" << id << start;
                }
                newOrModifiedChapters = newOrModifiedChapters | isNewOrModified;
            }
        }
    }

    return newOrModifiedChapters;
}

void UpdateFeedJob::writeToDatabase()
{
    QSet<qint64> newEntryuids, updatedEntryuids;

    QSqlQuery writeQuery(QSqlDatabase::database(m_url));

    dbTransaction();

    // update feed details
    writeQuery.prepare(
        QStringLiteral("UPDATE Feeds SET url=:url, name=:name, image=:image, link=:link, description=:description, lastUpdated=:lastUpdated, dirname=:dirname "
                       "WHERE feeduid=:feeduid;"));
    writeQuery.bindValue(QStringLiteral(":feeduid"), m_feed.feeduid);
    writeQuery.bindValue(QStringLiteral(":url"), m_feed.url);
    writeQuery.bindValue(QStringLiteral(":name"), m_feed.name);
    writeQuery.bindValue(QStringLiteral(":link"), m_feed.link);
    writeQuery.bindValue(QStringLiteral(":description"), m_feed.description);
    writeQuery.bindValue(QStringLiteral(":lastUpdated"), m_feed.lastUpdated);
    writeQuery.bindValue(QStringLiteral(":image"), m_feed.image);
    writeQuery.bindValue(QStringLiteral(":dirname"), m_feed.dirname);
    // we only write the new lastHash to the database after entries etc. have
    // all been updated!
    dbExecute(writeQuery);
    writeQuery.clear(); // make sure this writeQuery is not blocking anything anymore

    // new feed authors
    writeQuery.prepare(QStringLiteral("INSERT INTO FeedAuthors (feeduid, name, email) VALUES (:feeduid, :name, :email);"));
    for (const AuthorDetails &authorDetails : std::as_const(m_feed.authors)) {
        if (authorDetails.state == RecordState::New) {
            writeQuery.bindValue(QStringLiteral(":feeduid"), m_feed.feeduid);
            writeQuery.bindValue(QStringLiteral(":name"), authorDetails.name);
            writeQuery.bindValue(QStringLiteral(":email"), authorDetails.email);
            dbExecute(writeQuery);
        }
    }
    writeQuery.clear();

    // update feed authors
    writeQuery.prepare(QStringLiteral("UPDATE FeedAuthors SET email=:email WHERE feeduid=:feeduid AND name=:name;"));
    for (const AuthorDetails &authorDetails : std::as_const(m_feed.authors)) {
        if (authorDetails.state == RecordState::Modified) {
            writeQuery.bindValue(QStringLiteral(":feeduid"), m_feed.feeduid);
            writeQuery.bindValue(QStringLiteral(":name"), authorDetails.name);
            writeQuery.bindValue(QStringLiteral(":email"), authorDetails.email);
            dbExecute(writeQuery);
        }
    }
    writeQuery.clear();

    // deleted removed feed authors
    writeQuery.prepare(QStringLiteral("DELETE FROM FeedAuthors WHERE feeduid=:feeduid and name=:name;"));
    for (const AuthorDetails &authorDetails : std::as_const(m_feed.authors)) {
        if (authorDetails.state == RecordState::Deleted) {
            writeQuery.bindValue(QStringLiteral(":feeduid"), m_feed.feeduid);
            writeQuery.bindValue(QStringLiteral(":name"), authorDetails.name);
            dbExecute(writeQuery);
            qCDebug(kastsUpdater) << "deleted old feed author:" << m_feed.feeduid << authorDetails.name;
        }
    }
    writeQuery.clear();

    // new entries
    writeQuery.prepare(
        QStringLiteral("INSERT INTO Entries (feeduid, id, title, content, created, updated, link, read, new, hasEnclosure, image, favorite) VALUES "
                       "(:feeduid, :id, :title, :content, :created, :updated, :link, :read, :new, :hasEnclosure, :image, :favorite);"));
    for (const EntryDetails &entryDetails : std::as_const(m_feed.entries)) {
        if (entryDetails.state == RecordState::New) {
            writeQuery.bindValue(QStringLiteral(":feeduid"), entryDetails.feeduid);
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
            if (dbExecute(writeQuery)) {
                QVariant lastId = writeQuery.lastInsertId();
                if (lastId.isValid()) {
                    m_feed.entries[entryDetails.id].entryuid = lastId.toLongLong(); // TODO: check if this is ok wrt 'detaching'
                    newEntryuids.insert(lastId.toLongLong());
                } else {
                    qCDebug(kastsUpdater) << "new episode did not get a valid entryuid" << entryDetails.id;
                }
            }
        }
    }
    writeQuery.clear();

    // update entries
    writeQuery.prepare(
        QStringLiteral("UPDATE Entries SET id=:id, title=:title, content=:content, created=:created, updated=:updated, link=:link, hasEnclosure=:hasEnclosure, "
                       "image=:image WHERE entryuid=:entryuid;"));
    for (const EntryDetails &entryDetails : std::as_const(m_feed.entries)) {
        if (entryDetails.state == RecordState::Modified) {
            updatedEntryuids.insert(entryDetails.entryuid);
            writeQuery.bindValue(QStringLiteral(":entryuid"), entryDetails.entryuid);
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
    }
    writeQuery.clear();

    // new authors
    writeQuery.prepare(QStringLiteral("INSERT INTO EntryAuthors (entryuid, name, email) VALUES (:entryuid, :name, :email);"));
    for (const EntryDetails &entryDetails : std::as_const(m_feed.entries)) {
        if (entryDetails.entryuid == 0) {
            qCDebug(kastsUpdater) << "new episode did not get a valid entryuid; skipping authors for id:" << entryDetails.id;
        } else {
            for (const AuthorDetails &authorDetails : std::as_const(entryDetails.authors)) {
                if (authorDetails.state == RecordState::New) {
                    updatedEntryuids.insert(entryDetails.entryuid);
                    writeQuery.bindValue(QStringLiteral(":entryuid"), entryDetails.entryuid);
                    writeQuery.bindValue(QStringLiteral(":name"), authorDetails.name);
                    writeQuery.bindValue(QStringLiteral(":email"), authorDetails.email);
                    dbExecute(writeQuery);
                }
            }
        }
    }
    writeQuery.clear();

    // update authors
    writeQuery.prepare(QStringLiteral("UPDATE EntryAuthors SET email=:email WHERE entryuid=:entryuid AND name=:name;"));
    for (const EntryDetails &entryDetails : std::as_const(m_feed.entries)) {
        for (const AuthorDetails &authorDetails : std::as_const(entryDetails.authors)) {
            if (authorDetails.state == RecordState::Modified) {
                updatedEntryuids.insert(entryDetails.entryuid);
                writeQuery.bindValue(QStringLiteral(":entryuid"), entryDetails.entryuid);
                writeQuery.bindValue(QStringLiteral(":name"), authorDetails.name);
                writeQuery.bindValue(QStringLiteral(":email"), authorDetails.email);
                dbExecute(writeQuery);
            }
        }
    }
    writeQuery.clear();

    // delete entry authors that were removed
    writeQuery.prepare(QStringLiteral("DELETE FROM EntryAuthors WHERE entryuid=:entryuid AND name=:name;"));
    for (const EntryDetails &entryDetails : std::as_const(m_feed.entries)) {
        if (entryDetails.state != RecordState::Deleted) {
            for (const AuthorDetails &authorDetails : std::as_const(entryDetails.authors)) {
                if (authorDetails.state == RecordState::Deleted) {
                    updatedEntryuids.insert(entryDetails.entryuid);
                    writeQuery.bindValue(QStringLiteral(":entryuid"), entryDetails.entryuid);
                    writeQuery.bindValue(QStringLiteral(":name"), authorDetails.name);
                    dbExecute(writeQuery);
                    qCDebug(kastsUpdater) << "deleted old entry author:" << m_feed.feeduid << entryDetails.entryuid << authorDetails.name;
                }
            }
        }
    }
    writeQuery.clear();

    // new enclosures
    writeQuery.prepare(
        QStringLiteral("INSERT INTO Enclosures (entryuid, feeduid, url, duration, size, type, playposition, downloaded) VALUES (:entryuid, :feeduid, "
                       ":url, :duration, :size, :type, :playposition, :downloaded);"));
    for (const EntryDetails &entryDetails : std::as_const(m_feed.entries)) {
        if (entryDetails.entryuid == 0) {
            qCDebug(kastsUpdater) << "new episode did not get a valid entryuid; skipping enclosures for id:" << entryDetails.id;
        } else {
            for (const EnclosureDetails &enclosureDetails : std::as_const(entryDetails.enclosures)) {
                if (enclosureDetails.state == RecordState::New) {
                    updatedEntryuids.insert(entryDetails.entryuid);
                    writeQuery.bindValue(QStringLiteral(":entryuid"), entryDetails.entryuid);
                    writeQuery.bindValue(QStringLiteral(":feeduid"), entryDetails.feeduid);
                    writeQuery.bindValue(QStringLiteral(":duration"), enclosureDetails.duration);
                    writeQuery.bindValue(QStringLiteral(":size"), enclosureDetails.size);
                    writeQuery.bindValue(QStringLiteral(":type"), enclosureDetails.type);
                    writeQuery.bindValue(QStringLiteral(":url"), enclosureDetails.url);
                    writeQuery.bindValue(QStringLiteral(":playposition"), enclosureDetails.playPosition);
                    writeQuery.bindValue(QStringLiteral(":downloaded"), Enclosure::statusToDb(enclosureDetails.downloaded));
                    dbExecute(writeQuery);
                }
            }
        }
    }
    writeQuery.clear();

    // update enclosures
    writeQuery.prepare(
        QStringLiteral("UPDATE Enclosures SET duration=:duration, size=:size, title=:title, type=:type, url=:url WHERE entryuid=:entryuid "
                       "AND enclosureuid=:enclosureuid;"));
    for (const EntryDetails &entryDetails : std::as_const(m_feed.entries)) {
        for (const EnclosureDetails &enclosureDetails : std::as_const(entryDetails.enclosures)) {
            if (enclosureDetails.state == RecordState::Modified) {
                updatedEntryuids.insert(entryDetails.entryuid);
                writeQuery.bindValue(QStringLiteral(":enclosureuid"), enclosureDetails.enclosureuid);
                writeQuery.bindValue(QStringLiteral(":entryuid"), entryDetails.entryuid);
                writeQuery.bindValue(QStringLiteral(":duration"), enclosureDetails.duration);
                writeQuery.bindValue(QStringLiteral(":size"), enclosureDetails.size);
                writeQuery.bindValue(QStringLiteral(":type"), enclosureDetails.type);
                writeQuery.bindValue(QStringLiteral(":url"), enclosureDetails.url);
                dbExecute(writeQuery);
            }
        }
    }
    writeQuery.clear();

    // delete removed enclosures
    writeQuery.prepare(QStringLiteral("DELETE FROM Enclosures WHERE enclosureuid=:enclosureuid;"));
    for (const EntryDetails &entryDetails : std::as_const(m_feed.entries)) {
        if (entryDetails.state != RecordState::Deleted) {
            for (const EnclosureDetails &enclosureDetails : std::as_const(entryDetails.enclosures)) {
                if (enclosureDetails.state == RecordState::Deleted) {
                    updatedEntryuids.insert(entryDetails.entryuid);
                    writeQuery.bindValue(QStringLiteral(":enclosureuid"), enclosureDetails.enclosureuid);
                    dbExecute(writeQuery);
                    qCDebug(kastsUpdater) << "deleted old enclosure:" << m_feed.feeduid << enclosureDetails.enclosureuid << enclosureDetails.url;
                }
            }
        }
    }
    writeQuery.clear();

    // new chapters
    writeQuery.prepare(QStringLiteral("INSERT INTO Chapters (entryuid, start, title, link, image) VALUES (:entryuid, :start, :title, :link, :image);"));
    for (const EntryDetails &entryDetails : std::as_const(m_feed.entries)) {
        if (entryDetails.entryuid == 0) {
            qCDebug(kastsUpdater) << "new episode did not get a valid entryuid; skipping chapters for id:" << entryDetails.id;
        } else {
            for (const ChapterDetails &chapterDetails : std::as_const(entryDetails.chapters)) {
                if (chapterDetails.state == RecordState::New) {
                    updatedEntryuids.insert(entryDetails.entryuid);
                    writeQuery.bindValue(QStringLiteral(":entryuid"), entryDetails.entryuid);
                    writeQuery.bindValue(QStringLiteral(":start"), chapterDetails.start);
                    writeQuery.bindValue(QStringLiteral(":title"), chapterDetails.title);
                    writeQuery.bindValue(QStringLiteral(":link"), chapterDetails.link);
                    writeQuery.bindValue(QStringLiteral(":image"), chapterDetails.image);
                    dbExecute(writeQuery);
                }
            }
        }
    }
    writeQuery.clear();

    // update chapters
    writeQuery.prepare(QStringLiteral("UPDATE Chapters SET title=:title, link=:link, image=:image WHERE entryuid=:entryuid AND start=:start;"));
    for (const EntryDetails &entryDetails : std::as_const(m_feed.entries)) {
        for (const ChapterDetails &chapterDetails : std::as_const(entryDetails.chapters)) {
            if (chapterDetails.state == RecordState::Modified) {
                updatedEntryuids.insert(entryDetails.entryuid);
                writeQuery.bindValue(QStringLiteral(":entryuid"), entryDetails.entryuid);
                writeQuery.bindValue(QStringLiteral(":start"), chapterDetails.start);
                writeQuery.bindValue(QStringLiteral(":title"), chapterDetails.title);
                writeQuery.bindValue(QStringLiteral(":link"), chapterDetails.link);
                writeQuery.bindValue(QStringLiteral(":image"), chapterDetails.image);
                dbExecute(writeQuery);
            }
        }
    }
    writeQuery.clear();

    // We don't delete chapters that haven't been found anymore, since they could also have been added through other means
    // e.g. id3 tags.

    // set custom amount of episodes to unread/new if required
    if (m_feed.isNew && (SettingsManager::self()->markUnreadOnNewFeed() == 1) && (SettingsManager::self()->markUnreadOnNewFeedCustomAmount() > 0)) {
        writeQuery.prepare(
            QStringLiteral("UPDATE Entries SET read=:read, new=:new WHERE entryuid in (SELECT entryuid FROM Entries WHERE feeduid =:feeduid ORDER BY updated "
                           "DESC LIMIT :recentUnread);"));
        writeQuery.bindValue(QStringLiteral(":feeduid"), m_feed.feeduid);
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
        writeQuery.prepare(QStringLiteral("UPDATE Feeds SET new=:new WHERE feeduid=:feeduid;"));
        writeQuery.bindValue(QStringLiteral(":feeduid"), m_feed.feeduid);
        writeQuery.bindValue(QStringLiteral(":new"), false);
        dbExecute(writeQuery);
        writeQuery.clear();
    }

    if (m_feed.lastHash != m_feed.oldLastHash) {
        writeQuery.prepare(QStringLiteral("UPDATE Feeds SET lastHash=:lastHash WHERE feeduid=:feeduid;"));
        writeQuery.bindValue(QStringLiteral(":feeduid"), m_feed.feeduid);
        writeQuery.bindValue(QStringLiteral(":lastHash"), m_feed.lastHash);
        dbExecute(writeQuery);
        writeQuery.clear();
    }

    if (dbCommit()) {
        // emit signals for new entries
        for (const qint64 entryuid : std::as_const(newEntryuids)) {
            qCDebug(kastsUpdater) << "new episode" << entryuid;
            Q_EMIT entryAdded(entryuid);
        }

        // emit signals for updated entries or entries with new/updated authors,
        // enclosures or chapters
        for (const qint64 entryuid : std::as_const(updatedEntryuids)) {
            qCDebug(kastsUpdater) << "updated episode" << entryuid;
            Q_EMIT entryUpdated(entryuid);
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
    query.prepare(QStringLiteral("BEGIN IMMEDIATE TRANSACTION;"));
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
