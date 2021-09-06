/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QItemSelection>
#include <QObject>
#include <QVariant>

class Feed;

class EntriesModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(Feed *feed READ feed CONSTANT)

public:
    explicit EntriesModel(Feed *feed);
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent) const override;

    Feed *feed() const;

    Q_INVOKABLE QItemSelection createSelection(int rowa, int rowb);

private:
    Feed *m_feed;
};
