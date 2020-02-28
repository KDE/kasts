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

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QUrl>

#include <syndication.h>

#include "feedListModel.h"
#include "fetcher.h"

FeedListModel::FeedListModel(QObject *parent)
    : QAbstractListModel(parent)
{
    QSqlQuery query(QSqlDatabase::database());
    query.exec(QStringLiteral("SELECT name, url FROM Feeds"));
    beginInsertRows(QModelIndex(), 0, query.size());
    while (query.next()) {
        feeds += Feed(query.value(1).toString(), query.value(0).toString());
    }
    endInsertRows();
}

QHash<int, QByteArray> FeedListModel::roleNames() const
{
    QHash<int, QByteArray> roleNames;
    roleNames[Qt::DisplayRole] = "display";
    roleNames[Url] = "url";
    return roleNames;
}

QVariant FeedListModel::data(const QModelIndex &index, int role) const
{
    if (role == Url)
        return QVariant(this->feeds[index.row()].url());
    if (role == Qt::DisplayRole)
        return QVariant(this->feeds[index.row()].name());
    return QVariant();
}
int FeedListModel::rowCount(const QModelIndex &index) const
{
    return this->feeds.size();
}

void FeedListModel::add_feed(QString url)
{
    Fetcher::instance().fetch(QUrl(url));
    beginInsertRows(QModelIndex(), feeds.size(), feeds.size());
    feeds.append(Feed(url));
    endInsertRows();
    QSqlQuery query(QSqlDatabase::database());
    query.prepare(QStringLiteral("INSERT INTO Feeds VALUES (:url, :name);"));
    query.bindValue(QStringLiteral(":url"), url);
    query.bindValue(QStringLiteral(":name"), url);
    query.exec();
}

void FeedListModel::remove_feed(int index)
{
    Feed toRemove = feeds[index];
    QSqlQuery query(QSqlDatabase::database());
    query.prepare(QStringLiteral("DELETE FROM Feeds WHERE name=:name AND url=url;"));
    query.bindValue(QStringLiteral(":url"), toRemove.url());
    query.bindValue(QStringLiteral(":name"), toRemove.name());
    query.exec();
    beginRemoveRows(QModelIndex(), index, index);
    feeds.remove(index);
    endRemoveRows();
}
