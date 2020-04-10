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

#include <QUrl>

#include "feedListModel.h"
#include "fetcher.h"
#include "database.h"

FeedListModel::FeedListModel(QObject *parent)
    : QAbstractListModel(parent)
{
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT name, url, image FROM Feeds"));
    Database::instance().execute(query);
    beginInsertRows(QModelIndex(), 0, query.size());
    while (query.next()) {
        feeds += Feed(query.value(1).toString(), query.value(0).toString(), query.value(2).toString());
    }
    endInsertRows();
}

QHash<int, QByteArray> FeedListModel::roleNames() const
{
    QHash<int, QByteArray> roleNames;
    roleNames[FeedRole] = "feed";
    return roleNames;
}

QVariant FeedListModel::data(const QModelIndex &index, int role) const
{
    if (role == FeedRole) {
        return QVariant::fromValue(feeds[index.row()]);
    }
    return QStringLiteral("DEADBEEF");
}
int FeedListModel::rowCount(const QModelIndex &index) const
{
    return feeds.size();
}

void FeedListModel::addFeed(QString url)
{
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT COUNT (url) FROM Feeds WHERE url=:url;"));
    query.bindValue(QStringLiteral(":url"), url);
    Database::instance().execute(query);
    query.next();
    if(query.value(0).toInt() != 0) return;
    connect(&Fetcher::instance(), &Fetcher::finished, this, [this, url]() {
        QSqlQuery query;
        query.prepare(QStringLiteral("SELECT name, image FROM Feeds WHERE url=:url;"));
        query.bindValue(QStringLiteral(":url"), url);
        Database::instance().execute(query);
        query.next();
        for(int i = 0; i < feeds.length(); i++) {
            if(feeds[i].url() == url) {
                feeds.removeAt(i);
                feeds.insert(i, Feed(url, query.value(0).toString(), query.value(1).toString()));
                emit dataChanged(index(i), index(i));
                break;
            }
        }
    });
    Fetcher::instance().fetch(QUrl(url));
    beginInsertRows(QModelIndex(), feeds.size(), feeds.size());
    feeds.append(Feed(url));
    endInsertRows();

    query.prepare(QStringLiteral("INSERT INTO Feeds VALUES (:url, :name, '');"));
    query.bindValue(QStringLiteral(":url"), url);
    query.bindValue(QStringLiteral(":name"), url);
    Database::instance().execute(query);
}

void FeedListModel::remove_feed(int index)
{
    Feed toRemove = feeds[index];
    QSqlQuery query;
    query.prepare(QStringLiteral("DELETE FROM Feeds WHERE name=:name AND url=url;"));
    query.bindValue(QStringLiteral(":url"), toRemove.url());
    query.bindValue(QStringLiteral(":name"), toRemove.name());
    Database::instance().execute(query);
    beginRemoveRows(QModelIndex(), index, index);
    feeds.remove(index);
    endRemoveRows();
}
