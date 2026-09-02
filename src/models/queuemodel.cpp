/**
 * SPDX-FileCopyrightText: 2021-2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "models/queuemodel.h"
#include "models/queuemodellogging.h"

#include <QThread>

#include <KFormat>
#include <QAbstractItemModel>
#include <QHash>
#include <QSqlQuery>
#include <utility>

#include "audiomanager.h"
#include "database.h"
#include "datamanager.h"
#include "entry.h"
#include "objectslogging.h"
#include "settingsmanager.h"

QueueModel::QueueModel(QObject *parent)
    : AbstractEpisodeModel(QStringLiteral("SELECT feeduid, name, image, dirname FROM Feeds;"),
                           QStringLiteral("SELECT * FROM Queue JOIN Entries ON Entries.entryuid=Queue.entryuid ORDER BY listnr;"),
                           QStringLiteral("SELECT * FROM Queue JOIN Enclosures ON Enclosures.entryuid=Queue.entryuid;"),
                           parent)
{
    // Connect positionChanged to make sure that the remaining playing time in
    // the queue header is up-to-date
    connect(&DataManager::instance(), &DataManager::entryPlayPositionsChanged, this, &QueueModel::timeLeftChanged);

    qCDebug(kastsObjects) << "QueueModel object" << this << "constructed";
}

qint64 QueueModel::timeLeft() const
{
    qint64 unscaledTimeLeft = 0;

    QSqlQuery query;
    query.prepare(
        QStringLiteral("SELECT SUM(Enclosures.duration), SUM(Enclosures.playPosition) FROM Queue JOIN Enclosures ON Enclosures.entryuid = Queue.entryuid"));
    Database::instance().execute(query);
    if (query.next()) {
        qint64 total_duration = 1000 * query.value(QStringLiteral("SUM(Enclosures.duration)")).toLongLong();
        qint64 total_playedtime = query.value(QStringLiteral("SUM(Enclosures.playPosition)")).toLongLong();
        unscaledTimeLeft = total_duration - total_playedtime;
        qCDebug(kastsQueueModel) << "timeLeft is" << unscaledTimeLeft;
    }
    query.finish();

    qreal rate = 1.0;
    if (SettingsManager::self()->adjustTimeLeft()) {
        rate = AudioManager::instance().playbackRate();
        rate = (rate > 0.0) ? rate : 1.0;
    }
    return (unscaledTimeLeft / rate);
}

QString QueueModel::getSortName(AbstractEpisodeProxyModel::SortType type)
{
    return AbstractEpisodeProxyModel::getSortName(type);
}

QString QueueModel::getSortIconName(AbstractEpisodeProxyModel::SortType type)
{
    return AbstractEpisodeProxyModel::getSortIconName(type);
}

// Hack to get a QItemSelection in QML
QItemSelection QueueModel::createSelection(int rowa, int rowb)
{
    return QItemSelection(index(rowa, 0), index(rowb, 0));
}

void QueueModel::addToQueue(const QList<qint64> &entryuids)
{
    const qint64 beginQueueIndex = m_entryOrder.count();

    // Figure out how many entries actually have to be added (excluding the ones already in the queue)
    qint64 endQueueIndex = beginQueueIndex - 1;
    for (const qint64 entryuid : std::as_const(entryuids)) {
        if (!m_entryOrder.contains(entryuid)) {
            ++endQueueIndex;
        }
    }

    if (endQueueIndex == beginQueueIndex - 1) {
        // i.e. all entryuids are already in the queue
        return;
    }

    beginInsertRows(QModelIndex(), beginQueueIndex, endQueueIndex);

    qint64 currentQueueIndex = beginQueueIndex - 1; // Counter to be used inside the for loop to keep track of index
    Database::instance().transaction();
    QSqlQuery query;
    query.prepare(QStringLiteral("INSERT INTO Queue (listnr, entryuid, playing) VALUES (:listnr, :entryuid, :playing);"));
    for (const qint64 entryuid : std::as_const(entryuids)) {
        // If item is already in queue, then don't do anything
        if (!m_entryOrder.contains(entryuid)) {
            ++currentQueueIndex; // Increment index first because it's needed as listnr in the database

            // Add to Queue database
            query.bindValue(QStringLiteral(":listnr"), currentQueueIndex);
            query.bindValue(QStringLiteral(":entryuid"), entryuid);
            query.bindValue(QStringLiteral(":playing"), false);
            Database::instance().execute(query);
        }
    }
    Database::instance().commit();

    // now update the internal data structure as well
    updateInternalState();

    endInsertRows();

    Q_EMIT timeLeftChanged();
    qCDebug(kastsQueueModel) << "Added entry at from-to positions:" << beginQueueIndex << endQueueIndex;
    qCDebug(kastsQueueModel) << "m_entryOrder is now:" << m_entryOrder;
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
        qint64 index = m_entryOrder.indexOf(AudioManager::instance().entryuid()) + 1;
        Q_ASSERT(index > -1);

        while (index < m_entryOrder.count() && entryuids.contains(m_entryOrder[index])) {
            ++index;
        }

        // index should now contain the index of the next track that isn't going
        // to be removed
        if (index < m_entryOrder.count() && m_entryOrder[index] > -1) {
            AudioManager::instance().setEntryuid(m_entryOrder[index]);
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
        if (m_entryOrder.contains(entryuid)) {
            const int index = m_entryOrder.indexOf(entryuid);
            qCDebug(kastsQueueModel) << "m_entryOrder is now:" << m_entryOrder;
            qCDebug(kastsQueueModel) << "Queue index of item to be removed" << index;

            if (!useResetModel) {
                beginRemoveRows(QModelIndex(), index, index);
            }

            // Remove the item from the internal data structure
            m_entries.remove(entryuid);
            m_entryOrder.removeAt(index);

            // Then make sure that the database Queue table reflects these changes
            query.bindValue(QStringLiteral(":entryuid"), entryuid);
            Database::instance().execute(query);

            qCDebug(kastsQueueModel) << "Removed entry at index" << index;
            qCDebug(kastsQueueModel) << "queueCount is" << m_entryOrder.count();

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

    qCDebug(kastsQueueModel) << "m_entryOrder is now:" << m_entryOrder;
    Q_EMIT timeLeftChanged();
}

void QueueModel::moveQueueItem(const qint64 from, const qint64 to_orig)
{
    int to = (from < to_orig) ? to_orig + 1 : to_orig;
    beginMoveRows(QModelIndex(), from, from, QModelIndex(), to);
    // First move the items in the internal data structure
    m_entryOrder.move(from, to_orig);

    // Then make sure that the database Queue table reflects these changes
    updateQueueListnrs();

    endMoveRows();

    qCDebug(kastsQueueModel) << "Moved entry" << from << "to" << to_orig;

    // Send this signal mainly to inform AudioManager about a change in the queue
    Q_EMIT DataManager::instance().entryQueueStatusChanged(true, QList<qint64>());
}

QList<qint64> QueueModel::queue() const
{
    return m_entryOrder;
}

bool QueueModel::entryInQueue(const qint64 entryuid) const
{
    return m_entryOrder.contains(entryuid);
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
    for (int i = 0; i < m_entryOrder.length(); i++) {
        query.bindValue(QStringLiteral(":entryuid"), new_queue[i]);
        query.bindValue(QStringLiteral(":listnr"), i);
        Database::instance().execute(query);
    }
    Database::instance().commit();

    m_entryOrder.clear();
    m_entryOrder = new_queue;

    endResetModel();

    qCDebug(kastsQueueModel) << "Queue was sorted";
}

void QueueModel::updateQueueListnrs() const
{
    QSqlQuery query;
    Database::instance().transaction();
    query.prepare(QStringLiteral("UPDATE Queue SET listnr=:i WHERE entryuid=:entryuid;"));
    for (int i = 0; i < m_entryOrder.count(); i++) {
        query.bindValue(QStringLiteral(":i"), i);
        query.bindValue(QStringLiteral(":entryuid"), m_entryOrder[i]);
        Database::instance().execute(query);
    }
    Database::instance().commit();
}
