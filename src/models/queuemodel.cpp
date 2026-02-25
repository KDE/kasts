/**
 * SPDX-FileCopyrightText: 2021-2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "models/queuemodel.h"
#include "models/queuemodellogging.h"

#include <QThread>

#include <KFormat>
#include <QSqlQuery>
#include <qabstractitemmodel.h>
#include <qhashfunctions.h>
#include <utility>

#include "audiomanager.h"
#include "database.h"
#include "datamanager.h"
#include "entry.h"
#include "objectslogging.h"
#include "settingsmanager.h"

QueueModel::QueueModel(QObject *parent)
    : AbstractEpisodeModel(parent)
{
    // Connect positionChanged to make sure that the remaining playing time in
    // the queue header is up-to-date
    connect(&DataManager::instance(), &DataManager::entryPlayPositionsChanged, this, &QueueModel::timeLeftChanged);

    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT entryuid FROM Queue ORDER BY listnr;"));
    Database::instance().execute(query);
    while (query.next()) {
        m_queue += query.value(QStringLiteral("entryuid")).toLongLong();
    }
    query.finish();
    qCDebug(kastsQueueModel) << "m_queue contains:" << m_queue;
    qCDebug(kastsObjects) << "QueueModel object" << this << "constructed";
}

QVariant QueueModel::data(const QModelIndex &index, int role) const
{
    switch (role) {
    case AbstractEpisodeModel::Roles::EntryRole:
        return QVariant::fromValue(getQueueEntry(index.row()));
    case AbstractEpisodeModel::Roles::EntryuidRole:
        return QVariant::fromValue(m_queue[index.row()]);
    default:
        return QVariant();
    }
}

int QueueModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    qCDebug(kastsQueueModel) << "queueCount is" << m_queue.count();
    return m_queue.count();
}

qint64 QueueModel::timeLeft() const
{
    qint64 result = 0;

    QSqlQuery query;
    query.prepare(
        QStringLiteral("SELECT SUM(Enclosures.duration), SUM(Enclosures.playPosition) FROM Queue JOIN Enclosures ON Enclosures.entryuid = Queue.entryuid"));
    Database::instance().execute(query);
    if (query.next()) {
        qint64 total_duration = 1000 * query.value(QStringLiteral("SUM(Enclosures.duration)")).toLongLong();
        qint64 total_playedtime = query.value(QStringLiteral("SUM(Enclosures.playPosition)")).toLongLong();
        result = total_duration - total_playedtime;
        qCDebug(kastsQueueModel) << "timeLeft is" << result;
    }
    query.finish();

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
    return format.formatDuration(timeLeft() / rate, KFormat::HideSeconds | KFormat::InitialDuration);
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

void QueueModel::addToQueue(const QList<qint64> &entryuids)
{
    const qint64 beginQueueIndex = m_queue.count();

    // Figure out how many entries actually have to be added (excluding the ones already in the queue)
    qint64 endQueueIndex = beginQueueIndex - 1;
    for (const qint64 entryuid : std::as_const(entryuids)) {
        if (m_queue.contains(entryuid)) {
            ++endQueueIndex;
        }
    }

    beginInsertRows(QModelIndex(), beginQueueIndex, endQueueIndex);

    qint64 currentQueueIndex = beginQueueIndex - 1; // Counter to be used inside the for loop to keep track of index
    Database::instance().transaction();
    QSqlQuery query;
    query.prepare(QStringLiteral("INSERT INTO Queue (listnr, entryuid, playing) VALUES (:listnr, :entryuid, :playing);"));
    for (const qint64 entryuid : std::as_const(entryuids)) {
        // If item is already in queue, then don't do anything
        if (!m_queue.contains(entryuid)) {
            ++currentQueueIndex; // Increment index first because it's needed as listnr in the database

            // Add to Queue database
            query.bindValue(QStringLiteral(":listnr"), currentQueueIndex);
            query.bindValue(QStringLiteral(":entryuid"), entryuid);
            query.bindValue(QStringLiteral(":playing"), false);
            Database::instance().execute(query);

            // Add to internal queuemap data structure
            m_queue += entryuid;
        }
    }
    Database::instance().commit();

    endInsertRows();

    Q_EMIT timeLeftChanged();
    qCDebug(kastsQueueModel) << "Added entry at from-to positions:" << beginQueueIndex << endQueueIndex;
    qCDebug(kastsQueueModel) << "m_queue is now:" << m_queue;
}

void QueueModel::removeFromQueue(const QList<qint64> &entryuids)
{
    // As long as the amount of items to be removed is low, we can use beginRemoveRows
    // Otherwise we use resetModel
    bool useResetModel = entryuids.count() > 50;

    // First we check whether the currently playing track needs to be removed
    // and, if so, skip to the next track on the queue that isn't going to be
    // removed either.
    if (entryuids.contains(AudioManager::instance().entryuid())) {
        qint64 index = m_queue.indexOf(AudioManager::instance().entryuid()) + 1;
        Q_ASSERT(index > -1);

        while (index < m_queue.count() && entryuids.contains(m_queue[index])) {
            ++index;
        }

        // index should now contain the index of the next track that isn't going
        // to be removed
        if (index < m_queue.count() && m_queue[index] > -1) {
            AudioManager::instance().setEntryuid(m_queue[index]);
        } else {
            AudioManager::instance().setEntryuid(0);
        }
    }

    if (useResetModel) {
        beginResetModel();
    }

    Database::instance().transaction();
    QSqlQuery query;
    query.prepare(QStringLiteral("DELETE FROM Queue WHERE entryuid=:entryuid;"));
    // doing a reverse loop here to avoid constantly resetting the currently playing track, which is expensive
    for (auto i = entryuids.rbegin(); i != entryuids.rend(); ++i) {
        qint64 entryuid = *i;
        // If item is not in queue then don't do anything
        if (m_queue.contains(entryuid)) {
            const int index = m_queue.indexOf(entryuid);
            qCDebug(kastsQueueModel) << "m_queue is now:" << m_queue;
            qCDebug(kastsQueueModel) << "Queue index of item to be removed" << index;

            if (!useResetModel) {
                beginRemoveRows(QModelIndex(), index, index);
            }

            // Remove the item from the internal data structure
            m_queue.removeAt(index);

            // Then make sure that the database Queue table reflects these changes
            query.bindValue(QStringLiteral(":entryuid"), entryuid);
            Database::instance().execute(query);

            qCDebug(kastsQueueModel) << "Removed entry at index" << index;
            qCDebug(kastsQueueModel) << "queueCount is" << m_queue.count();

            if (!useResetModel) {
                endRemoveRows();
            }
        }
    }
    Database::instance().commit();

    updateQueueListnrs();

    if (useResetModel) {
        endResetModel();
    }

    qCDebug(kastsQueueModel) << "m_queue is now:" << m_queue;
    Q_EMIT timeLeftChanged();
}

void QueueModel::moveQueueItem(const qint64 from, const qint64 to_orig)
{
    int to = (from < to_orig) ? to_orig + 1 : to_orig;
    beginMoveRows(QModelIndex(), from, from, QModelIndex(), to);
    // First move the items in the internal data structure
    m_queue.move(from, to_orig);

    // Then make sure that the database Queue table reflects these changes
    updateQueueListnrs();

    endMoveRows();

    qCDebug(kastsQueueModel) << "Moved entry" << from << "to" << to_orig;

    // Send this signal mainly to inform AudioManager about a change in the queue
    Q_EMIT DataManager::instance().entryQueueStatusChanged(true, QList<qint64>());
}

Entry *QueueModel::getQueueEntry(int index) const
{
    return DataManager::instance().getEntry(m_queue[index]);
}

QList<qint64> QueueModel::queue() const
{
    return m_queue;
}

bool QueueModel::entryInQueue(const qint64 entryuid) const
{
    return m_queue.contains(entryuid);
}

void QueueModel::sortQueue(const AbstractEpisodeProxyModel::SortType sortType)
{
    beginResetModel();

    QString columnName;
    QString order;

    switch (sortType) {
    case AbstractEpisodeProxyModel::SortType::DateAscending:
        order = QStringLiteral("ASC");
        columnName = QStringLiteral("updated");
        break;
    case AbstractEpisodeProxyModel::SortType::DateDescending:
        order = QStringLiteral("DESC");
        columnName = QStringLiteral("updated");
        break;
    }

    QList<qint64> new_queue;

    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT * FROM Queue INNER JOIN Entries ON Queue.entryuid = Entries.entryuid ORDER BY %1 %2;").arg(columnName, order));
    Database::instance().execute(query);

    while (query.next()) {
        qCDebug(kastsQueueModel) << "new queue order:" << query.value(QStringLiteral("entryuid")).toLongLong();
        new_queue += query.value(QStringLiteral("entryuid")).toLongLong();
    }

    Database::instance().transaction();
    query.prepare(QStringLiteral("UPDATE Queue SET listnr=:listnr WHERE entryuid=:entryuid;"));
    for (int i = 0; i < m_queue.length(); i++) {
        query.bindValue(QStringLiteral(":entryuid"), new_queue[i]);
        query.bindValue(QStringLiteral(":listnr"), i);
        Database::instance().execute(query);
    }
    Database::instance().commit();

    m_queue.clear();
    m_queue = new_queue;

    endResetModel();

    qCDebug(kastsQueueModel) << "Queue was sorted";
}

void QueueModel::updateQueueListnrs() const
{
    QSqlQuery query;
    Database::instance().transaction();
    query.prepare(QStringLiteral("UPDATE Queue SET listnr=:i WHERE entryuid=:entryuid;"));
    for (int i = 0; i < m_queue.count(); i++) {
        query.bindValue(QStringLiteral(":i"), i);
        query.bindValue(QStringLiteral(":entryuid"), m_queue[i]);
        Database::instance().execute(query);
    }
    Database::instance().commit();
}
