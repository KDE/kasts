/**
 * SPDX-FileCopyrightText: 2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QObject>
#include <QString>

#include "models/abstractepisodeproxymodel.h"
#include "models/queuemodel.h"

class QueueProxyModel : public AbstractEpisodeProxyModel
{
    Q_OBJECT

    Q_PROPERTY(int timeLeft READ timeLeft NOTIFY timeLeftChanged)
    Q_PROPERTY(QString formattedTimeLeft READ formattedTimeLeft NOTIFY timeLeftChanged)

public:
    explicit QueueProxyModel(QObject *parent = nullptr);

    int timeLeft() const;
    QString formattedTimeLeft() const;

Q_SIGNALS:
    void timeLeftChanged();

private:
    QueueModel *m_queueModel;
};
