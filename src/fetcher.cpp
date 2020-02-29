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

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSqlDatabase>
#include <QSqlQuery>

#include <Syndication/Syndication>

#include "fetcher.h"

Fetcher::Fetcher() {
}

void Fetcher::fetch(QUrl url)
{
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    manager->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
    manager->setStrictTransportSecurityEnabled(true);
    manager->enableStrictTransportSecurityStore(true);
    QNetworkRequest request = QNetworkRequest(QUrl(url));
    QNetworkReply *reply = manager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, url, reply]() {
        QByteArray data = reply->readAll();
        Syndication::DocumentSource *document = new Syndication::DocumentSource(data, url.toString());
        Syndication::FeedPtr feed = Syndication::parserCollection()->parse(*document, QStringLiteral("Atom"));

        QSqlDatabase db = QSqlDatabase::database();
        QSqlQuery query(db);

        for (const auto &entry : feed->items()) {
            query = QSqlQuery(db);
            query.prepare(QStringLiteral("INSERT INTO Entries VALUES (:feed, :id, :title, :contents);"));
            query.bindValue(QStringLiteral(":feed"), url.toString());
            query.bindValue(QStringLiteral(":id"), entry->id());
            query.bindValue(QStringLiteral(":title"), entry->title());
            query.bindValue(QStringLiteral(":contents"), entry->content());
            query.exec();
            for (const auto &author : entry->authors()) {
                query = QSqlQuery(db);
                query.prepare(QStringLiteral("INSERT INTO Authors VALUES(:id, :name, :uri, :email);"));
                query.bindValue(QStringLiteral(":id"), entry->id());
                query.bindValue(QStringLiteral(":name"), author->name());
                query.bindValue(QStringLiteral(":uri"), author->uri());
                query.bindValue(QStringLiteral(":email"), author->email());
                query.exec();
            }
        }
        delete reply;
        emit finished();
    });
}
