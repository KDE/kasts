/**
 * SPDX-FileCopyrightText: 2021-2022 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QHash>
#include <QObject>
#include <QString>
#include <QStringList>

#include "models/episodemodel.h"

class Entry;
class Feed;

class DataManager : public QObject
{
    Q_OBJECT

public:
    static DataManager &instance()
    {
        static DataManager _instance;
        return _instance;
    }

    Feed *getFeed(const int index) const;
    Q_INVOKABLE Feed *getFeed(const QString &feedurl) const;
    Entry *getEntry(const int feed_index, const int entry_index) const;
    Entry *getEntry(const Feed *feed, const int entry_index) const;
    Q_INVOKABLE Entry *getEntry(const QString &id) const;
    int feedCount() const;
    QStringList getIdList(const Feed *feed) const;
    int entryCount(const int feed_index) const;
    int entryCount(const Feed *feed) const;
    int newEntryCount(const Feed *feed) const;
    int favoriteEntryCount(const Feed *feed) const;
    Q_INVOKABLE void addFeed(const QString &url);
    void addFeed(const QString &url, const bool fetch);
    void addFeeds(const QStringList &urls);
    void addFeeds(const QStringList &urls, const bool fetch);
    Q_INVOKABLE void removeFeed(Feed *feed);
    void removeFeed(const int index);
    void removeFeeds(const QStringList &feedurls);
    Q_INVOKABLE void removeFeeds(const QVariantList feedsVariantList);
    void removeFeeds(const QList<Feed *> &feeds);

    Entry *getQueueEntry(int index) const;
    int queueCount() const;
    QStringList queue() const;
    bool entryInQueue(const Entry *entry);
    bool entryInQueue(const QString &id) const;
    Q_INVOKABLE void moveQueueItem(const int from, const int to);
    void addToQueue(const QString &id);
    void removeFromQueue(const QString &id);

    Q_INVOKABLE QString lastPlayingEntry();
    Q_INVOKABLE void setLastPlayingEntry(const QString &id);

    Q_INVOKABLE void deletePlayedEnclosures();

    Q_INVOKABLE void importFeeds(const QString &path);
    Q_INVOKABLE void exportFeeds(const QString &path);
    Q_INVOKABLE bool feedExists(const QString &url);

    Q_INVOKABLE void bulkMarkRead(bool state, QStringList list);
    Q_INVOKABLE void bulkMarkNew(bool state, QStringList list);
    Q_INVOKABLE void bulkMarkFavorite(bool state, QStringList list);
    Q_INVOKABLE void bulkQueueStatus(bool state, QStringList list);
    Q_INVOKABLE void bulkDownloadEnclosures(QStringList list);
    Q_INVOKABLE void bulkDeleteEnclosures(QStringList list);

    Q_INVOKABLE void bulkMarkReadByIndex(bool state, QModelIndexList list);
    Q_INVOKABLE void bulkMarkNewByIndex(bool state, QModelIndexList list);
    Q_INVOKABLE void bulkMarkFavoriteByIndex(bool state, QModelIndexList list);
    Q_INVOKABLE void bulkQueueStatusByIndex(bool state, QModelIndexList list);
    Q_INVOKABLE void bulkDownloadEnclosuresByIndex(QModelIndexList list);
    Q_INVOKABLE void bulkDeleteEnclosuresByIndex(QModelIndexList list);

Q_SIGNALS:
    void feedAdded(const QString &url);
    void feedRemoved(const int &index);
    void feedEntriesUpdated(const QString &url);
    void queueEntryAdded(const int &index, const QString &id);
    void queueEntryRemoved(const int &index, const QString &id);
    void queueEntryMoved(const int &from, const int &to);

    void unreadEntryCountChanged(const QString &url);
    void newEntryCountChanged(const QString &url);
    void favoriteEntryCountChanged(const QString &url);

    void bulkReadStatusActionFinished();
    void bulkNewStatusActionFinished();
    void bulkFavoriteStatusActionFinished();

    // this will relay the AudioManager::playbackRateChanged signal; this is
    // required to avoid a dependency loop on startup
    // TODO: find less hackish solution
    void playbackRateChanged();

private:
    DataManager();
    void loadFeed(const QString &feedurl) const;
    void loadEntry(QString id) const;
    void updateQueueListnrs() const;

    QString cleanUrl(const QString &url);

    QStringList getIdsFromModelIndexList(const QModelIndexList &list) const;

    mutable QHash<QString, Feed *> m_feeds; // hash of pointers to all feeds in db, key = url (lazy loading)
    mutable QHash<QString, Entry *> m_entries; // hash of pointers to all entries in db, key = id (lazy loading)

    QStringList m_feedmap; // list of feedurls in the order that they should appear in feedlist
    QHash<QString, QStringList> m_entrymap; // list of entries (per feed; key = url) in the order that they should appear in entrylist
    QStringList m_queuemap; // list of entries/enclosures in the order that they should show up in queuelist
};
