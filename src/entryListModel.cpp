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

#include <QAbstractListModel>
#include <QVariant>
#include <QVector>

#include "database.h"
#include "entryListModel.h"
#include "fetcher.h"

EntryListModel::EntryListModel(QObject *parent)
    : QAbstractListModel(parent)
{
    connect(&Fetcher::instance(), &Fetcher::feedUpdated, this, [this](QString url) {
        if (m_feed->url() == url) {
            beginResetModel();
            for (auto &entry : m_entries) {
                delete entry;
            }
            m_entries.clear();
            endResetModel();
        }
    });
}

QVariant EntryListModel::data(const QModelIndex &index, int role) const
{
    if (role != 0)
        return QVariant();
    if (m_entries[index.row()] == nullptr)
        loadEntry(index.row());
    return QVariant::fromValue(m_entries[index.row()]);
}

QHash<int, QByteArray> EntryListModel::roleNames() const
{
    QHash<int, QByteArray> roleNames;
    roleNames[0] = "entry";
    return roleNames;
}

int EntryListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT COUNT() FROM Entries WHERE feed=:feed;"));
    query.bindValue(QStringLiteral(":feed"), m_feed->url());
    Database::instance().execute(query);
    if (!query.next())
        qWarning() << "Failed to query feed count";
    return query.value(0).toInt();
}

void EntryListModel::loadEntry(int index) const
{
    QSqlQuery entryQuery;
    entryQuery.prepare(QStringLiteral("SELECT * FROM Entries WHERE feed=:feed ORDER BY updated DESC LIMIT 1 OFFSET :index;"));
    entryQuery.bindValue(QStringLiteral(":feed"), m_feed->url());
    entryQuery.bindValue(QStringLiteral(":index"), index);
    Database::instance().execute(entryQuery);
    if (!entryQuery.next())
        qWarning() << "No element with index" << index << "found in feed" << m_feed->url();

    QSqlQuery authorQuery;
    authorQuery.prepare(QStringLiteral("SELECT * FROM Authors WHERE id=:id"));
    authorQuery.bindValue(QStringLiteral(":id"), entryQuery.value(QStringLiteral("id")).toString());
    Database::instance().execute(authorQuery);
    QVector<Author *> authors;
    while (authorQuery.next()) {
        authors += new Author(authorQuery.value(QStringLiteral("name")).toString(), authorQuery.value(QStringLiteral("email")).toString(), authorQuery.value(QStringLiteral("uri")).toString(), nullptr);
    }

    QDateTime created;
    created.setSecsSinceEpoch(entryQuery.value(QStringLiteral("created")).toInt());

    QDateTime updated;
    updated.setSecsSinceEpoch(entryQuery.value(QStringLiteral("updated")).toInt());

    Entry *entry = new Entry(m_feed,
                             entryQuery.value(QStringLiteral("id")).toString(),
                             entryQuery.value(QStringLiteral("title")).toString(),
                             entryQuery.value(QStringLiteral("content")).toString(),
                             authors,
                             created,
                             updated,
                             entryQuery.value(QStringLiteral("link")).toString(),
                             entryQuery.value(QStringLiteral("read")).toBool(),
                             nullptr);
    m_entries[index] = entry;
}

Feed *EntryListModel::feed() const
{
    return m_feed;
}

void EntryListModel::setFeed(Feed *feed)
{
    m_feed = feed;
    Q_EMIT feedChanged(feed);
}
