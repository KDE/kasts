/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "models/queuemodel.h"
#include "models/queuemodellogging.h"

#include <QString>
#include <QThread>

#include "audiomanager.h"
#include "datamanager.h"
#include "entry.h"
#include "models/episodemodel.h"
#include "settingsmanager.h"

QueueModel::QueueModel(QObject *parent)
    : QAbstractListModel(parent)
{
    connect(&DataManager::instance(), &DataManager::queueEntryMoved, this, [this](int from, int to_orig) {
        int to = (from < to_orig) ? to_orig + 1 : to_orig;
        beginMoveRows(QModelIndex(), from, from, QModelIndex(), to);
        endMoveRows();
        qCDebug(kastsQueueModel) << "Moved entry" << from << "to" << to;
    });
    connect(&DataManager::instance(), &DataManager::queueEntryAdded, this, [this](int pos, const QString &id) {
        Q_UNUSED(id)
        beginInsertRows(QModelIndex(), pos, pos);
        endInsertRows();
        Q_EMIT timeLeftChanged();
        qCDebug(kastsQueueModel) << "Added entry at pos" << pos;
    });
    connect(&DataManager::instance(), &DataManager::queueEntryRemoved, this, [this](int pos, const QString &id) {
        Q_UNUSED(id)
        beginRemoveRows(QModelIndex(), pos, pos);
        endRemoveRows();
        Q_EMIT timeLeftChanged();
        qCDebug(kastsQueueModel) << "Removed entry at pos" << pos;
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
    case EpisodeModel::Roles::EntryRole:
        return QVariant::fromValue(DataManager::instance().getQueueEntry(index.row()));
    case EpisodeModel::Roles::IdRole:
        return QVariant::fromValue(DataManager::instance().queue()[index.row()]);
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> QueueModel::roleNames() const
{
    return {
        {EpisodeModel::Roles::EntryRole, "entry"},
        {EpisodeModel::Roles::IdRole, "id"},
        {EpisodeModel::Roles::ReadRole, "read"},
        {EpisodeModel::Roles::NewRole, "new"},
    };
}

int QueueModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    qCDebug(kastsQueueModel) << "queueCount is" << DataManager::instance().queueCount();
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

// Hack to get a QItemSelection in QML
QItemSelection QueueModel::createSelection(int rowa, int rowb)
{
    return QItemSelection(index(rowa, 0), index(rowb, 0));
}
