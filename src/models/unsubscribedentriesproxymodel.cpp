/**
 * SPDX-FileCopyrightText: 2026 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "models/unsubscribedentriesproxymodel.h"

#include <QString>

#include "models/abstractepisodeproxymodel.h"
#include "models/unsubscribedentriesmodel.h"
#include "objectslogging.h"

UnsubscribedEntriesProxyModel::UnsubscribedEntriesProxyModel(const QString &url, QObject *parent)
    : AbstractEpisodeProxyModel(parent)
{
    m_entriesModel = new UnsubscribedEntriesModel(url, this);
    setSourceModel(m_entriesModel);

    qCDebug(kastsObjects) << "UnsubscribedEntriesProxyModel constructed" << url;
}

UnsubscribedEntriesProxyModel::~UnsubscribedEntriesProxyModel()
{
    qCDebug(kastsObjects) << "UnsubscribedEntriesProxyModel destructed" << m_entriesModel;
}
