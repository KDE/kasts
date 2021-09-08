/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "models/episodeproxymodel.h"

EpisodeProxyModel::EpisodeProxyModel()
    : QSortFilterProxyModel(nullptr)
{
    m_currentFilter = NoFilter;
    m_episodeModel = new EpisodeModel();
    setSourceModel(m_episodeModel);
}

EpisodeProxyModel::~EpisodeProxyModel()
{
    delete m_episodeModel;
}

bool EpisodeProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);

    switch (m_currentFilter) {
    case NoFilter:
        return true;
    case ReadFilter:
        return sourceModel()->data(index, EpisodeModel::Roles::ReadRole).value<bool>();
    case NotReadFilter:
        return !sourceModel()->data(index, EpisodeModel::Roles::ReadRole).value<bool>();
    case NewFilter:
        return sourceModel()->data(index, EpisodeModel::Roles::NewRole).value<bool>();
    case NotNewFilter:
        return !sourceModel()->data(index, EpisodeModel::Roles::NewRole).value<bool>();
    default:
        return true;
    }
}

EpisodeProxyModel::FilterType EpisodeProxyModel::filterType() const
{
    return m_currentFilter;
}

void EpisodeProxyModel::setFilterType(FilterType type)
{
    m_currentFilter = type;
    Q_EMIT filterTypeChanged(m_currentFilter);
}
