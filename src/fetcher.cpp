/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QStandardPaths>
#include <QTextDocumentFragment>
#include <QDomElement>
#include <QMultiMap>

#include <Syndication/Syndication>

#include "database.h"
#include "fetcher.h"
#include "settingsmanager.h"

Fetcher::Fetcher()
{
    manager = new QNetworkAccessManager(this);
    manager->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
    manager->setStrictTransportSecurityEnabled(true);
    manager->enableStrictTransportSecurityStore(true);
}

void Fetcher::fetch(const QString &url)
{
    qDebug() << "Starting to fetch" << url;

    Q_EMIT startedFetchingFeed(url);

    QNetworkRequest request((QUrl(url)));
    QNetworkReply *reply = get(request);
    connect(reply, &QNetworkReply::finished, this, [this, url, reply]() {
        if(reply->error()) {
            qWarning() << "Error fetching feed";
            qWarning() << reply->errorString();
            Q_EMIT error(url, reply->error(), reply->errorString());
        } else {
            QByteArray data = reply->readAll();
            Syndication::DocumentSource *document = new Syndication::DocumentSource(data, url);
            Syndication::FeedPtr feed = Syndication::parserCollection()->parse(*document, QStringLiteral("Atom"));
            processFeed(feed, url);
        }
        delete reply;
    });
}

void Fetcher::fetchAll()
{
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT url FROM Feeds;"));
    Database::instance().execute(query);
    while (query.next()) {
        fetch(query.value(0).toString());
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
        qDebug() << "Feed not found in database" << url;
        return;
    }
    if (isNewFeed) qDebug() << "New feed" << feed->title() << ":" << isNewFeed;

    // Retrieve "other" fields; this will include the "itunes" tags
    QMultiMap<QString, QDomElement> otherItems = feed->additionalProperties();

    query.prepare(QStringLiteral("UPDATE Feeds SET name=:name, image=:image, link=:link, description=:description, lastUpdated=:lastUpdated, new=:new WHERE url=:url;"));
    query.bindValue(QStringLiteral(":name"), feed->title());
    query.bindValue(QStringLiteral(":url"), url);
    query.bindValue(QStringLiteral(":link"), feed->link());
    query.bindValue(QStringLiteral(":description"), feed->description());
    query.bindValue(QStringLiteral(":new"), false); // set "new" to false now that new feed is being processed

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
            for (int i=0; i < nodelist.length(); i++) {
                if (nodelist.item(i).nodeName() == QStringLiteral("itunes:name")) {
                    authorname = nodelist.item(i).toElement().text();
                } else if (nodelist.item(i).nodeName() == QStringLiteral("itunes:email")) {
                    authoremail = nodelist.item(i).toElement().text();
                }
            }
        } else {
            authorname = otherItems.value(QStringLiteral("http://www.itunes.com/dtds/podcast-1.0.dtdauthor")).text();
            qDebug() << "authorname" << authorname;
        }
        if (!authorname.isEmpty()) processAuthor(url, QLatin1String(""), authorname, QLatin1String(""), authoremail);
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

    qDebug() << "Updated feed:" << feed->title();

    Q_EMIT feedDetailsUpdated(url, feed->title(), image, feed->link(), feed->description(), current);

    for (const auto &entry : feed->items()) {
        QCoreApplication::processEvents(); // keep the main thread semi-responsive
        processEntry(entry, url, isNewFeed);
    }

    // Now mark the appropriate number of recent entries "new" and "read" only for new feeds
    if (isNewFeed) {
        query.prepare(QStringLiteral("SELECT * FROM Entries WHERE feed=:feed ORDER BY updated DESC LIMIT :recentNew;"));
        query.bindValue(QStringLiteral(":feed"), url);
        query.bindValue(QStringLiteral(":recentNew"), SettingsManager::self()->numberNewEpisodes());
        Database::instance().execute(query);
        QSqlQuery updateQuery;
        while (query.next()) {
            qDebug() << "Marked as new:" << query.value(QStringLiteral("id")).toString();
            updateQuery.prepare(QStringLiteral("UPDATE Entries SET read=:read, new=:new WHERE id=:id AND feed=:feed;"));
            updateQuery.bindValue(QStringLiteral(":read"), false);
            updateQuery.bindValue(QStringLiteral(":new"), true);
            updateQuery.bindValue(QStringLiteral(":feed"), url);
            updateQuery.bindValue(QStringLiteral(":id"), query.value(QStringLiteral("id")).toString());
            Database::instance().execute(updateQuery);
        }
    }

    Q_EMIT feedUpdated(url);
}

void Fetcher::processEntry(Syndication::ItemPtr entry, const QString &url, const bool &isNewFeed)
{
    qDebug() << "Processing" << entry->title();

    // Retrieve "other" fields; this will include the "itunes" tags
    QMultiMap<QString, QDomElement> otherItems = entry->additionalProperties();

    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT COUNT (id) FROM Entries WHERE id=:id;"));
    query.bindValue(QStringLiteral(":id"), entry->id());
    Database::instance().execute(query);
    query.next();

    if (query.value(0).toInt() != 0)
        return;

    query.prepare(QStringLiteral("INSERT INTO Entries VALUES (:feed, :id, :title, :content, :created, :updated, :link, :read, :new, :hasEnclosure, :image);"));
    query.bindValue(QStringLiteral(":feed"), url);
    query.bindValue(QStringLiteral(":id"), entry->id());
    query.bindValue(QStringLiteral(":title"), QTextDocumentFragment::fromHtml(entry->title()).toPlainText());
    query.bindValue(QStringLiteral(":created"), static_cast<int>(entry->datePublished()));
    query.bindValue(QStringLiteral(":updated"), static_cast<int>(entry->dateUpdated()));
    query.bindValue(QStringLiteral(":link"), entry->link());
    query.bindValue(QStringLiteral(":hasEnclosure"), entry->enclosures().length() == 0 ? 0 : 1);
    query.bindValue(QStringLiteral(":read"), isNewFeed);  // if new feed, then mark all as read
    query.bindValue(QStringLiteral(":new"), !isNewFeed);  // if new feed, then mark none as new

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
    //qDebug() << "Entry image found" << image;

    Database::instance().execute(query);

    if (entry->authors().count() > 0) {
        for (const auto &author : entry->authors()) {
            processAuthor(url, entry->id(), author->name(), author->uri(), author->email());
        }
    } else {
        // As fallback, check if there is itunes "author" information
        QString authorName = otherItems.value(QStringLiteral("http://www.itunes.com/dtds/podcast-1.0.dtdauthor")).text();
        if (!authorName.isEmpty()) processAuthor(url, entry->id(), authorName, QLatin1String(""), QLatin1String(""));
    }

    for (const auto &enclosure : entry->enclosures()) {
        processEnclosure(enclosure, entry, url);
    }

    Q_EMIT entryAdded(url, entry->id());
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
    query.bindValue(QStringLiteral(":downloaded"), false);
    Database::instance().execute(query);
}

QString Fetcher::image(const QString& url) const
{
    QString path = imagePath(url);
    if (QFileInfo::exists(path)) {
        if (QFileInfo(path).size() != 0 )
            return path;
    }

    download(url, path);

    return QLatin1String("");
}

QNetworkReply *Fetcher::download(const QString &url, const QString &filePath) const
{
    QNetworkRequest request((QUrl(url)));
    QNetworkReply *reply = get(request);
    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->isOpen()) {
            QByteArray data = reply->readAll();
            QFile file(filePath);
            file.open(QIODevice::WriteOnly);
            file.write(data);
            file.close();

            Q_EMIT downloadFinished(url);
        }
        reply->deleteLater();
    });

    return reply;
}

void Fetcher::removeImage(const QString &url)
{
    qDebug() << imagePath(url);
    QFile(imagePath(url)).remove();
}

QString Fetcher::imagePath(const QString &url) const
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QStringLiteral("/images/");
    // Create path in cache if it doesn't exist yet
    QFileInfo().absoluteDir().mkpath(path);
    return path + QString::fromStdString(QCryptographicHash::hash(url.toUtf8(), QCryptographicHash::Md5).toHex().toStdString());
}


QString Fetcher::enclosurePath(const QString &url) const
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + QStringLiteral("/enclosures/");
    // Create path in cache if it doesn't exist yet
    QFileInfo().absoluteDir().mkpath(path);
    return path + QString::fromStdString(QCryptographicHash::hash(url.toUtf8(), QCryptographicHash::Md5).toHex().toStdString());
}

QNetworkReply *Fetcher::get(QNetworkRequest &request) const
{
    request.setRawHeader("User-Agent", "Alligator/0.1; Syndication");
    return manager->get(request);
}
