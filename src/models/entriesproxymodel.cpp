/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
 * SPDX-FileCopyrightText: 2021-2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "models/entriesproxymodel.h"

#include <QModelIndex>
#include <QString>

#include "feed.h"
#include "models/entriesmodel.h"

EntriesProxyModel::EntriesProxyModel(Feed *feed)
    : AbstractEpisodeProxyModel(feed)
{
    m_entriesModel = new EntriesModel(feed);
    setSourceModel(m_entriesModel);
}

Feed *EntriesProxyModel::feed() const
{
    return m_entriesModel->feed();
}
