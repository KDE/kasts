/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "models/feedsproxymodel.h"

#include <QDebug>

#include <KLocalizedString>

FeedsProxyModel::FeedsProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    m_feedsModel = new FeedsModel(this);
    setSourceModel(m_feedsModel);
    setDynamicSortFilter(true);
    sort(0);
}

bool FeedsProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    QString leftTitle = sourceModel()->data(left, FeedsModel::TitleRole).toString();
    QString rightTitle = sourceModel()->data(right, FeedsModel::TitleRole).toString();

    if (m_currentSort == SortType::UnreadDescending || m_currentSort == SortType::UnreadAscending) {
        int leftUnreadCount = sourceModel()->data(left, FeedsModel::UnreadCountRole).toInt();
        int rightUnreadCount = sourceModel()->data(right, FeedsModel::UnreadCountRole).toInt();

        if (leftUnreadCount != rightUnreadCount) {
            if (m_currentSort == SortType::UnreadDescending) {
                return leftUnreadCount > rightUnreadCount;
            } else {
                return leftUnreadCount < rightUnreadCount;
            }
        }
    } else if (m_currentSort == SortType::NewDescending || m_currentSort == SortType::NewAscending) {
        int leftNewCount = sourceModel()->data(left, FeedsModel::NewCountRole).toInt();
        int rightNewCount = sourceModel()->data(right, FeedsModel::NewCountRole).toInt();

        if (leftNewCount != rightNewCount) {
            if (m_currentSort == SortType::NewDescending) {
                return leftNewCount > rightNewCount;
            } else {
                return leftNewCount < rightNewCount;
            }
        }
    } else if (m_currentSort == SortType::FavoriteDescending || m_currentSort == SortType::FavoriteAscending) {
        int leftFavoriteCount = sourceModel()->data(left, FeedsModel::FavoriteCountRole).toInt();
        int rightFavoriteCount = sourceModel()->data(right, FeedsModel::FavoriteCountRole).toInt();

        if (leftFavoriteCount != rightFavoriteCount) {
            if (m_currentSort == SortType::FavoriteDescending) {
                return leftFavoriteCount > rightFavoriteCount;
            } else {
                return leftFavoriteCount < rightFavoriteCount;
            }
        }
    } else if (m_currentSort == SortType::TitleDescending) {
        return QString::localeAwareCompare(leftTitle, rightTitle) > 0;
    }

    // In case there is a "tie" always use ascending alphabetical ordering
    return QString::localeAwareCompare(leftTitle, rightTitle) < 0;
}

bool FeedsProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);

    bool found = m_searchFilter.isEmpty();
    if (!m_searchFilter.isEmpty()) {
        if (sourceModel()->data(index, FeedsModel::Roles::TitleRole).value<QString>().contains(m_searchFilter, Qt::CaseInsensitive)) {
            found |= true;
        }
    }

    return found;
}

QString FeedsProxyModel::searchFilter() const
{
    return m_searchFilter;
}

FeedsProxyModel::SortType FeedsProxyModel::sortType() const
{
    return m_currentSort;
}

void FeedsProxyModel::setSearchFilter(const QString &searchString)
{
    if (searchString != m_searchFilter) {
        beginResetModel();
        m_searchFilter = searchString;
        endResetModel();

        Q_EMIT searchFilterChanged();
    }
}

void FeedsProxyModel::setSortType(SortType type)
{
    if (type != m_currentSort) {
        m_currentSort = type;

        // HACK: get the list re-sorted with a custom lessThan implementation
        sort(-1);
        sort(0);

        Q_EMIT sortTypeChanged();
    }
}

QString FeedsProxyModel::getSortName(SortType type) const
{
    switch (type) {
    case SortType::UnreadDescending:
        return i18nc("@label:chooser Sort podcasts by decreasing number of unplayed episodes", "Unplayed count: descending");
    case SortType::UnreadAscending:
        return i18nc("@label:chooser Sort podcasts by increasing number of unplayed episodes", "Unplayed count: ascending");
    case SortType::NewDescending:
        return i18nc("@label:chooser Sort podcasts by decreasing number of new episodes", "New count: descending");
    case SortType::NewAscending:
        return i18nc("@label:chooser Sort podcasts by increasing number of new episodes", "New count: ascending");
    case SortType::FavoriteDescending:
        return i18nc("@label:chooser Sort podcasts by decreasing number of favorites", "Favorite count: descending");
    case SortType::FavoriteAscending:
        return i18nc("@label:chooser Sort podcasts by increasing number of favorites", "Favorite count: ascending");
    case SortType::TitleAscending:
        return i18nc("@label:chooser Sort podcasts titles alphabetically", "Podcast title: A → Z");
    case SortType::TitleDescending:
        return i18nc("@label:chooser Sort podcasts titles in reverse alphabetical order", "Podcast title: Z → A");
    default:
        return QString();
    }
}

QString FeedsProxyModel::getSortIconName(SortType type) const
{
    switch (type) {
    case SortType::UnreadDescending:
    case SortType::NewDescending:
    case SortType::FavoriteDescending:
        return QStringLiteral("view-sort-descending");
    case SortType::UnreadAscending:
    case SortType::NewAscending:
    case SortType::FavoriteAscending:
        return QStringLiteral("view-sort-ascending");
    case SortType::TitleDescending:
        return QStringLiteral("view-sort-descending-name");
    case SortType::TitleAscending:
        return QStringLiteral("view-sort-ascending-name");
    default:
        return QString();
    }
}

// Hack to get a QItemSelection in QML
QItemSelection FeedsProxyModel::createSelection(int rowa, int rowb)
{
    return QItemSelection(index(rowa, 0), index(rowb, 0));
}
