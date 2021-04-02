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

private:
    DataManager();
    void loadFeed(QString feedurl) const;

    QVector<QString> m_feedmap;
    mutable QHash<QString, Feed*> m_feeds;
    QHash<QString, QVector<QString> > m_entrymap;
    mutable QHash<QString, Entry*> m_entries;
};
