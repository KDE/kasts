/**
 * SPDX-FileCopyrightText: 2021-2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "models/episodeproxymodel.h"

EpisodeProxyModel::EpisodeProxyModel(QObject *parent)
    : AbstractEpisodeProxyModel(parent)
{
    m_episodeModel = &EpisodeModel::instance();
    setSourceModel(m_episodeModel);
}
