/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include <QAbstractListModel>
#include <QVariant>
#include <QVector>
#include <QSqlQuery>
#include <QFile>

#include "database.h"
#include "datamanager.h"
#include "queuemodel.h"
#include "fetcher.h"

QueueModel::QueueModel(QObject *parent)
    : QAbstractListModel(parent)
{
    connect(&DataManager::instance(), &DataManager::queueEntryMoved, this, [this](const int &from, const int &to_orig) {
        int to = (from < to_orig) ? to_orig + 1 : to_orig;
        beginMoveRows(QModelIndex(), from, from, QModelIndex(), to);
        endMoveRows();
        qDebug() << "Moved entry" << from << "to" << to;
    });
    connect(&DataManager::instance(), &DataManager::queueEntryAdded, this, [this](const int &pos, const QString &id) {
        Q_UNUSED(id)
        beginInsertRows(QModelIndex(), pos, pos);
        endInsertRows();
        qDebug() << "Added entry at pos" << pos;
    });
    connect(&DataManager::instance(), &DataManager::queueEntryRemoved, this, [this](const int &pos, const QString &id) {
        Q_UNUSED(id)
        beginRemoveRows(QModelIndex(), pos, pos);
        endRemoveRows();
        qDebug() << "Removed entry at pos" << pos;
    });
}

QVariant QueueModel::data(const QModelIndex &index, int role) const
{
    if (role != 0)
        return QVariant();
    qDebug() << "return entry" << DataManager::instance().getQueueEntry(index.row());
    return QVariant::fromValue(DataManager::instance().getQueueEntry(index.row()));
}

QHash<int, QByteArray> QueueModel::roleNames() const
{
    QHash<int, QByteArray> roleNames;
    roleNames[0] = "entry";
    return roleNames;
}

int QueueModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    qDebug() << "queueCount is" << DataManager::instance().queueCount();
    return DataManager::instance().queueCount();
}

/*void QueueModel::addEntry(QString feedurl, QString id) {
    qDebug() << feedurl << id;

    QSqlQuery feedQuery;
    feedQuery.prepare(QStringLiteral("SELECT * FROM Feeds WHERE url=:feedurl"));
    feedQuery.bindValue(QStringLiteral(":feedurl"), feedurl);
    Database::instance().execute(feedQuery);
    if (!feedQuery.next()) {
        qWarning() << "Feed not found:" << feedurl;
        // TODO: remove enclosures belonging to non-existent feed
        return;
    }
    int feed_index = feedQuery.value(QStringLiteral("rowid")).toInt() - 1;
    qDebug() << feed_index << feedurl;
    Feed* feed = new Feed(feed_index);

    // Find query index
    QSqlQuery entryQuery;
    entryQuery.prepare(QStringLiteral("SELECT rowid FROM Entries WHERE feed=:feedurl ORDER BY updated;"));
    entryQuery.bindValue(QStringLiteral(":feedurl"), feedurl);
    Database::instance().execute(entryQuery);
    int counter = -1;
    int entry_index = -1;
    while (entryQuery.next()) {
        counter++;
        QString idquery = entryQuery.value(QStringLiteral("id")).toString();
        if (idquery == id) entry_index = counter;
    }
    if (entry_index == -1) return;

    beginInsertRows(QModelIndex(), rowCount(QModelIndex()) - 1, rowCount(QModelIndex()) - 1);
    m_entries.append(new Entry(feed, entry_index));
    endInsertRows();
    return;
}
*/
