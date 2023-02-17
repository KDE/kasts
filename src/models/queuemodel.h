/**
 * SPDX-FileCopyrightText: 2021-2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QHash>
#include <QItemSelection>
#include <QModelIndex>
#include <QObject>
#include <QString>
#include <QVariant>

#include "models/abstractepisodemodel.h"

class QueueModel : public AbstractEpisodeModel
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
    explicit QueueModel(QObject *parent = nullptr);
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent) const override;

    int timeLeft() const;
    QString formattedTimeLeft() const;

    Q_INVOKABLE QItemSelection createSelection(int rowa, int rowb);

public Q_SLOTS:
    void updateInternalState() override;

Q_SIGNALS:
    void timeLeftChanged();
};
