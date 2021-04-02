/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QObject>
#include <QString>

#include "entry.h"

class QueueModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit QueueModel(QObject* = nullptr);
    ~QueueModel() override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent) const override;

    Q_INVOKABLE bool move(int from, int to);
    Q_INVOKABLE bool moveRows(const QModelIndex &sourceParent, int sourceRow, int count, const QModelIndex &destinationParent, int destinationChild) override;
    Q_INVOKABLE void addEntry(QString feedurl, QString id);

private:
    void updateQueue();

    mutable QVector<Entry *> m_entries;
};
