/**
 * SPDX-FileCopyrightText: 2021-2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QItemSelection>
#include <QModelIndex>
#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QVariant>

#include "models/abstractepisodemodel.h"
#include "models/abstractepisodeproxymodel.h"

class QueueModel : public AbstractEpisodeModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(int timeLeft READ timeLeft NOTIFY timeLeftChanged)
    Q_PROPERTY(QString formattedTimeLeft READ formattedTimeLeft NOTIFY timeLeftChanged)

public:
    static QueueModel &instance()
    {
        static QueueModel _instance;
        return _instance;
    }
    static QueueModel *create(QQmlEngine *engine, QJSEngine *)
    {
        engine->setObjectOwnership(&instance(), QQmlEngine::CppOwnership);
        return &instance();
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent) const override;

    qint64 timeLeft() const;
    QString formattedTimeLeft() const;

    Q_INVOKABLE static QString getSortName(AbstractEpisodeProxyModel::SortType type);
    Q_INVOKABLE static QString getSortIconName(AbstractEpisodeProxyModel::SortType type);

    Q_INVOKABLE QItemSelection createSelection(int rowa, int rowb);

    void addToQueue(const QList<qint64> &entryuids);
    void removeFromQueue(const QList<qint64> &entryuids);
    Q_INVOKABLE void moveQueueItem(const qint64 from, const qint64 to);

    // TODO: check if any of these can be made private after refactor
    Entry *getQueueEntry(int index) const;
    QList<qint64> queue() const;
    bool entryInQueue(const qint64 entryuid) const;
    Q_INVOKABLE void sortQueue(const AbstractEpisodeProxyModel::SortType sortType);

public Q_SLOTS:
    void updateInternalState() override;

Q_SIGNALS:
    void timeLeftChanged();

    void queueEntriesAdded(const qint64 beginPos, const qint64 endPos, const QList<qint64> &entryuids);
    void queueEntriesRemoved(const QList<qint64> &positions, const QList<qint64> &entryuids);
    void queueEntryMoved(const int &from, const int &to);
    void queueSorted();

private:
    explicit QueueModel(QObject *parent = nullptr);
    void updateQueueListnrs() const;

    QList<qint64> m_queue; // list of entries/enclosures in the order that they should show up in queuelist
};
