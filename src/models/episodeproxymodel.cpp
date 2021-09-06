/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "models/episodeproxymodel.h"

#include <KLocalizedString>

#include "datamanager.h"
#include "entry.h"

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
    if (type != m_currentFilter) {
        disconnect(&DataManager::instance(), &DataManager::bulkReadStatusActionFinished, this, nullptr);
        disconnect(&DataManager::instance(), &DataManager::bulkNewStatusActionFinished, this, nullptr);

        beginResetModel();
        m_currentFilter = type;
        m_episodeModel->updateInternalState();
        endResetModel();

        // connect to signals which indicate that read/new statuses have been updated
        if (type == ReadFilter || type == NotReadFilter) {
            connect(&DataManager::instance(), &DataManager::bulkReadStatusActionFinished, this, [this]() {
                beginResetModel();
                m_episodeModel->updateInternalState();
                endResetModel();
            });
        } else if (type == NewFilter || type == NotNewFilter) {
            connect(&DataManager::instance(), &DataManager::bulkNewStatusActionFinished, this, [this]() {
                beginResetModel();
                m_episodeModel->updateInternalState();
                endResetModel();
            });
        }

        Q_EMIT filterTypeChanged();
    }
}

QString EpisodeProxyModel::filterName() const
{
    return getFilterName(m_currentFilter);
}

QString EpisodeProxyModel::getFilterName(FilterType type) const
{
    switch (type) {
    case NoFilter:
        return i18n("No Filter");
    case ReadFilter:
        return i18n("Played Episodes");
    case NotReadFilter:
        return i18n("Unplayed Episodes");
    case NewFilter:
        return i18n("Episodes marked as \"New\"");
    case NotNewFilter:
        return i18n("Episodes not marked as \"New\"");
    default:
        return QString();
    }
}

// Hack to get a QItemSelection in QML
QItemSelection EpisodeProxyModel::createSelection(int rowa, int rowb)
{
    return QItemSelection(index(rowa, 0), index(rowb, 0));
}
