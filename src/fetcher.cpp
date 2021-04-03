/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QStandardPaths>
#include <QTextDocumentFragment>
#include <QDomElement>
#include <QMultiMap>

#include <Syndication/Syndication>

#include "database.h"
#include "fetcher.h"

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

    QSqlQuery query;
    query.prepare(QStringLiteral("UPDATE Feeds SET name=:name, image=:image, link=:link, description=:description, lastUpdated=:lastUpdated WHERE url=:url;"));
    query.bindValue(QStringLiteral(":name"), feed->title());
    query.bindValue(QStringLiteral(":url"), url);
    query.bindValue(QStringLiteral(":link"), feed->link());
    query.bindValue(QStringLiteral(":description"), feed->description());

    QDateTime current = QDateTime::currentDateTime();
    query.bindValue(QStringLiteral(":lastUpdated"), current.toSecsSinceEpoch());

    for (auto &author : feed->authors()) {
        processAuthor(author, QLatin1String(""), url);
    }

    QString image;
    if (feed->image()->url().startsWith(QStringLiteral("/")))
        image = QUrl(url).adjusted(QUrl::RemovePath).toString() + feed->image()->url();
    else
        image = feed->image()->url();
    query.bindValue(QStringLiteral(":image"), image);
    Database::instance().execute(query);

    qDebug() << "Updated feed title:" << feed->title();

    Q_EMIT feedDetailsUpdated(url, feed->title(), image, feed->link(), feed->description(), current);

    for (const auto &entry : feed->items()) {
        processEntry(entry, url);
    }

    Q_EMIT feedUpdated(url);
}

void Fetcher::processEntry(Syndication::ItemPtr entry, const QString &url)
{
    qDebug() << "Processing" << entry->title();
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT COUNT (id) FROM Entries WHERE id=:id;"));
    query.bindValue(QStringLiteral(":id"), entry->id());
    Database::instance().execute(query);
    query.next();

    if (query.value(0).toInt() != 0)
        return;

    query.prepare(QStringLiteral("INSERT INTO Entries VALUES (:feed, :id, :title, :content, :created, :updated, :link, 0, :hasEnclosure);"));
    query.bindValue(QStringLiteral(":feed"), url);
    query.bindValue(QStringLiteral(":id"), entry->id());
    query.bindValue(QStringLiteral(":title"), QTextDocumentFragment::fromHtml(entry->title()).toPlainText());
    query.bindValue(QStringLiteral(":created"), static_cast<int>(entry->datePublished()));
    query.bindValue(QStringLiteral(":updated"), static_cast<int>(entry->dateUpdated()));
    query.bindValue(QStringLiteral(":link"), entry->link());
    query.bindValue(QStringLiteral(":hasEnclosure"), entry->enclosures().length() == 0 ? 0 : 1);

    if (!entry->content().isEmpty())
        query.bindValue(QStringLiteral(":content"), entry->content());
    else
        query.bindValue(QStringLiteral(":content"), entry->description());

    Database::instance().execute(query);

    for (const auto &author : entry->authors()) {
        processAuthor(author, entry->id(), url);
    }

    for (const auto &enclosure : entry->enclosures()) {
        processEnclosure(enclosure, entry, url);
    }
    QMultiMap<QString, QDomElement> otherItems = entry->additionalProperties();
    qDebug() << "other items" << otherItems.value(QStringLiteral("http://www.itunes.com/dtds/podcast-1.0.dtdimage")).attribute(QStringLiteral("href"));
    Q_EMIT entryAdded(url, entry->id());
}

void Fetcher::processAuthor(Syndication::PersonPtr author, const QString &entryId, const QString &url)
{
    QSqlQuery query;
    query.prepare(QStringLiteral("INSERT INTO Authors VALUES(:feed, :id, :name, :uri, :email);"));
    query.bindValue(QStringLiteral(":feed"), url);
    query.bindValue(QStringLiteral(":id"), entryId);
    query.bindValue(QStringLiteral(":name"), author->name());
    query.bindValue(QStringLiteral(":uri"), author->uri());
    query.bindValue(QStringLiteral(":email"), author->email());
    Database::instance().execute(query);
}

void Fetcher::processEnclosure(Syndication::EnclosurePtr enclosure, Syndication::ItemPtr entry, const QString &feedUrl)
{
    QSqlQuery query;
    query.prepare(QStringLiteral("INSERT INTO Enclosures VALUES (:feed, :id, :duration, :size, :title, :type, :url);"));
    query.bindValue(QStringLiteral(":feed"), feedUrl);
    query.bindValue(QStringLiteral(":id"), entry->id());
    query.bindValue(QStringLiteral(":duration"), enclosure->duration());
    query.bindValue(QStringLiteral(":size"), enclosure->length());
    query.bindValue(QStringLiteral(":title"), enclosure->title());
    query.bindValue(QStringLiteral(":type"), enclosure->type());
    query.bindValue(QStringLiteral(":url"), enclosure->url());
    Database::instance().execute(query);
}

QString Fetcher::image(const QString &url)
{
    QString path = filePath(url);
    if (QFileInfo::exists(path)) {
        return path;
    }

    download(url);

    return QLatin1String("");
}

QNetworkReply *Fetcher::download(QString url)
{
    QNetworkRequest request((QUrl(url)));
    QNetworkReply *reply = get(request);
    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->isOpen()) {
            QByteArray data = reply->readAll();
            QFile file(filePath(url));
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
    qDebug() << filePath(url);
    QFile(filePath(url)).remove();
}

QString Fetcher::filePath(const QString &url)
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/") + QString::fromStdString(QCryptographicHash::hash(url.toUtf8(), QCryptographicHash::Md5).toHex().toStdString());
}

QNetworkReply *Fetcher::get(QNetworkRequest &request)
{
    request.setRawHeader("User-Agent", "Alligator/0.1; Syndication");
    return manager->get(request);
}
