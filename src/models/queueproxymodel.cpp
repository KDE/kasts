/**
 * SPDX-FileCopyrightText: 2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "models/queueproxymodel.h"

QueueProxyModel::QueueProxyModel(QObject *parent)
    : AbstractEpisodeProxyModel(parent)
{
    m_queueModel = &QueueModel::instance();
    setSourceModel(m_queueModel);

    connect(m_queueModel, &QueueModel::timeLeftChanged, this, [this]() {
        Q_EMIT timeLeftChanged();
    });
}

int QueueProxyModel::timeLeft() const
{
    return m_queueModel->timeLeft();
}

QString QueueProxyModel::formattedTimeLeft() const
{
    return m_queueModel->formattedTimeLeft();
}
