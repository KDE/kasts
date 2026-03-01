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
#include <qtmetamacros.h>

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

    // routines for fuzzy matching of feeds and entries/enclosures to uids
    // returns a list because there can be more than one result per input value
    QList<QList<qint64>> findEntryuids(const QStringList &ids, const QStringList &enclosureUrls = QStringList()) const;

    Q_INVOKABLE void addFeed(const QString &url);
    void addFeeds(const QStringList &urls, const bool fetch);
    Q_INVOKABLE void removeFeed(Feed *feed);
    void removeFeeds(const QStringList &feedurls);
    Q_INVOKABLE void removeFeeds(const QVariantList feedsVariantList);
    void removeFeeds(const QList<Feed *> &feeds);

    // TODO: remove afer queuemodel refactor
    Q_INVOKABLE qint64 lastPlayingEntry();
    Q_INVOKABLE void setLastPlayingEntry(const qint64 entryuid);

    Q_INVOKABLE void deletePlayedEnclosures();

    Q_INVOKABLE void importFeeds(const QString &path);
    Q_INVOKABLE void exportFeeds(const QString &path);
    Q_INVOKABLE bool feedExists(const QString &url);

    Q_INVOKABLE void bulkMarkRead(bool state, const QList<qint64> &entryuids) const;
    Q_INVOKABLE void bulkMarkNew(bool state, const QList<qint64> &entryuids) const;
    Q_INVOKABLE void bulkMarkFavorite(bool state, const QList<qint64> &entryuids) const;
    Q_INVOKABLE void bulkQueueStatus(bool state, const QList<qint64> &entryuids) const;
    Q_INVOKABLE void bulkDownloadEnclosures(const QList<qint64> &entryuids) const;
    Q_INVOKABLE void bulkDeleteEnclosures(const QList<qint64> &entryuids) const;
    Q_INVOKABLE void bulkSetPlayPositions(const QList<qint64> &playPositions, const QList<qint64> &entryuids) const;
    Q_INVOKABLE void bulkSetEnclosureDurations(const QList<qint64> &durations, const QList<qint64> &entryuids) const;
    Q_INVOKABLE void bulkSetEnclosureSizes(const QList<qint64> &sizes, const QList<qint64> &entryuids) const;
    Q_INVOKABLE void bulkSetEnclosureStatuses(const QList<Enclosure::Status> &statuses, const QList<qint64> &entryuids) const;

    Q_INVOKABLE void bulkMarkReadByIndex(bool state, const QModelIndexList &list) const;
    Q_INVOKABLE void bulkMarkNewByIndex(bool state, const QModelIndexList &list) const;
    Q_INVOKABLE void bulkMarkFavoriteByIndex(bool state, const QModelIndexList &list) const;
    Q_INVOKABLE void bulkQueueStatusByIndex(bool state, const QModelIndexList &list) const;
    Q_INVOKABLE void bulkDownloadEnclosuresByIndex(const QModelIndexList &list) const;
    Q_INVOKABLE void bulkDeleteEnclosuresByIndex(const QModelIndexList &list) const;

Q_SIGNALS:
    void feedAdded(const qint64 feeduid);
    void feedRemoved(const qint64 feeduid);
    void feedEntriesUpdated(const qint64 feeduid);

    void entryReadStatusChanged(bool state, const QList<qint64> &entryuids) const;
    void entryNewStatusChanged(bool state, const QList<qint64> &entryuids) const;
    void entryFavoriteStatusChanged(bool state, const QList<qint64> &entryuids) const;
    void entryQueueStatusChanged(bool state, const QList<qint64> &entryuids) const;
    void entryPlayPositionsChanged(const QList<qint64> &positions, const QList<qint64> &entryuids) const;
    void enclosureDurationsChanged(const QList<qint64> &durations, const QList<qint64> &entryuids) const;
    void enclosureSizesChanged(const QList<qint64> &sizes, const QList<qint64> &entryuids) const;
    void enclosureStatusesChanged(const QList<Enclosure::Status> &statuses, const QList<qint64> &entryuids) const;

    void unreadEntryCountChanged(const qint64 feeduid) const;
    void newEntryCountChanged(const qint64 feeduid) const;
    void favoriteEntryCountChanged(const qint64 feeduid) const;

private:
    DataManager();
    void loadFeed(const qint64 feeduid) const;
    void loadEntry(const qint64 entryuid) const;

    // TODO: probably needs to be updated after refactor
    qint64 getFeeduidFromUrl(const QString &url) const;
    qint64 getEntryuidFromId(const QString &id) const;

    QString cleanUrl(const QString &url);

    QList<qint64> getEntryuidsFromModelIndexList(const QModelIndexList &list) const;

    mutable QHash<qint64, QPointer<Feed>> m_feeds; // hash of pointers to all feeds in db, key = feeduid (lazy loading)
    mutable QHash<qint64, QPointer<Entry>> m_entries; // hash of pointers to all entries in db, key = entryuid (lazy loading)
};
