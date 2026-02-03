/**
 * SPDX-FileCopyrightText: 2021-2022 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QHash>
#include <QObject>
#include <QPointer>
#include <QQmlEngine>
#include <QString>
#include <QStringList>
#include <QtQml/qqmlregistration.h>

#include "entry.h"
#include "feed.h"
#include "models/abstractepisodeproxymodel.h"

class DataManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
public:
    static DataManager &instance()
    {
        static DataManager _instance;
        return _instance;
    }
    static DataManager *create(QQmlEngine *engine, QJSEngine *)
    {
        engine->setObjectOwnership(&instance(), QQmlEngine::CppOwnership);
        return &instance();
    }

    Q_INVOKABLE Feed *getFeed(const qint64 feeduid) const;
    Q_INVOKABLE Entry *getEntry(const qint64 entryuid) const;

    // TODO: to be removed
    Q_INVOKABLE Feed *getFeed(const QString &feedurl) const;
    Q_INVOKABLE Entry *getEntry(const QString &id) const;

    int feedCount() const;
    Q_INVOKABLE void addFeed(const QString &url);
    void addFeeds(const QStringList &urls, const bool fetch);
    Q_INVOKABLE void removeFeed(Feed *feed);
    void removeFeeds(const QStringList &feedurls);
    Q_INVOKABLE void removeFeeds(const QVariantList feedsVariantList);
    void removeFeeds(const QList<Feed *> &feeds);

    Entry *getQueueEntry(int index) const;
    QList<qint64> queue() const;
    bool entryInQueue(const qint64 entryuid);
    Q_INVOKABLE void moveQueueItem(const int from, const int to);
    void addToQueue(const qint64 entryuid);
    void removeFromQueue(const qint64 entryuid);
    Q_INVOKABLE void sortQueue(AbstractEpisodeProxyModel::SortType sortType);

    Q_INVOKABLE qint64 lastPlayingEntry();
    Q_INVOKABLE void setLastPlayingEntry(const qint64 entryuid);

    Q_INVOKABLE void deletePlayedEnclosures();

    Q_INVOKABLE void importFeeds(const QString &path);
    Q_INVOKABLE void exportFeeds(const QString &path);
    Q_INVOKABLE bool feedExists(const QString &url);

    Q_INVOKABLE void bulkMarkRead(bool state, const QList<qint64> &list);
    Q_INVOKABLE void bulkMarkNew(bool state, const QList<qint64> &list);
    Q_INVOKABLE void bulkMarkFavorite(bool state, const QList<qint64> &list);
    Q_INVOKABLE void bulkQueueStatus(bool state, const QList<qint64> &list);
    Q_INVOKABLE void bulkDownloadEnclosures(const QList<qint64> &list);
    Q_INVOKABLE void bulkDeleteEnclosures(const QList<qint64> &list);

    Q_INVOKABLE void bulkMarkReadByIndex(bool state, const QModelIndexList &list);
    Q_INVOKABLE void bulkMarkNewByIndex(bool state, const QModelIndexList &list);
    Q_INVOKABLE void bulkMarkFavoriteByIndex(bool state, const QModelIndexList &list);
    Q_INVOKABLE void bulkQueueStatusByIndex(bool state, const QModelIndexList &list);
    Q_INVOKABLE void bulkDownloadEnclosuresByIndex(const QModelIndexList &list);
    Q_INVOKABLE void bulkDeleteEnclosuresByIndex(const QModelIndexList &list);

Q_SIGNALS:
    void feedAdded(const qint64 feeduid);
    void feedRemoved(const qint64 feeduid);
    void feedEntriesUpdated(const qint64 feeduid);
    void queueEntryAdded(const int &index, const qint64 entryuid);
    void queueEntryRemoved(const int &index, const qint64 entryuid);
    void queueEntryMoved(const int &from, const int &to);
    void queueSorted();

    void unreadEntryCountChanged(const qint64 feeduid);
    void newEntryCountChanged(const qint64 feeduid);
    void favoriteEntryCountChanged(const qint64 feeduid);

    void bulkReadStatusActionFinished();
    void bulkNewStatusActionFinished();
    void bulkFavoriteStatusActionFinished();

    // this will relay the AudioManager::playbackRateChanged signal; this is
    // required to avoid a dependency loop on startup
    // TODO: find less hackish solution
    void playbackRateChanged();
    void playPositionChanged(const qint64 entryuid, const qint64 position);

private:
    DataManager();
    void loadFeed(const qint64 feeduid) const;
    void loadEntry(const qint64 entryuid) const;
    void updateQueueListnrs() const;

    // TODO: probably needs to be removed after refactor
    qint64 getFeeduidFromUrl(const QString &url) const;
    qint64 getEntryuidFromId(const QString &id) const;

    QString cleanUrl(const QString &url);

    QList<qint64> getEntryuidsFromModelIndexList(const QModelIndexList &list) const;

    mutable QHash<qint64, QPointer<Feed>> m_feeds; // hash of pointers to all feeds in db, key = feeduid (lazy loading)
    mutable QHash<qint64, QPointer<Entry>> m_entries; // hash of pointers to all entries in db, key = entryuid (lazy loading)

    QList<qint64> m_queuemap; // list of entries/enclosures in the order that they should show up in queuelist
};
