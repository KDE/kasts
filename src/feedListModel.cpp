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
#include <QSqlRecord>
#include <QDebug>
#include <QModelIndex>
#include <QSqlError>

#include "feedListModel.h"
#include "fetcher.h"
#include "database.h"

FeedListModel::FeedListModel(QObject *parent)
    : QSqlTableModel(parent)
{
    setTable("Feeds");
    setSort(0, Qt::AscendingOrder);
    setEditStrategy(OnFieldChange);
    select();
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
    qDebug() << "Adding feed";
    if(feedExists(url)) {
        qDebug() << "Feed already exists";
        return;
    }
    qDebug() << "Feed does not yet exist";

    QSqlRecord rec = record();
    rec.setValue(0, url);
    rec.setValue(1, url);
    rec.setValue(2, "");

    insertRecord(-1, rec);

    connect(&Fetcher::instance(), &Fetcher::updated, this, [this]() {
        select();

        disconnect(&Fetcher::instance(), &Fetcher::updated, nullptr, nullptr);
    });
    Fetcher::instance().fetch(QUrl(url));
}

QVariant FeedListModel::data(const QModelIndex &index, int role) const
{
    return QSqlTableModel::data(createIndex(index.row(), role), 0);
}

bool FeedListModel::feedExists(QString url) {
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT COUNT (url) FROM Feeds WHERE url=:url;"));
    query.bindValue(QStringLiteral(":url"), url);
    Database::instance().execute(query);
    query.next();
    return query.value(0).toInt() != 0;
}

void FeedListModel::removeFeed(int index)
{
    QSqlQuery query;
    query.prepare(QStringLiteral("DELETE FROM Entries WHERE feed=:feed;"));
    query.bindValue(QStringLiteral(":feed"), data(createIndex(index, 0), 1).toString());
    Database::instance().execute(query);

    //Workaround...
    query.prepare(QStringLiteral("DELETE FROM Feeds WHERE url=:url;"));
    query.bindValue(QStringLiteral(":url"), data(createIndex(index, 0), 1).toString());
    Database::instance().execute(query);
    select();
}
