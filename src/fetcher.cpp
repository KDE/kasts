/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "fetcher.h"

#include <KLocalizedString>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QDomElement>
#include <QFile>
#include <QFileInfo>
#include <QMultiMap>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTextDocumentFragment>

#include <Syndication/Syndication>

#include "database.h"
#include "enclosure.h"
#include "fetcherlogging.h"
#include "kasts-version.h"
#include "settingsmanager.h"
#include "storagemanager.h"

Fetcher::Fetcher()
{
    m_updateProgress = -1;
    m_updateTotal = -1;
    m_updating = false;

    manager = new QNetworkAccessManager(this);
    manager->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
    manager->setStrictTransportSecurityEnabled(true);
    manager->enableStrictTransportSecurityStore(true);

#if !defined Q_OS_ANDROID && !defined Q_OS_WIN
    m_nmInterface = new OrgFreedesktopNetworkManagerInterface(QStringLiteral("org.freedesktop.NetworkManager"),
                                                              QStringLiteral("/org/freedesktop/NetworkManager"),
                                                              QDBusConnection::systemBus(),
                                                              this);
#endif
}

void Fetcher::fetch(const QString &url)
{
    QStringList urls(url);
    fetch(urls);
}

void Fetcher::fetch(const QStringList &urls)
{
    if (m_updating)
        return; // update is already running, do nothing

    m_updating = true;
    m_updateProgress = 0;
    m_updateTotal = urls.count();
    connect(this, &Fetcher::updateProgressChanged, this, &Fetcher::updateMonitor);
    Q_EMIT updatingChanged(m_updating);
    Q_EMIT updateProgressChanged(m_updateProgress);
    Q_EMIT updateTotalChanged(m_updateTotal);

    for (int i = 0; i < urls.count(); i++) {
        retrieveFeed(urls[i]);
    }
}

void Fetcher::fetchAll()
{
    QStringList urls;
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT url FROM Feeds;"));
    Database::instance().execute(query);
    while (query.next()) {
        urls += query.value(0).toString();
        ;
    }

    if (urls.count() > 0) {
        fetch(urls);
    }
}

void Fetcher::retrieveFeed(const QString &url)
{
    if (isMeteredConnection() && !SettingsManager::self()->allowMeteredFeedUpdates()) {
        Q_EMIT error(Error::Type::MeteredNotAllowed, url, QString(), 0, i18n("Podcast updates not allowed due to user setting"), QString());
        m_updateProgress++;
        Q_EMIT updateProgressChanged(m_updateProgress);
        return;
    }

    qCDebug(kastsFetcher) << "Starting to fetch" << url;

    Q_EMIT startedFetchingFeed(url);

    QNetworkRequest request((QUrl(url)));
    request.setTransferTimeout();
    QNetworkReply *reply = get(request);
    connect(reply, &QNetworkReply::finished, this, [this, url, reply]() {
        if (reply->error()) {
            qWarning() << "Error fetching feed";
            qWarning() << reply->errorString();
            Q_EMIT error(Error::Type::FeedUpdate, url, QString(), reply->error(), reply->errorString(), QString());
        } else {
            QByteArray data = reply->readAll();
            Syndication::DocumentSource *document = new Syndication::DocumentSource(data, url);
            Syndication::FeedPtr feed = Syndication::parserCollection()->parse(*document, QStringLiteral("Atom"));
            processFeed(feed, url);
        }
        m_updateProgress++;
        Q_EMIT updateProgressChanged(m_updateProgress);
        delete reply;
    });
}

void Fetcher::updateMonitor(int progress)
{
    qCDebug(kastsFetcher) << "Update monitor" << progress << "/" << m_updateTotal;
    // this method will watch for the end of the update process
    if (progress > -1 && m_updateTotal > -1 && progress == m_updateTotal) {
        m_updating = false;
        m_updateProgress = -1;
        m_updateTotal = -1;
        disconnect(this, &Fetcher::updateProgressChanged, this, &Fetcher::updateMonitor);
        Q_EMIT updatingChanged(m_updating);
        // Q_EMIT updateProgressChanged(m_updateProgress);
        // Q_EMIT updateTotalChanged(m_updateTotal);
    }
}

void Fetcher::processFeed(Syndication::FeedPtr feed, const QString &url)
{
    if (feed.isNull())
        return;

    // First check if this is a newly added feed
    bool isNewFeed = false;
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT new FROM Feeds WHERE url=:url;"));
    query.bindValue(QStringLiteral(":url"), url);
    Database::instance().execute(query);
    if (query.next()) {
        isNewFeed = query.value(QStringLiteral("new")).toBool();
    } else {
        qCDebug(kastsFetcher) << "Feed not found in database" << url;
        return;
    }
    if (isNewFeed)
        qCDebug(kastsFetcher) << "New feed" << feed->title() << ":" << isNewFeed;

    // Retrieve "other" fields; this will include the "itunes" tags
    QMultiMap<QString, QDomElement> otherItems = feed->additionalProperties();

    query.prepare(QStringLiteral("UPDATE Feeds SET name=:name, image=:image, link=:link, description=:description, lastUpdated=:lastUpdated WHERE url=:url;"));
    query.bindValue(QStringLiteral(":name"), feed->title());
    query.bindValue(QStringLiteral(":url"), url);
    query.bindValue(QStringLiteral(":link"), feed->link());
    query.bindValue(QStringLiteral(":description"), feed->description());

    QDateTime current = QDateTime::currentDateTime();
    query.bindValue(QStringLiteral(":lastUpdated"), current.toSecsSinceEpoch());

    // Process authors
    QString authorname, authoremail;
    if (feed->authors().count() > 0) {
        for (auto &author : feed->authors()) {
            processAuthor(url, QLatin1String(""), author->name(), QLatin1String(""), QLatin1String(""));
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
            processAuthor(url, QLatin1String(""), authorname, QLatin1String(""), authoremail);
        }
    }

    QString image = feed->image()->url();
    // If there is no regular image tag, then try the itunes tags
    if (image.isEmpty()) {
        if (otherItems.value(QStringLiteral("http://www.itunes.com/dtds/podcast-1.0.dtdimage")).hasAttribute(QStringLiteral("href"))) {
            image = otherItems.value(QStringLiteral("http://www.itunes.com/dtds/podcast-1.0.dtdimage")).attribute(QStringLiteral("href"));
        }
    }
    if (image.startsWith(QStringLiteral("/")))
        image = QUrl(url).adjusted(QUrl::RemovePath).toString() + image;
    query.bindValue(QStringLiteral(":image"), image);
    Database::instance().execute(query);

    qCDebug(kastsFetcher) << "Updated feed details:" << feed->title();

    Q_EMIT feedDetailsUpdated(url, feed->title(), image, feed->link(), feed->description(), current);

    bool updatedEntries = false;
    for (const auto &entry : feed->items()) {
        QCoreApplication::processEvents(); // keep the main thread semi-responsive
        bool isNewEntry = processEntry(entry, url, isNewFeed);
        updatedEntries = updatedEntries || isNewEntry;
    }

    // Now mark the appropriate number of recent entries "new" and "read" only for new feeds
    if (isNewFeed) {
        query.prepare(QStringLiteral("SELECT * FROM Entries WHERE feed=:feed ORDER BY updated DESC LIMIT :recentNew;"));
        query.bindValue(QStringLiteral(":feed"), url);
        query.bindValue(QStringLiteral(":recentNew"), 0); // hardcode to marking no episode as new on a new feed
        Database::instance().execute(query);
        QSqlQuery updateQuery;
        while (query.next()) {
            qCDebug(kastsFetcher) << "Marked as new:" << query.value(QStringLiteral("id")).toString();
            updateQuery.prepare(QStringLiteral("UPDATE Entries SET read=:read, new=:new WHERE id=:id AND feed=:feed;"));
            updateQuery.bindValue(QStringLiteral(":read"), false);
            updateQuery.bindValue(QStringLiteral(":new"), true);
            updateQuery.bindValue(QStringLiteral(":feed"), url);
            updateQuery.bindValue(QStringLiteral(":id"), query.value(QStringLiteral("id")).toString());
            Database::instance().execute(updateQuery);
        }
        // Finally, reset the new flag to false now that the new feed has been fully processed
        // If we would reset the flag sooner, then too many episodes will get flagged as new if
        // the initial import gets interrupted somehow.
        query.prepare(QStringLiteral("UPDATE Feeds SET new=:new WHERE url=:url;"));
        query.bindValue(QStringLiteral(":url"), url);
        query.bindValue(QStringLiteral(":new"), false);
        Database::instance().execute(query);
    }

    if (updatedEntries || isNewFeed)
        Q_EMIT feedUpdated(url);
    Q_EMIT feedUpdateFinished(url);
}

bool Fetcher::processEntry(Syndication::ItemPtr entry, const QString &url, bool isNewFeed)
{
    qCDebug(kastsFetcher) << "Processing" << entry->title();

    // Retrieve "other" fields; this will include the "itunes" tags
    QMultiMap<QString, QDomElement> otherItems = entry->additionalProperties();

    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT COUNT (id) FROM Entries WHERE id=:id;"));
    query.bindValue(QStringLiteral(":id"), entry->id());
    Database::instance().execute(query);
    query.next();

    if (query.value(0).toInt() != 0)
        return false; // entry already exists

    query.prepare(QStringLiteral("INSERT INTO Entries VALUES (:feed, :id, :title, :content, :created, :updated, :link, :read, :new, :hasEnclosure, :image);"));
    query.bindValue(QStringLiteral(":feed"), url);
    query.bindValue(QStringLiteral(":id"), entry->id());
    query.bindValue(QStringLiteral(":title"), QTextDocumentFragment::fromHtml(entry->title()).toPlainText());
    query.bindValue(QStringLiteral(":created"), static_cast<int>(entry->datePublished()));
    query.bindValue(QStringLiteral(":updated"), static_cast<int>(entry->dateUpdated()));
    query.bindValue(QStringLiteral(":link"), entry->link());
    query.bindValue(QStringLiteral(":hasEnclosure"), entry->enclosures().length() == 0 ? 0 : 1);
    query.bindValue(QStringLiteral(":read"), isNewFeed); // if new feed, then mark all as read
    query.bindValue(QStringLiteral(":new"), !isNewFeed); // if new feed, then mark none as new

    if (!entry->content().isEmpty())
        query.bindValue(QStringLiteral(":content"), entry->content());
    else
        query.bindValue(QStringLiteral(":content"), entry->description());

    // Look for image in itunes tags
    QString image;
    if (otherItems.value(QStringLiteral("http://www.itunes.com/dtds/podcast-1.0.dtdimage")).hasAttribute(QStringLiteral("href"))) {
        image = otherItems.value(QStringLiteral("http://www.itunes.com/dtds/podcast-1.0.dtdimage")).attribute(QStringLiteral("href"));
    }
    if (image.startsWith(QStringLiteral("/")))
        image = QUrl(url).adjusted(QUrl::RemovePath).toString() + image;
    query.bindValue(QStringLiteral(":image"), image);
    qCDebug(kastsFetcher) << "Entry image found" << image;

    Database::instance().execute(query);

    if (entry->authors().count() > 0) {
        for (const auto &author : entry->authors()) {
            processAuthor(url, entry->id(), author->name(), author->uri(), author->email());
        }
    } else {
        // As fallback, check if there is itunes "author" information
        QString authorName = otherItems.value(QStringLiteral("http://www.itunes.com/dtds/podcast-1.0.dtdauthor")).text();
        if (!authorName.isEmpty())
            processAuthor(url, entry->id(), authorName, QLatin1String(""), QLatin1String(""));
    }

    for (const auto &enclosure : entry->enclosures()) {
        processEnclosure(enclosure, entry, url);
    }

    Q_EMIT entryAdded(url, entry->id());
    return true; // this is a new entry
}

void Fetcher::processAuthor(const QString &url, const QString &entryId, const QString &authorName, const QString &authorUri, const QString &authorEmail)
{
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT COUNT (id) FROM Authors WHERE feed=:feed AND id=:id AND name=:name;"));
    query.bindValue(QStringLiteral(":feed"), url);
    query.bindValue(QStringLiteral(":id"), entryId);
    query.bindValue(QStringLiteral(":name"), authorName);
    Database::instance().execute(query);
    query.next();

    if (query.value(0).toInt() != 0)
        query.prepare(QStringLiteral("UPDATE Authors SET feed=:feed, id=:id, name=:name, uri=:uri, email=:email WHERE feed=:feed AND id=:id;"));
    else
        query.prepare(QStringLiteral("INSERT INTO Authors VALUES(:feed, :id, :name, :uri, :email);"));

    query.bindValue(QStringLiteral(":feed"), url);
    query.bindValue(QStringLiteral(":id"), entryId);
    query.bindValue(QStringLiteral(":name"), authorName);
    query.bindValue(QStringLiteral(":uri"), authorUri);
    query.bindValue(QStringLiteral(":email"), authorEmail);
    Database::instance().execute(query);
}

void Fetcher::processEnclosure(Syndication::EnclosurePtr enclosure, Syndication::ItemPtr entry, const QString &feedUrl)
{
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT COUNT (id) FROM Enclosures WHERE feed=:feed AND id=:id;"));
    query.bindValue(QStringLiteral(":feed"), feedUrl);
    query.bindValue(QStringLiteral(":id"), entry->id());
    Database::instance().execute(query);
    query.next();

    if (query.value(0).toInt() != 0)
        query.prepare(QStringLiteral("UPDATE Enclosures SET feed=:feed, id=:id, duration=:duration, size=:size, title=:title, type=:type, url=:url;"));
    else
        query.prepare(QStringLiteral("INSERT INTO Enclosures VALUES (:feed, :id, :duration, :size, :title, :type, :url, :playposition, :downloaded);"));

    query.bindValue(QStringLiteral(":feed"), feedUrl);
    query.bindValue(QStringLiteral(":id"), entry->id());
    query.bindValue(QStringLiteral(":duration"), enclosure->duration());
    query.bindValue(QStringLiteral(":size"), enclosure->length());
    query.bindValue(QStringLiteral(":title"), enclosure->title());
    query.bindValue(QStringLiteral(":type"), enclosure->type());
    query.bindValue(QStringLiteral(":url"), enclosure->url());
    query.bindValue(QStringLiteral(":playposition"), 0);
    query.bindValue(QStringLiteral(":downloaded"), Enclosure::statusToDb(Enclosure::Downloadable));
    Database::instance().execute(query);
}

QString Fetcher::image(const QString &url) const
{
    if (url.isEmpty()) {
        return QLatin1String("no-image");
    }

    // if image is already cached, then return the path
    QString path = StorageManager::instance().imagePath(url);
    if (QFileInfo::exists(path)) {
        if (QFileInfo(path).size() != 0) {
            return QUrl::fromLocalFile(path).toString();
        }
    }

    // if image has not yet been cached, then check for network connectivity if
    // possible; and download the image
    if (canCheckNetworkStatus()) {
        if (networkConnected() && (!isMeteredConnection() || SettingsManager::self()->allowMeteredImageDownloads())) {
            download(url, path);
        } else {
            return QLatin1String("no-image");
        }
    } else {
        download(url, path);
    }

    return QLatin1String("fetching");
}

QNetworkReply *Fetcher::download(const QString &url, const QString &filePath) const
{
    QNetworkRequest request((QUrl(url)));
    request.setTransferTimeout();

    QFile *file = new QFile(filePath);
    int resumedAt = 0;
    if (file->exists() && file->size() > 0) {
        // try to resume download
        resumedAt = file->size();
        qCDebug(kastsFetcher) << "Resuming download at" << resumedAt << "bytes";
        QByteArray rangeHeaderValue = QByteArray("bytes=") + QByteArray::number(resumedAt) + QByteArray("-");
        request.setRawHeader(QByteArray("Range"), rangeHeaderValue);
        file->open(QIODevice::WriteOnly | QIODevice::Append);
    } else {
        qCDebug(kastsFetcher) << "Starting new download";
        file->open(QIODevice::WriteOnly);
    }

    /*
    QNetworkReply *headerReply = head(request);
    connect(headerReply, &QNetworkReply::finished, this, [=]() {
        if (headerReply->isOpen()) {
            int fileSize = headerReply->header(QNetworkRequest::ContentLengthHeader).toInt();
            qCDebug(kastsFetcher) << "Reported download size" << fileSize;
        }
        headerReply->deleteLater();
    });
    */

    QNetworkReply *reply = get(request);

    connect(reply, &QNetworkReply::readyRead, this, [=]() {
        if (reply->isOpen() && file) {
            QByteArray data = reply->readAll();
            file->write(data);
        }
    });

    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->isOpen() && file) {
            QByteArray data = reply->readAll();
            file->write(data);
            file->close();

            Q_EMIT downloadFinished(url);
        }

        // clean up; close file if still open in case something has gone wrong
        if (file) {
            if (file->isOpen()) {
                file->close();
            }
            delete file;
        }
        reply->deleteLater();
    });

    return reply;
}

QNetworkReply *Fetcher::get(QNetworkRequest &request) const
{
    setHeader(request);
    return manager->get(request);
}

QNetworkReply *Fetcher::head(QNetworkRequest &request) const
{
    setHeader(request);
    return manager->head(request);
}

void Fetcher::setHeader(QNetworkRequest &request) const
{
    request.setRawHeader(QByteArray("User-Agent"), QByteArray("Kasts/") + QByteArray(KASTS_VERSION_STRING) + QByteArray("; Syndication"));
}

bool Fetcher::canCheckNetworkStatus() const
{
#if !defined Q_OS_ANDROID && !defined Q_OS_WIN
    qCDebug(kastsFetcher) << "Can NetworkManager be reached?" << m_nmInterface->isValid();
    return (m_nmInterface && m_nmInterface->isValid());
#else
    return false;
#endif
}

bool Fetcher::networkConnected() const
{
#if !defined Q_OS_ANDROID && !defined Q_OS_WIN
    qCDebug(kastsFetcher) << "Network connected?" << (m_nmInterface->state() >= 70) << m_nmInterface->state();
    return (m_nmInterface && m_nmInterface->state() >= 70);
#else
    return true;
#endif
}

bool Fetcher::isMeteredConnection() const
{
#if !defined Q_OS_ANDROID && !defined Q_OS_WIN
    if (canCheckNetworkStatus()) {
        // Get network connection status through DBus (NetworkManager)
        // state == 1: explicitly configured as metered
        // state == 3: connection guessed as metered
        uint state = m_nmInterface->metered();
        qCDebug(kastsFetcher) << "Network Status:" << state;
        qCDebug(kastsFetcher) << "Connection is metered?" << (state == 1 || state == 3);
        return (state == 1 || state == 3);
    } else {
        return false;
    }
#else
    // TODO: get network connection type for Android and windows
    return false;
#endif
}
