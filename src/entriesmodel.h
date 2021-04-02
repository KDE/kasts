/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QObject>
#include <QString>

#include "entry.h"
#include "feed.h"

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

private:
    Feed *m_feed;
};
