/**
 * Copyright 2020 Tobias Fella <fella@posteo.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <QFile>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QStandardPaths>
#include <QTextDocumentFragment>

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

void Fetcher::fetch(QUrl url)
{
    qDebug() << "Starting to fetch" << url.toString();

    QNetworkRequest request(url);
    QNetworkReply *reply = manager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, url, reply]() {
        QByteArray data = reply->readAll();
        Syndication::DocumentSource *document = new Syndication::DocumentSource(data, url.toString());
        Syndication::FeedPtr feed = Syndication::parserCollection()->parse(*document, QStringLiteral("Atom"));

        if (feed.isNull())
            return;

        QSqlQuery query;

        query.prepare(QStringLiteral("UPDATE Feeds SET name=:name, image=:image WHERE url=:url;"));
        query.bindValue(QStringLiteral(":name"), feed->title());
        query.bindValue(QStringLiteral(":url"), url.toString());
        if (feed->image()->url().startsWith(QStringLiteral("/"))) {
            QString absolute = url.adjusted(QUrl::RemovePath).toString() + feed->image()->url();
            query.bindValue(QStringLiteral(":image"), absolute);
        } else
            query.bindValue(QStringLiteral(":image"), feed->image()->url());
        Database::instance().execute(query);
        qDebug() << "Updated feed title:" << feed->title();

        for (const auto &entry : feed->items()) {
            qDebug() << "Processing" << entry->title();
            query.prepare(QStringLiteral("SELECT COUNT (id) FROM Entries WHERE id=:id;"));
            query.bindValue(QStringLiteral(":id"), entry->id());
            Database::instance().execute(query);
            query.next();
            if (query.value(0).toInt() != 0)
                continue;
            query.prepare(QStringLiteral("INSERT INTO Entries VALUES (:feed, :id, :title, :content, :created, :updated, :link);"));
            query.bindValue(QStringLiteral(":feed"), url.toString());
            query.bindValue(QStringLiteral(":id"), entry->id());
            query.bindValue(QStringLiteral(":title"), QTextDocumentFragment::fromHtml(entry->title()).toPlainText());
            query.bindValue(QStringLiteral(":created"), static_cast<int>(entry->datePublished()));
            query.bindValue(QStringLiteral(":updated"), static_cast<int>(entry->dateUpdated()));
            query.bindValue(QStringLiteral(":link"), entry->link());
            if (!entry->content().isEmpty())
                query.bindValue(QStringLiteral(":content"), entry->content());
            else
                query.bindValue(QStringLiteral(":content"), entry->description());
            Database::instance().execute(query);
            for (const auto &author : entry->authors()) {
                query.prepare(QStringLiteral("INSERT INTO Authors VALUES(:feed, :id, :name, :uri, :email);"));
                query.bindValue(QStringLiteral(":feed"), url.toString());
                query.bindValue(QStringLiteral(":id"), entry->id());
                query.bindValue(QStringLiteral(":name"), author->name());
                query.bindValue(QStringLiteral(":uri"), author->uri());
                query.bindValue(QStringLiteral(":email"), author->email());
                Database::instance().execute(query);
            }
        }

        emit updated();
        delete reply;
    });
}

QString Fetcher::image(QString url)
{
    QString path = imagePath(url);
    if (QFileInfo(path).exists()) {
        return path;
    }

    QNetworkRequest request((QUrl(url)));
    QNetworkReply *reply = manager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, url, reply, path]() {
        QByteArray data = reply->readAll();
        QFile file(path);
        file.open(QIODevice::WriteOnly);
        file.write(data);
        file.close();

        emit updated();
        delete reply;
    });

    return QLatin1String("");
}

void Fetcher::removeImage(QString url)
{
    qDebug() << imagePath(url);
    QFile(imagePath(url)).remove();
}

QString Fetcher::imagePath(QString url)
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/") + QString::fromStdString(QCryptographicHash::hash(url.toUtf8(), QCryptographicHash::Md5).toHex().toStdString());
}
