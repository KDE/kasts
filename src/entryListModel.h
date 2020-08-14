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

class EntryListModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(Feed *feed READ feed WRITE setFeed NOTIFY feedChanged)

public:
    explicit EntryListModel(QObject *parent = nullptr);
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent) const override;

    Feed *feed() const;

    void setFeed(Feed *feed);

Q_SIGNALS:
    void feedChanged(Feed *feed);

private:
    void loadEntry(int index) const;

    Feed *m_feed;
    mutable QHash<int, Entry *> m_entries;
};
