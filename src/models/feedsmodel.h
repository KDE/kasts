/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <tobias.fella@kde.org>
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QQmlEngine>
#include <QSqlTableModel>
#include <QUrl>

#include "datatypes.h"

class FeedsModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

public:
    enum Roles {
        NameRole = Qt::DisplayRole,
        FeeduidRole = Qt::UserRole,
        UrlRole,
        ImageRole,
        LinkRole,
        DescriptionRole,
        AuthorsRole,
        RefreshingRole,
        IsSubscribedRole,
        SubscribedRole,
        LastUpdatedRole,
        EntryCountRole,
        UnreadCountRole,
        NewCountRole,
        FavoriteCountRole,
        FeedRole, // TODO: to be removed when refactor is done
    };
    Q_ENUM(Roles)

    explicit FeedsModel(QObject *parent = nullptr);
    ~FeedsModel();

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent) const override;

private:
    void triggerFeedUpdate(const qint64 feeduid);

    void updateFeed(const qint64 feeduid);

    QList<DataTypes::FeedDetails> m_feeds;
};
