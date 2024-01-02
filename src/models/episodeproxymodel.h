/**
 * SPDX-FileCopyrightText: 2021-2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QObject>
#include <QQmlEngine>

#include "models/abstractepisodeproxymodel.h"
#include "models/episodemodel.h"

class EpisodeProxyModel : public AbstractEpisodeProxyModel
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit EpisodeProxyModel(QObject *parent = nullptr);

private:
    EpisodeModel *m_episodeModel;
};
