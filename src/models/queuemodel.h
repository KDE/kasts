/**
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

#include <KFormat>

#include "audiomanager.h"

class QueueModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(int timeLeft READ timeLeft NOTIFY timeLeftChanged)
    Q_PROPERTY(QString formattedTimeLeft READ formattedTimeLeft NOTIFY timeLeftChanged)

public:
    static QueueModel &instance()
    {
        static QueueModel _instance;
        return _instance;
    }
    explicit QueueModel(QObject * = nullptr);
    //~QueueModel() override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent) const override;
    int timeLeft() const;
    QString formattedTimeLeft() const;

    Q_INVOKABLE QItemSelection createSelection(int rowa, int rowb);

Q_SIGNALS:
    void timeLeftChanged();
};
