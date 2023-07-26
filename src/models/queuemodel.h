/**
 * SPDX-FileCopyrightText: 2021-2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QModelIndex>
#include <QObject>
#include <QVariant>

#include "models/abstractepisodemodel.h"

class QueueModel : public AbstractEpisodeModel
{
    Q_OBJECT

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

Q_SIGNALS:
    void timeLeftChanged();

public Q_SLOTS:
    void updateInternalState() override;
};
