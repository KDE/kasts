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
#include <QSqlQuery>
#include <QUrl>
#include <QVariant>

#include "database.h"
#include "feedListModel.h"
#include "fetcher.h"

FeedListModel::FeedListModel(QObject *parent)
    : QAbstractListModel(parent)
{
    connect(&Database::instance(), &Database::feedAdded, this, [this]() {
        beginInsertRows(QModelIndex(), rowCount(QModelIndex()) - 1, rowCount(QModelIndex()) - 1);
        endInsertRows();
    });
    connect(&Fetcher::instance(), &Fetcher::feedDetailsUpdated, this, [this](QString url, QString name, QString image, QString link, QString description, QDateTime lastUpdated) {
        for (int i = rowCount(QModelIndex()) - 1; i >= 0; i--) {
            if (m_feeds[i]->url() == url) {
                m_feeds[i]->setName(name);
                m_feeds[i]->setImage(image);
                m_feeds[i]->setLink(link);
                m_feeds[i]->setDescription(description);
                m_feeds[i]->setLastUpdated(lastUpdated);
                Q_EMIT dataChanged(createIndex(i, 0), createIndex(i, 0));
                break;
            }
        }
    });
}

QHash<int, QByteArray> FeedListModel::roleNames() const
{
    QHash<int, QByteArray> roleNames;
    roleNames[0] = "feed";
    return roleNames;
}

int FeedListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT COUNT() FROM Feeds;"));
    Database::instance().execute(query);
    if (!query.next())
        qWarning() << "Failed to query feed count";
    return query.value(0).toInt();
}

QVariant FeedListModel::data(const QModelIndex &index, int role) const
{
    if (role != 0)
        return QVariant();
    if (m_feeds[index.row()] == nullptr)
        loadFeed(index.row());
    return QVariant::fromValue(m_feeds[index.row()]);
}

void FeedListModel::loadFeed(int index) const
{
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT * FROM Feeds LIMIT 1 OFFSET :index;"));
    query.bindValue(QStringLiteral(":index"), index);
    Database::instance().execute(query);
    if (!query.next())
        qWarning() << "Failed to load feed" << index;

    QSqlQuery authorQuery;
    authorQuery.prepare(QStringLiteral("SELECT * FROM Authors WHERE id='' AND feed=:feed"));
    authorQuery.bindValue(QStringLiteral(":feed"), query.value(QStringLiteral("url")).toString());
    Database::instance().execute(authorQuery);
    QVector<Author *> authors;
    while (authorQuery.next()) {
        authors += new Author(authorQuery.value(QStringLiteral("name")).toString(), authorQuery.value(QStringLiteral("email")).toString(), authorQuery.value(QStringLiteral("uri")).toString(), nullptr);
    }

    QDateTime subscribed;
    subscribed.setSecsSinceEpoch(query.value(QStringLiteral("subscribed")).toInt());

    QDateTime lastUpdated;
    lastUpdated.setSecsSinceEpoch(query.value(QStringLiteral("lastUpdated")).toInt());

    Feed *feed = new Feed(query.value(QStringLiteral("url")).toString(),
                          query.value(QStringLiteral("name")).toString(),
                          query.value(QStringLiteral("image")).toString(),
                          query.value(QStringLiteral("link")).toString(),
                          query.value(QStringLiteral("description")).toString(),
                          authors,
                          query.value(QStringLiteral("deleteAfterCount")).toInt(),
                          query.value(QStringLiteral("deleteAfterType")).toInt(),
                          subscribed,
                          lastUpdated,
                          query.value(QStringLiteral("autoUpdateCount")).toInt(),
                          query.value(QStringLiteral("autoUpdateType")).toInt(),
                          query.value(QStringLiteral("notify")).toBool(),
                          nullptr);
    m_feeds[index] = feed;
}

void FeedListModel::removeFeed(int index)
{
    Feed *feed = m_feeds[index];
    beginRemoveRows(QModelIndex(), index, index);
    m_feeds[index] = nullptr;
    endRemoveRows();
    feed->remove();
    delete feed;
}

void FeedListModel::refreshAll()
{
    for (auto &feed : m_feeds) {
        feed->refresh();
    }
}