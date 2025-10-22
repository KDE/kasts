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
#include <qnamespace.h>

class FeedsModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

public:
    enum Roles {
        UrlRole = Qt::DisplayRole,
        FeedIdRole = Qt::UserRole + 1,
        FeedRole,
        NameRole,
        TitleRole,
        UnreadCountRole,
        NewCountRole,
        FavoriteCountRole,
    };
    Q_ENUM(Roles)

    explicit FeedsModel(QObject *parent = nullptr);
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent) const override;

private:
    void triggerFeedUpdate(const int &feedid);
};
