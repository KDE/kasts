/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QSqlTableModel>
#include <QUrl>

#include "feed.h"

class FeedsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        FeedRole = Qt::UserRole,
        UrlRole,
        TitleRole,
        UnreadCountRole,
    };
    Q_ENUM(Roles)

    explicit FeedsModel(QObject *parent = nullptr);
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent) const override;
};
