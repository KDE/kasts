/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <tobias.fella@kde.org>
 * SPDX-FileCopyrightText: 2021-2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "models/entriesproxymodel.h"

#include <QModelIndex>
#include <QString>

#include "models/entriesmodel.h"

EntriesProxyModel::EntriesProxyModel(const qint64 feeduid, QObject *parent)
    : AbstractEpisodeProxyModel(parent)
{
    m_entriesModel = new EntriesModel(feeduid, parent);
    setSourceModel(m_entriesModel);
}
