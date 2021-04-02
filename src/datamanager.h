/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "feed.h"
#include "entry.h"

class DataManager : public QObject
{
    Q_OBJECT

public:
    static DataManager &instance()
    {
        static DataManager _instance;
        return _instance;
    }

    Feed* getFeed(int const index) const;
    Feed* getFeed(QString const feedurl) const;
    Entry* getEntry(int const feed_index, int const entry_index) const;
    Entry* getEntry(const Feed* feed, int const entry_index) const;
    Entry* getEntry(const QString id) const;
    int feedCount() const;
    int entryCount(const int feed_index) const;
    int entryCount(const Feed* feed) const;
    int unreadEntryCount(const Feed* feed) const;
    Q_INVOKABLE void addFeed(const QString &url);
    Q_INVOKABLE void removeFeed(const Feed* feed);
    Q_INVOKABLE void removeFeed(const int &index);

    //Q_INVOKABLE void addEntry(const QString &url);
    //Q_INVOKABLE void removeEntry(const QString &url);
    //Q_INVOKABLE void removeEntry(const Feed* feed, const int &index);

    Q_INVOKABLE void importFeeds(const QString &path);
    Q_INVOKABLE void exportFeeds(const QString &path);

Q_SIGNALS:
    void feedAdded(const QString &url);
    void feedRemoved(const int &index);
    void entryAdded(const QString &id);
    void entryRemoved(const Feed*, const int &index);
    void feedEntriesUpdated(const QString &url);

private:
    DataManager();
    void loadFeed(QString feedurl) const;
    void loadEntry(QString id) const;
    bool feedExists(const QString &url);

    QStringList m_feedmap;
    mutable QHash<QString, Feed*> m_feeds;
    QHash<QString, QStringList> m_entrymap;
    mutable QHash<QString, Entry*> m_entries;
};
