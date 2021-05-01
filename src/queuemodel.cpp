/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include <QString>

#include "queuemodel.h"
#include "entry.h"
#include "datamanager.h"

QueueModel::QueueModel(QObject *parent)
    : QAbstractListModel(parent)
{
    connect(&DataManager::instance(), &DataManager::queueEntryMoved, this, [this](const int from, const int to_orig) {
        int to = (from < to_orig) ? to_orig + 1 : to_orig;
        beginMoveRows(QModelIndex(), from, from, QModelIndex(), to);
        endMoveRows();
        //qDebug() << "Moved entry" << from << "to" << to;
    });
    connect(&DataManager::instance(), &DataManager::queueEntryAdded, this, [this](const int pos, const QString &id) {
        Q_UNUSED(id)
        beginInsertRows(QModelIndex(), pos, pos);
        endInsertRows();
        //qDebug() << "Added entry at pos" << pos;
    });
    connect(&DataManager::instance(), &DataManager::queueEntryRemoved, this, [this](const int pos, const QString &id) {
        Q_UNUSED(id)
        beginRemoveRows(QModelIndex(), pos, pos);
        endRemoveRows();
        //qDebug() << "Removed entry at pos" << pos;
    });
}

QVariant QueueModel::data(const QModelIndex &index, int role) const
{
    if (role != 0)
        return QVariant();
    //qDebug() << "return entry" << DataManager::instance().getQueueEntry(index.row());
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
    //qDebug() << "queueCount is" << DataManager::instance().queueCount();
    return DataManager::instance().queueCount();
}
