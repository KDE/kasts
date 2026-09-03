/**
 * SPDX-FileCopyrightText: 2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QAbstractListModel>
#include <QByteArray>
#include <QHash>
#include <QObject>
#include <QQmlEngine>

#include "datatypes.h"

class AbstractEpisodeModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

public:
    enum Roles {
        TitleRole = Qt::DisplayRole,
        EntryuidRole = Qt::UserRole + 1,
        IdRole,
        QueueStatusRole,
        ReadRole,
        NewRole,
        FavoriteRole,
        RemovedRole,
        ContentRole,
        CreatedRole,
        UpdatedRole,
        LinkRole,
        ImageRole,
        HasEnclosureRole,
        PlayPositionRole,
        DurationRole,
        SizeRole,
        DownloadedRole,
        DownloadedOrderRole,
        FeeduidRole,
        FeedNameRole,
        FeedImageRole,
    };
    Q_ENUM(Roles)

    explicit AbstractEpisodeModel(const QString &feedQuery, const QString &entryQuery, const QString &enclosureQuery, QObject *parent = nullptr);
    ~AbstractEpisodeModel();
    virtual QHash<int, QByteArray> roleNames() const override;
    virtual QVariant data(const QModelIndex &index, int role = Qt::UserRole) const override;
    virtual int rowCount(const QModelIndex &parent) const override;

protected:
    void updateInternalState();
    void updateEntries(const QList<qint64> &entryuids);
    void updateFeeds(const QList<qint64> &feeduids);

    QString m_feedQuery, m_entryQuery, m_enclosureQuery;
    QList<qint64> m_entryOrder;
    QHash<qint64, DataTypes::EntryDetails> m_entries;
    QHash<qint64, DataTypes::FeedDetails> m_feeds;
};
