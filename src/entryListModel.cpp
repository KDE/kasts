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

#include <QVector>
#include <QDateTime>

#include "entryListModel.h"
#include "fetcher.h"
#include "database.h"

EntryListModel::EntryListModel(QObject *parent)
    : QSqlTableModel(parent)
{
    setTable("entries");
    setSort(Updated, Qt::DescendingOrder);
    setEditStrategy(OnFieldChange);
    select();
}

QVariant EntryListModel::data(const QModelIndex &index, int role) const
{
    if(role == Updated || role == Created) {
        QDateTime updated;
        updated.setSecsSinceEpoch(QSqlQueryModel::data(createIndex(index.row(), role), 0).toInt());
        return updated;
    }
        return QSqlQueryModel::data(createIndex(index.row(), role), 0);
}

QHash<int, QByteArray> EntryListModel::roleNames() const
{
    QHash<int, QByteArray> roleNames;
    roleNames[Feed] = "feed";
    roleNames[Id] = "id";
    roleNames[Title] = "title";
    roleNames[Content] = "content";
    roleNames[Created] = "created";
    roleNames[Updated] = "updated";
    return roleNames;
}

void EntryListModel::setFeed(QString url)
{
    m_feed = url;
    setFilter(QStringLiteral("feed ='%1'").arg(url));
    select();
}

QString EntryListModel::feed() const
{
    return m_feed;
}

void EntryListModel::fetch()
{
    Fetcher::instance().fetch(m_feed);
}
