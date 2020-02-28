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
#include <QVector>

#include <syndication.h>

#include "entryListModel.h"
#include "fetcher.h"

EntryListModel::EntryListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

QVariant EntryListModel::data(const QModelIndex &index, int role) const
{
    if (role == Bookmark)
        return QVariant(m_entries[index.row()].isBookmark());
    if (role == Read)
        return QVariant(m_entries[index.row()].isRead());
    if (role == Qt::DisplayRole)
        return m_entries[index.row()].title();
    return QVariant(index.row());
}
int EntryListModel::rowCount(const QModelIndex &index) const
{
    return m_entries.size();
}
QHash<int, QByteArray> EntryListModel::roleNames() const
{
    QHash<int, QByteArray> roleNames;
    roleNames[Qt::DisplayRole] = "display";
    roleNames[Bookmark] = "bookmark";
    roleNames[Read] = "read";
    return roleNames;
}
bool EntryListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role == Bookmark) {
        m_entries[index.row()].setBookmark(value.toBool());
    } else if (role == Read) {
        m_entries[index.row()].setRead(value.toBool());
    }
    emit dataChanged(index, index, QVector<int>({role}));
    return true;
}

void EntryListModel::fetch()
{
    connect(&Fetcher::instance(), &Fetcher::finished, this, [this]() {
        beginResetModel();
        QSqlQuery query(QSqlDatabase::database());
        query.prepare(QStringLiteral("SELECT id, title, content FROM Entries WHERE feed=:feed;"));
        query.bindValue(QStringLiteral(":feed"), m_feed);
        query.exec();
        while (query.next()) {
            m_entries.append(Entry(query.value(1).toString(), false, false));
        }
        endResetModel();
    });
    Fetcher::instance().fetch(m_feed);
}

QString EntryListModel::feed() const
{
    return m_feed;
}

void EntryListModel::setFeed(QString feed)
{
    m_feed = feed;
}
