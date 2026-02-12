/**
 * SPDX-FileCopyrightText: 2021-2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "models/queuemodel.h"
#include "models/queuemodellogging.h"

#include <QThread>

#include <KFormat>

#include "audiomanager.h"
#include "datamanager.h"
#include "entry.h"
#include "settingsmanager.h"

QueueModel::QueueModel(QObject *parent)
    : AbstractEpisodeModel(parent)
{
    connect(&DataManager::instance(), &DataManager::queueEntryMoved, this, [this](int from, int to_orig) {
        int to = (from < to_orig) ? to_orig + 1 : to_orig;
        beginMoveRows(QModelIndex(), from, from, QModelIndex(), to);
        endMoveRows();
        qCDebug(kastsQueueModel) << "Moved entry" << from << "to" << to;
    });
    connect(&DataManager::instance(), &DataManager::queueEntryAdded, this, [this](int pos) {
        beginInsertRows(QModelIndex(), pos, pos);
        endInsertRows();
        Q_EMIT timeLeftChanged();
        qCDebug(kastsQueueModel) << "Added entry at pos" << pos;
    });
    connect(&DataManager::instance(), &DataManager::queueEntryRemoved, this, [this](int pos) {
        beginRemoveRows(QModelIndex(), pos, pos);
        endRemoveRows();
        Q_EMIT timeLeftChanged();
        qCDebug(kastsQueueModel) << "Removed entry at pos" << pos;
    });
    connect(&DataManager::instance(), &DataManager::queueSorted, this, [this]() {
        beginResetModel();
        endResetModel();
        qCDebug(kastsQueueModel) << "Queue was sorted";
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
    switch (role) {
    case AbstractEpisodeModel::Roles::EntryRole:
        return QVariant::fromValue(DataManager::instance().getQueueEntry(index.row()));
    case AbstractEpisodeModel::Roles::EntryuidRole:
        return QVariant::fromValue(DataManager::instance().queue()[index.row()]);
    default:
        return QVariant();
    }
}

int QueueModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    qCDebug(kastsQueueModel) << "queueCount is" << DataManager::instance().queue().count();
    return DataManager::instance().queue().count();
}

qint64 QueueModel::timeLeft() const
{
    int result = 0;
    const QList<qint64> queue = DataManager::instance().queue();
    for (const qint64 &item : queue) {
        Entry *entry = DataManager::instance().getEntry(item);
        if (entry->enclosure()) {
            result += entry->enclosure()->duration() * 1000 - entry->enclosure()->playPosition();
        }
    }
    qCDebug(kastsQueueModel) << "timeLeft is" << result;
    return result;
}

QString QueueModel::formattedTimeLeft() const
{
    qreal rate = 1.0;
    if (SettingsManager::self()->adjustTimeLeft()) {
        rate = AudioManager::instance().playbackRate();
        rate = (rate > 0.0) ? rate : 1.0;
    }
    static KFormat format;
    return format.formatDuration(timeLeft() / rate);
}

QString QueueModel::getSortName(AbstractEpisodeProxyModel::SortType type)
{
    return AbstractEpisodeProxyModel::getSortName(type);
}

QString QueueModel::getSortIconName(AbstractEpisodeProxyModel::SortType type)
{
    return AbstractEpisodeProxyModel::getSortIconName(type);
}

void QueueModel::updateInternalState()
{
    // nothing to do; DataManager already has the updated data.
}

// Hack to get a QItemSelection in QML
QItemSelection QueueModel::createSelection(int rowa, int rowb)
{
    return QItemSelection(index(rowa, 0), index(rowb, 0));
}
