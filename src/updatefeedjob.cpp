/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "updatefeedjob.h"

#include <QDomElement>
#include <QMultiMap>
#include <QNetworkReply>
#include <QSqlQuery>
#include <QTextDocumentFragment>
#include <QTimer>

#include <KLocalizedString>

#include "database.h"
#include "enclosure.h"
#include "fetcher.h"
#include "fetcherlogging.h"
#include "settingsmanager.h"

UpdateFeedJob::UpdateFeedJob(const QString &url, QObject *parent)
    : KJob(parent)
    , m_url(url)
{
    // connect to signals in Fetcher such that GUI can pick up the changes
    connect(this, &UpdateFeedJob::feedDetailsUpdated, &Fetcher::instance(), &Fetcher::feedDetailsUpdated);
    connect(this, &UpdateFeedJob::feedUpdated, &Fetcher::instance(), &Fetcher::feedUpdated);
    connect(this, &UpdateFeedJob::entryAdded, &Fetcher::instance(), &Fetcher::entryAdded);
    connect(this, &UpdateFeedJob::feedUpdateStatusChanged, &Fetcher::instance(), &Fetcher::feedUpdateStatusChanged);
}

void UpdateFeedJob::start()
{
    QTimer::singleShot(0, this, &UpdateFeedJob::retrieveFeed);
}

void UpdateFeedJob::retrieveFeed()
{
    if (m_abort) {
        emitResult();
        return;
    }

    qCDebug(kastsFetcher) << "Starting to fetch" << m_url;
    Q_EMIT feedUpdateStatusChanged(m_url, true);

    QNetworkRequest request((QUrl(m_url)));
    request.setTransferTimeout();
    m_reply = Fetcher::instance().get(request);
    connect(this, &UpdateFeedJob::aborting, m_reply, &QNetworkReply::abort);
    connect(m_reply, &QNetworkReply::finished, this, [this]() {
        qCDebug(kastsFetcher) << "got networkreply for" << m_reply;
        if (m_reply->error()) {
            if (!m_abort) {
                qWarning() << "Error fetching feed";
                qWarning() << m_reply->errorString();
                setError(m_reply->error());
                setErrorText(m_reply->errorString());
            }
        } else {
            QByteArray data = m_reply->readAll();
            Syndication::DocumentSource document(data, m_url);
            Syndication::FeedPtr feed = Syndication::parserCollection()->parse(document, QStringLiteral("Atom"));
            processFeed(feed);
        }
        Q_EMIT feedUpdateStatusChanged(m_url, false);
        m_reply->deleteLater();
        emitResult();
    });
    qCDebug(kastsFetcher) << "End of retrieveFeed for" << m_url;
}

void UpdateFeedJob::processFeed(Syndication::FeedPtr feed)
{
    qCDebug(kastsFetcher) << "start process feed" << feed;

    if (feed.isNull())
        return;

    // First check if this is a newly added feed
    m_isNewFeed = false;
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT new FROM Feeds WHERE url=:url;"));
    query.bindValue(QStringLiteral(":url"), m_url);
    Database::instance().execute(query);
    if (query.next()) {
        m_isNewFeed = query.value(QStringLiteral("new")).toBool();
    } else {
        qCDebug(kastsFetcher) << "Feed not found in database" << m_url;
        return;
    }
    if (m_isNewFeed)
        qCDebug(kastsFetcher) << "New feed" << feed->title();

    // Retrieve "other" fields; this will include the "itunes" tags
    QMultiMap<QString, QDomElement> otherItems = feed->additionalProperties();

    query.prepare(QStringLiteral("UPDATE Feeds SET name=:name, image=:image, link=:link, description=:description, lastUpdated=:lastUpdated WHERE url=:url;"));
    query.bindValue(QStringLiteral(":name"), feed->title());
    query.bindValue(QStringLiteral(":url"), m_url);
    query.bindValue(QStringLiteral(":link"), feed->link());
    query.bindValue(QStringLiteral(":description"), feed->description());

    QDateTime current = QDateTime::currentDateTime();
    query.bindValue(QStringLiteral(":lastUpdated"), current.toSecsSinceEpoch());

    QString image = feed->image()->url();
    // If there is no regular image tag, then try the itunes tags
    if (image.isEmpty()) {
        if (otherItems.value(QStringLiteral("http://www.itunes.com/dtds/podcast-1.0.dtdimage")).hasAttribute(QStringLiteral("href"))) {
            image = otherItems.value(QStringLiteral("http://www.itunes.com/dtds/podcast-1.0.dtdimage")).attribute(QStringLiteral("href"));
        }
    }
    if (image.startsWith(QStringLiteral("/")))
        image = QUrl(m_url).adjusted(QUrl::RemovePath).toString() + image;
    query.bindValue(QStringLiteral(":image"), image);

    // Do the actual database UPDATE of this feed
    Database::instance().execute(query);

    // Now that we have the feed details, we make vectors of the data that's
    // already in the database relating to this feed
    // NOTE: We will do the feed authors after this step, because otherwise
    // we can't check for duplicates and we'll keep adding more of the same!
    query.prepare(QStringLiteral("SELECT id FROM Entries WHERE feed=:feed;"));
    query.bindValue(QStringLiteral(":feed"), m_url);
    Database::instance().execute(query);
    while (query.next()) {
        m_existingEntryIds += query.value(QStringLiteral("id")).toString();
    }

    query.prepare(QStringLiteral("SELECT id, url FROM Enclosures WHERE feed=:feed;"));
    query.bindValue(QStringLiteral(":feed"), m_url);
    Database::instance().execute(query);
    while (query.next()) {
        m_existingEnclosures += qMakePair(query.value(QStringLiteral("id")).toString(), query.value(QStringLiteral("url")).toString());
    }

    query.prepare(QStringLiteral("SELECT id, name FROM Authors WHERE feed=:feed;"));
    query.bindValue(QStringLiteral(":feed"), m_url);
    Database::instance().execute(query);
    while (query.next()) {
        m_existingAuthors += qMakePair(query.value(QStringLiteral("id")).toString(), query.value(QStringLiteral("name")).toString());
    }

    query.prepare(QStringLiteral("SELECT id, start FROM Chapters WHERE feed=:feed;"));
    query.bindValue(QStringLiteral(":feed"), m_url);
    Database::instance().execute(query);
    while (query.next()) {
        m_existingChapters += qMakePair(query.value(QStringLiteral("id")).toString(), query.value(QStringLiteral("start")).toInt());
    }

    // Process feed authors
    QString authorname, authoremail;
    if (feed->authors().count() > 0) {
        for (auto &author : feed->authors()) {
            processAuthor(QLatin1String(""), author->name(), QLatin1String(""), QLatin1String(""));
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
            processAuthor(QLatin1String(""), authorname, QLatin1String(""), authoremail);
        }
    }

    qCDebug(kastsFetcher) << "Updated feed details:" << feed->title();

    // TODO: Only emit signal if the details have really changed
    Q_EMIT feedDetailsUpdated(m_url, feed->title(), image, feed->link(), feed->description(), current);

    if (m_abort)
        return;

    // Now deal with the entries, enclosures, entry authors and chapter marks
    bool updatedEntries = false;
    for (const auto &entry : feed->items()) {
        if (m_abort)
            return;
        QCoreApplication::processEvents(); // keep the main thread semi-responsive
        bool isNewEntry = processEntry(entry);
        updatedEntries = updatedEntries || isNewEntry;
    }

    writeToDatabase();

    if (m_isNewFeed) {
        // Finally, reset the new flag to false now that the new feed has been
        // fully processed.  If we would reset the flag sooner, then too many
        // episodes will get flagged as new if the initial import gets
        // interrupted somehow.
        query.prepare(QStringLiteral("UPDATE Feeds SET new=:new WHERE url=:url;"));
        query.bindValue(QStringLiteral(":url"), m_url);
        query.bindValue(QStringLiteral(":new"), false);
        Database::instance().execute(query);
    }

    if (updatedEntries || m_isNewFeed)
        Q_EMIT feedUpdated(m_url);
    qCDebug(kastsFetcher) << "done processing feed" << feed;
}

bool UpdateFeedJob::processEntry(Syndication::ItemPtr entry)
{
    qCDebug(kastsFetcher) << "Processing" << entry->title();

    // Retrieve "other" fields; this will include the "itunes" tags
    QMultiMap<QString, QDomElement> otherItems = entry->additionalProperties();

    for (QString key : otherItems.uniqueKeys()) {
        qCDebug(kastsFetcher) << "other elements";
        qCDebug(kastsFetcher) << key << otherItems.value(key).tagName();
    }

    // check against existing entries in database
    if (m_existingEntryIds.contains(entry->id()))
        return false;

    // also check against the list of new entries
    for (EntryDetails entryDetails : m_entries) {
        if (entryDetails.id == entry->id())
            return false; // entry already exists
    }

    EntryDetails entryDetails;
    entryDetails.feed = m_url;
    entryDetails.id = entry->id();
    entryDetails.title = QTextDocumentFragment::fromHtml(entry->title()).toPlainText();
    entryDetails.created = static_cast<int>(entry->datePublished());
    entryDetails.updated = static_cast<int>(entry->dateUpdated());
    entryDetails.link = entry->link();
    entryDetails.hasEnclosure = (entry->enclosures().length() > 0);
    entryDetails.read = m_isNewFeed; // if new feed, then mark all as read
    entryDetails.isNew = !m_isNewFeed; // if new feed, then mark none as new

    if (!entry->content().isEmpty())
        entryDetails.content = entry->content();
    else
        entryDetails.content = entry->description();

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

    m_entries += entryDetails;

    // Process authors
    if (entry->authors().count() > 0) {
        for (const auto &author : entry->authors()) {
            processAuthor(entry->id(), author->name(), author->uri(), author->email());
        }
    } else {
        // As fallback, check if there is itunes "author" information
        QString authorName = otherItems.value(QStringLiteral("http://www.itunes.com/dtds/podcast-1.0.dtdauthor")).text();
        if (!authorName.isEmpty())
            processAuthor(entry->id(), authorName, QLatin1String(""), QLatin1String(""));
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
                processChapter(entry->id(), startInt, title, entry->link(), images);
            }
        }
    }

    // Process enclosures
    // only process first enclosure if there are multiple (e.g. mp3 and ogg);
    // the first one is probably the podcast author's preferred version
    // TODO: handle more than one enclosure?
    if (entry->enclosures().count() > 0) {
        processEnclosure(entry->enclosures()[0], entry);
    }

    return true; // this is a new entry
}

void UpdateFeedJob::processAuthor(const QString &entryId, const QString &authorName, const QString &authorUri, const QString &authorEmail)
{
    // check against existing authors already in database
    if (m_existingAuthors.contains(qMakePair(entryId, authorName)))
        return;

    AuthorDetails authorDetails;
    authorDetails.feed = m_url;
    authorDetails.id = entryId;
    authorDetails.name = authorName;
    authorDetails.uri = authorUri;
    authorDetails.email = authorEmail;
    m_authors += authorDetails;
}

void UpdateFeedJob::processEnclosure(Syndication::EnclosurePtr enclosure, Syndication::ItemPtr entry)
{
    // check against existing enclosures already in database
    if (m_existingEnclosures.contains(qMakePair(entry->id(), enclosure->url())))
        return;

    EnclosureDetails enclosureDetails;
    enclosureDetails.feed = m_url;
    enclosureDetails.id = entry->id();
    enclosureDetails.duration = enclosure->duration();
    enclosureDetails.size = enclosure->length();
    enclosureDetails.title = enclosure->title();
    enclosureDetails.type = enclosure->type();
    enclosureDetails.url = enclosure->url();
    enclosureDetails.playPosition = 0;
    enclosureDetails.downloaded = Enclosure::Downloadable;

    m_enclosures += enclosureDetails;
}

void UpdateFeedJob::processChapter(const QString &entryId, const int &start, const QString &chapterTitle, const QString &link, const QString &image)
{
    // check against existing enclosures already in database
    if (m_existingChapters.contains(qMakePair(entryId, start)))
        return;

    ChapterDetails chapterDetails;
    chapterDetails.feed = m_url;
    chapterDetails.id = entryId;
    chapterDetails.start = start;
    chapterDetails.title = chapterTitle;
    chapterDetails.link = link;
    chapterDetails.image = image;

    m_chapters += chapterDetails;
}

void UpdateFeedJob::writeToDatabase()
{
    QSqlQuery writeQuery;

    Database::instance().transaction();

    // Entries
    writeQuery.prepare(
        QStringLiteral("INSERT INTO Entries VALUES (:feed, :id, :title, :content, :created, :updated, :link, :read, :new, :hasEnclosure, :image);"));
    for (EntryDetails entryDetails : m_entries) {
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
        Database::instance().execute(writeQuery);
    }

    // Authors
    writeQuery.prepare(QStringLiteral("INSERT INTO Authors VALUES(:feed, :id, :name, :uri, :email);"));
    for (AuthorDetails authorDetails : m_authors) {
        writeQuery.bindValue(QStringLiteral(":feed"), authorDetails.feed);
        writeQuery.bindValue(QStringLiteral(":id"), authorDetails.id);
        writeQuery.bindValue(QStringLiteral(":name"), authorDetails.name);
        writeQuery.bindValue(QStringLiteral(":uri"), authorDetails.uri);
        writeQuery.bindValue(QStringLiteral(":email"), authorDetails.email);
        Database::instance().execute(writeQuery);
    }

    // Enclosures
    writeQuery.prepare(QStringLiteral("INSERT INTO Enclosures VALUES (:feed, :id, :duration, :size, :title, :type, :url, :playposition, :downloaded);"));
    for (EnclosureDetails enclosureDetails : m_enclosures) {
        writeQuery.bindValue(QStringLiteral(":feed"), enclosureDetails.feed);
        writeQuery.bindValue(QStringLiteral(":id"), enclosureDetails.id);
        writeQuery.bindValue(QStringLiteral(":duration"), enclosureDetails.duration);
        writeQuery.bindValue(QStringLiteral(":size"), enclosureDetails.size);
        writeQuery.bindValue(QStringLiteral(":title"), enclosureDetails.title);
        writeQuery.bindValue(QStringLiteral(":type"), enclosureDetails.type);
        writeQuery.bindValue(QStringLiteral(":url"), enclosureDetails.url);
        writeQuery.bindValue(QStringLiteral(":playposition"), enclosureDetails.playPosition);
        writeQuery.bindValue(QStringLiteral(":downloaded"), Enclosure::statusToDb(enclosureDetails.downloaded));
        Database::instance().execute(writeQuery);
    }

    // Chapters
    writeQuery.prepare(QStringLiteral("INSERT INTO Chapters VALUES(:feed, :id, :start, :title, :link, :image);"));
    for (ChapterDetails chapterDetails : m_chapters) {
        writeQuery.bindValue(QStringLiteral(":feed"), chapterDetails.feed);
        writeQuery.bindValue(QStringLiteral(":id"), chapterDetails.id);
        writeQuery.bindValue(QStringLiteral(":start"), chapterDetails.start);
        writeQuery.bindValue(QStringLiteral(":title"), chapterDetails.title);
        writeQuery.bindValue(QStringLiteral(":link"), chapterDetails.link);
        writeQuery.bindValue(QStringLiteral(":image"), chapterDetails.image);
        Database::instance().execute(writeQuery);
    }

    if (Database::instance().commit()) {
        for (EntryDetails entryDetails : m_entries) {
            Q_EMIT entryAdded(m_url, entryDetails.id);
        }
    }
}

void UpdateFeedJob::abort()
{
    m_abort = true;
    Q_EMIT aborting();
}
