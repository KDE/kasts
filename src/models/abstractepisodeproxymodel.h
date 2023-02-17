/**
 * SPDX-FileCopyrightText: 2021-2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QItemSelection>
#include <QSortFilterProxyModel>
#include <QString>

#include "models/episodemodel.h"

class Entry;

class AbstractEpisodeProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    enum FilterType {
        NoFilter,
        ReadFilter,
        NotReadFilter,
        NewFilter,
        NotNewFilter,
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

    Q_PROPERTY(FilterType filterType READ filterType WRITE setFilterType NOTIFY filterTypeChanged)
    Q_PROPERTY(QString filterName READ filterName NOTIFY filterTypeChanged)
    Q_PROPERTY(QString searchFilter READ searchFilter WRITE setSearchFilter NOTIFY searchFilterChanged)
    Q_PROPERTY(SearchFlags searchFlags READ searchFlags WRITE setSearchFlags NOTIFY searchFlagsChanged)

    explicit AbstractEpisodeProxyModel(QObject *parent = nullptr);

    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

    FilterType filterType() const;
    QString filterName() const;
    QString searchFilter() const;
    SearchFlags searchFlags() const;

    void setFilterType(FilterType type);
    void setSearchFilter(const QString &searchString);
    void setSearchFlags(SearchFlags searchFlags);

    Q_INVOKABLE QString getFilterName(FilterType type) const;
    Q_INVOKABLE QString getSearchFlagName(SearchFlag flag) const;

    Q_INVOKABLE QItemSelection createSelection(int rowa, int rowb);

Q_SIGNALS:
    void filterTypeChanged();
    void searchFilterChanged();
    void searchFlagsChanged();

protected:
    FilterType m_currentFilter = FilterType::NoFilter;
    QString m_searchFilter;
    SearchFlags m_searchFlags;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(AbstractEpisodeProxyModel::SearchFlags)
