/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QSqlTableModel>
#include <QUrl>

#include "feed.h"

class FeedListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit FeedListModel(QObject *parent = nullptr);
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent) const override;
    Q_INVOKABLE void removeFeed(int index);
    Q_INVOKABLE void refreshAll();

private:
    void loadFeed(int index) const;

    mutable QHash<int, Feed *> m_feeds;
};
