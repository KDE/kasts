/**
 * SPDX-FileCopyrightText: 2021-2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QItemSelection>
#include <QQmlEngine>
#include <QSortFilterProxyModel>
#include <QString>

#include "models/episodemodel.h"

class Entry;

class AbstractEpisodeProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

public:
    enum FilterType {
        NoFilter = 0,
        ReadFilter,
        NotReadFilter,
        NewFilter,
        NotNewFilter,
        FavoriteFilter,
        NotFavoriteFilter,
    };
    Q_ENUM(FilterType)

    enum SearchFlag {
        TitleFlag = 0x01,
        ContentFlag = 0x02,
        FeedNameFlag = 0x04,
    };
    Q_ENUM(SearchFlag)
    Q_DECLARE_FLAGS(SearchFlags, SearchFlag)
    Q_FLAGS(SearchFlags)

    enum SortType {
        DateDescending,
        DateAscending,
    };
    Q_ENUM(SortType)

    Q_PROPERTY(FilterType filterType READ filterType WRITE setFilterType NOTIFY filterTypeChanged)
    Q_PROPERTY(QString filterName READ filterName NOTIFY filterTypeChanged)
    Q_PROPERTY(QString searchFilter READ searchFilter WRITE setSearchFilter NOTIFY searchFilterChanged)
    Q_PROPERTY(SearchFlags searchFlags READ searchFlags WRITE setSearchFlags NOTIFY searchFlagsChanged)
    Q_PROPERTY(SortType sortType READ sortType WRITE setSortType NOTIFY sortTypeChanged)

    explicit AbstractEpisodeProxyModel(QObject *parent = nullptr);

    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

    FilterType filterType() const;
    QString filterName() const;
    QString searchFilter() const;
    SearchFlags searchFlags() const;
    SortType sortType() const;

    void setFilterType(FilterType type);
    void setSearchFilter(const QString &searchString);
    void setSearchFlags(SearchFlags searchFlags);
    void setSortType(SortType type);

    Q_INVOKABLE static QString getFilterName(FilterType type);
    Q_INVOKABLE static QString getSearchFlagName(SearchFlag flag);
    Q_INVOKABLE static QString getSortName(SortType type);
    Q_INVOKABLE static QString getSortIconName(SortType type);

    Q_INVOKABLE QItemSelection createSelection(int rowa, int rowb);

Q_SIGNALS:
    void filterTypeChanged();
    void searchFilterChanged();
    void searchFlagsChanged();
    void sortTypeChanged();

protected:
    FilterType m_currentFilter = FilterType::NoFilter;
    QString m_searchFilter;
    SearchFlags m_searchFlags;
    SortType m_currentSort = SortType::DateDescending;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(AbstractEpisodeProxyModel::SearchFlags)
Q_DECLARE_METATYPE(AbstractEpisodeProxyModel::SortType)
