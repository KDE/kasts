/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include <QString>
#include <QThread>

#include "audiomanager.h"
#include "datamanager.h"
#include "entry.h"
#include "queuemodel.h"

QueueModel::QueueModel(QObject *parent)
    : QAbstractListModel(parent)
{
    connect(&DataManager::instance(), &DataManager::queueEntryMoved, this, [this](int from, int to_orig) {
        int to = (from < to_orig) ? to_orig + 1 : to_orig;
        beginMoveRows(QModelIndex(), from, from, QModelIndex(), to);
        endMoveRows();
        // qDebug() << "Moved entry" << from << "to" << to;
    });
    connect(&DataManager::instance(), &DataManager::queueEntryAdded, this, [this](int pos, const QString &id) {
        Q_UNUSED(id)
        beginInsertRows(QModelIndex(), pos, pos);
        endInsertRows();
        Q_EMIT timeLeftChanged();
        // qDebug() << "Added entry at pos" << pos;
    });
    connect(&DataManager::instance(), &DataManager::queueEntryRemoved, this, [this](int pos, const QString &id) {
        Q_UNUSED(id)
        beginRemoveRows(QModelIndex(), pos, pos);
        endRemoveRows();
        Q_EMIT timeLeftChanged();
        // qDebug() << "Removed entry at pos" << pos;
    });
    // Connect positionChanged to make sure that the remaining playing time in
    // the queue header is up-to-date
    connect(&AudioManager::instance(), &AudioManager::positionChanged, this, [this](qint64 position) {
        Q_UNUSED(position)
        Q_EMIT timeLeftChanged();
    });
}

QVariant QueueModel::data(const QModelIndex &index, int role) const
{
    if (role != 0)
        return QVariant();
    // qDebug() << "return entry" << DataManager::instance().getQueueEntry(index.row());
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
    // qDebug() << "queueCount is" << DataManager::instance().queueCount();
    return DataManager::instance().queueCount();
}

int QueueModel::timeLeft() const
{
    int result = 0;
    QStringList queue = DataManager::instance().queue();
    for (QString item : queue) {
        Entry *entry = DataManager::instance().getEntry(item);
        if (entry->enclosure()) {
            result += entry->enclosure()->duration() * 1000 - entry->enclosure()->playPosition();
        }
    }
    // qDebug() << "timeLeft is" << result;
    return result;
}

QString QueueModel::formattedTimeLeft() const
{
    static KFormat format;
    return format.formatDuration(timeLeft());
}
