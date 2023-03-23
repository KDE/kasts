/**
 * SPDX-FileCopyrightText: 2021-2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "models/episodeproxymodel.h"

#include <KLocalizedString>

#include "datamanager.h"
#include "entry.h"
#include "models/abstractepisodemodel.h"

AbstractEpisodeProxyModel::AbstractEpisodeProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    m_searchFlags = SearchFlag::TitleFlag | SearchFlag::ContentFlag | SearchFlag::FeedNameFlag;
}

bool AbstractEpisodeProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);

    bool accepted = true;

    switch (m_currentFilter) {
    case NoFilter:
        accepted = true;
        break;
    case ReadFilter:
        accepted = sourceModel()->data(index, AbstractEpisodeModel::Roles::ReadRole).value<bool>();
        break;
    case NotReadFilter:
        accepted = !sourceModel()->data(index, AbstractEpisodeModel::Roles::ReadRole).value<bool>();
        break;
    case NewFilter:
        accepted = sourceModel()->data(index, AbstractEpisodeModel::Roles::NewRole).value<bool>();
        break;
    case NotNewFilter:
        accepted = !sourceModel()->data(index, AbstractEpisodeModel::Roles::NewRole).value<bool>();
        break;
    case FavoriteFilter:
        accepted = sourceModel()->data(index, AbstractEpisodeModel::Roles::FavoriteRole).value<bool>();
        break;
    case NotFavoriteFilter:
        accepted = !sourceModel()->data(index, AbstractEpisodeModel::Roles::FavoriteRole).value<bool>();
        break;
    default:
        accepted = true;
        break;
    }

    bool found = m_searchFilter.isEmpty();
    if (!m_searchFilter.isEmpty()) {
        if (m_searchFlags & SearchFlag::TitleFlag) {
            if (sourceModel()->data(index, AbstractEpisodeModel::Roles::TitleRole).value<QString>().contains(m_searchFilter, Qt::CaseInsensitive)) {
                found |= true;
            }
        }
        if (m_searchFlags & SearchFlag::ContentFlag) {
            if (sourceModel()->data(index, AbstractEpisodeModel::Roles::ContentRole).value<QString>().contains(m_searchFilter, Qt::CaseInsensitive)) {
                found |= true;
            }
        }
        if (m_searchFlags & SearchFlag::FeedNameFlag) {
            if (sourceModel()->data(index, AbstractEpisodeModel::Roles::FeedNameRole).value<QString>().contains(m_searchFilter, Qt::CaseInsensitive)) {
                found |= true;
            }
        }
    }

    accepted = accepted && found;

    return accepted;
}

AbstractEpisodeProxyModel::FilterType AbstractEpisodeProxyModel::filterType() const
{
    return m_currentFilter;
}

QString AbstractEpisodeProxyModel::filterName() const
{
    return getFilterName(m_currentFilter);
}

QString AbstractEpisodeProxyModel::searchFilter() const
{
    return m_searchFilter;
}

AbstractEpisodeProxyModel::SearchFlags AbstractEpisodeProxyModel::searchFlags() const
{
    return m_searchFlags;
}

void AbstractEpisodeProxyModel::setFilterType(FilterType type)
{
    if (type != m_currentFilter) {
        disconnect(&DataManager::instance(), &DataManager::bulkReadStatusActionFinished, this, nullptr);
        disconnect(&DataManager::instance(), &DataManager::bulkNewStatusActionFinished, this, nullptr);
        disconnect(&DataManager::instance(), &DataManager::bulkFavoriteStatusActionFinished, this, nullptr);

        beginResetModel();
        m_currentFilter = type;
        dynamic_cast<AbstractEpisodeModel *>(sourceModel())->updateInternalState();
        endResetModel();

        // connect to signals which indicate that read/new statuses have been updated
        if (type == ReadFilter || type == NotReadFilter) {
            connect(&DataManager::instance(), &DataManager::bulkReadStatusActionFinished, this, [this]() {
                beginResetModel();
                dynamic_cast<AbstractEpisodeModel *>(sourceModel())->updateInternalState();
                endResetModel();
            });
        } else if (type == NewFilter || type == NotNewFilter) {
            connect(&DataManager::instance(), &DataManager::bulkNewStatusActionFinished, this, [this]() {
                beginResetModel();
                dynamic_cast<AbstractEpisodeModel *>(sourceModel())->updateInternalState();
                endResetModel();
            });
        } else if (type == FavoriteFilter || type == NotFavoriteFilter) {
            connect(&DataManager::instance(), &DataManager::bulkFavoriteStatusActionFinished, this, [this]() {
                beginResetModel();
                dynamic_cast<AbstractEpisodeModel *>(sourceModel())->updateInternalState();
                endResetModel();
            });
        }

        Q_EMIT filterTypeChanged();
    }
}

void AbstractEpisodeProxyModel::setSearchFilter(const QString &searchString)
{
    if (searchString != m_searchFilter) {
        beginResetModel();
        m_searchFilter = searchString;
        endResetModel();

        Q_EMIT searchFilterChanged();
    }
}

void AbstractEpisodeProxyModel::setSearchFlags(AbstractEpisodeProxyModel::SearchFlags searchFlags)
{
    if (searchFlags != m_searchFlags) {
        beginResetModel();
        m_searchFlags = searchFlags;
        endResetModel();
    }
}

QString AbstractEpisodeProxyModel::getFilterName(FilterType type) const
{
    switch (type) {
    case FilterType::NoFilter:
        return i18nc("@label:chooser Choice of filter for episode list", "No Filter");
    case FilterType::ReadFilter:
        return i18nc("@label:chooser Choice of filter for episode list", "Played Episodes");
    case FilterType::NotReadFilter:
        return i18nc("@label:chooser Choice of filter for episode list", "Unplayed Episodes");
    case FilterType::NewFilter:
        return i18nc("@label:chooser Choice of filter for episode list", "Episodes marked as \"New\"");
    case FilterType::NotNewFilter:
        return i18nc("@label:chooser Choice of filter for episode list", "Episodes not marked as \"New\"");
    case FilterType::FavoriteFilter:
        return i18nc("@label:chooser Choice of filter for episode list", "Episodes marked as Favorite");
    case FilterType::NotFavoriteFilter:
        return i18nc("@label:chooser Choice of filter for episode list", "Episodes not marked as Favorite");
    default:
        return QString();
    }
}

QString AbstractEpisodeProxyModel::getSearchFlagName(SearchFlag flag) const
{
    switch (flag) {
    case SearchFlag::TitleFlag:
        return i18nc("@label:chooser Choice of fields to search for string", "Title");
    case SearchFlag::ContentFlag:
        return i18nc("@label:chooser Choice of fields to search for string", "Description");
    case SearchFlag::FeedNameFlag:
        return i18nc("@label:chooser Choice of fields to search for string", "Podcast Title");
    default:
        return QString();
    }
}

// Hack to get a QItemSelection in QML
QItemSelection AbstractEpisodeProxyModel::createSelection(int rowa, int rowb)
{
    return QItemSelection(index(rowa, 0), index(rowb, 0));
}
