/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "entry.h"
#include "episodemodel.h"
#include "feed.h"

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
    Feed *getFeed(const QString &feedurl) const;
    Entry *getEntry(const int feed_index, const int entry_index) const;
    Entry *getEntry(const Feed *feed, const int entry_index) const;
    Entry *getEntry(const EpisodeModel::Type type, const int entry_index) const;
    Q_INVOKABLE Entry *getEntry(const QString &id) const;
    int feedCount() const;
    int entryCount(const int feed_index) const;
    int entryCount(const Feed *feed) const;
    int entryCount(const EpisodeModel::Type type) const;
    int unreadEntryCount(const Feed *feed) const;
    int newEntryCount(const Feed *feed) const;
    Q_INVOKABLE void addFeed(const QString &url);
    void addFeed(const QString &url, const bool fetch);
    void addFeeds(const QStringList &urls);
    Q_INVOKABLE void removeFeed(Feed *feed);
    void removeFeed(const int index);

    // Q_INVOKABLE void addEntry(const QString &url);  // TODO: implement these methods
    // Q_INVOKABLE void removeEntry(const QString &url);
    // Q_INVOKABLE void removeEntry(const Feed* feed, const int &index);

    Entry *getQueueEntry(int index) const;
    int queueCount() const;
    QStringList queue() const;
    Q_INVOKABLE bool entryInQueue(const Entry *entry);
    Q_INVOKABLE bool entryInQueue(const QString &feedurl, const QString &id) const;
    Q_INVOKABLE void addToQueue(const Entry *entry);
    Q_INVOKABLE void addToQueue(const QString &feedurl, const QString &id);
    Q_INVOKABLE void moveQueueItem(const int from, const int to);
    Q_INVOKABLE void removeQueueItem(const int index);
    Q_INVOKABLE void removeQueueItem(const QString id);
    Q_INVOKABLE void removeQueueItem(Entry *entry);

    Q_INVOKABLE QString lastPlayingEntry();
    Q_INVOKABLE void setLastPlayingEntry(const QString &id);

    Q_INVOKABLE void importFeeds(const QString &path);
    Q_INVOKABLE void exportFeeds(const QString &path);

Q_SIGNALS:
    void feedAdded(const QString &url);
    void feedRemoved(const int &index);
    void entryAdded(const QString &feedurl, const QString &id);
    // void entryRemoved(const Feed*, const int &index); // TODO: implement this signal, is this needed?
    void feedEntriesUpdated(const QString &url);
    void queueEntryAdded(const int &index, const QString &id);
    void queueEntryRemoved(const int &index, const QString &id);
    void queueEntryMoved(const int &from, const int &to);

    void unreadEntryCountChanged(const QString &url);
    void newEntryCountChanged(const QString &url);
    void downloadCountChanged(const QString &url);

private:
    DataManager();
    void loadFeed(QString feedurl) const;
    void loadEntry(QString id) const;
    bool feedExists(const QString &url);
    void updateQueueListnrs() const;

    mutable QHash<QString, Feed *> m_feeds; // hash of pointers to all feeds in db, key = url (lazy loading)
    mutable QHash<QString, Entry *> m_entries; // hash of pointers to all entries in db, key = id (lazy loading)

    QStringList m_feedmap; // list of feedurls in the order that they should appear in feedlist
    QHash<QString, QStringList> m_entrymap; // list of entries (per feed; key = url) in the order that they should appear in entrylist
    QStringList m_queuemap; // list of entries/enclosures in the order that they should show up in queuelist
};
