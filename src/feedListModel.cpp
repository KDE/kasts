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

#include <QDebug>
#include <QModelIndex>
#include <QSqlRecord>
#include <QUrl>

#include "database.h"
#include "feedListModel.h"
#include "fetcher.h"

FeedListModel::FeedListModel(QObject *parent)
    : QSqlTableModel(parent)
{
    setTable(QStringLiteral("Feeds"));
    setSort(0, Qt::AscendingOrder);
    setEditStrategy(OnFieldChange);
    select();

    connect(&Fetcher::instance(), &Fetcher::updated, this, [this]() { select(); });
}

QHash<int, QByteArray> FeedListModel::roleNames() const
{
    QHash<int, QByteArray> roleNames;
    roleNames[Name] = "name";
    roleNames[Url] = "url";
    roleNames[Image] = "image";
    return roleNames;
}

void FeedListModel::addFeed(QString url)
{
    Database::instance().addFeed(url);
}

QVariant FeedListModel::data(const QModelIndex &index, int role) const
{
    return QSqlTableModel::data(createIndex(index.row(), role), 0);
}

void FeedListModel::removeFeed(int index)
{
    Fetcher::instance().removeImage(data(createIndex(index, 0), Image).toString());
    QSqlQuery query;
    query.prepare(QStringLiteral("DELETE FROM Authors WHERE feed=:feed;"));
    query.bindValue(QStringLiteral(":feed"), data(createIndex(index, 0), 1).toString());
    Database::instance().execute(query);

    query.prepare(QStringLiteral("DELETE FROM Entries WHERE feed=:feed;"));
    query.bindValue(QStringLiteral(":feed"), data(createIndex(index, 0), 1).toString());
    Database::instance().execute(query);

    query.prepare(QStringLiteral("DELETE FROM Enclosures WHERE feed=:feed;"));
    query.bindValue(QStringLiteral(":feed"), data(createIndex(index, 0), 1).toString());
    Database::instance().execute(query);

    // Workaround...
    query.prepare(QStringLiteral("DELETE FROM Feeds WHERE url=:url;"));
    query.bindValue(QStringLiteral(":url"), data(createIndex(index, 0), 1).toString());
    Database::instance().execute(query);
    select();
}
