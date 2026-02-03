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

class AbstractEpisodeModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

public:
    enum Roles {
        TitleRole = Qt::DisplayRole,
        EntryuidRole = Qt::UserRole + 1,
        EntryRole,
        IdRole,
        ReadRole,
        NewRole,
        FavoriteRole,
        ContentRole,
        FeedNameRole,
        UpdatedRole,
    };
    Q_ENUM(Roles)

    explicit AbstractEpisodeModel(QObject *parent = nullptr);
    virtual QHash<int, QByteArray> roleNames() const override;

public Q_SLOTS:
    virtual void updateInternalState() = 0;
};
