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

class AbstractEpisodeModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        TitleRole = Qt::DisplayRole,
        EntryRole = Qt::UserRole + 1,
        IdRole,
        ReadRole,
        NewRole,
        ContentRole,
        FeedNameRole,
    };
    Q_ENUM(Roles)

    explicit AbstractEpisodeModel(QObject *parent = nullptr);
    virtual QHash<int, QByteArray> roleNames() const override;

public Q_SLOTS:
    virtual void updateInternalState() = 0;
};
