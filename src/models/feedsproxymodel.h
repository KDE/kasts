/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QItemSelection>
#include <QModelIndex>
#include <QSortFilterProxyModel>
#include <QString>

#include "models/feedsmodel.h"

class Entry;

class FeedsProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    enum SortType {
        UnreadDescending,
        UnreadAscending,
        NewDescending,
        NewAscending,
        FavoriteDescending,
        FavoriteAscending,
        TitleAscending,
        TitleDescending,
    };
    Q_ENUM(SortType)

    Q_PROPERTY(QString searchFilter READ searchFilter WRITE setSearchFilter NOTIFY searchFilterChanged)
    Q_PROPERTY(SortType sortType READ sortType WRITE setSortType NOTIFY sortTypeChanged)

    explicit FeedsProxyModel(QObject *parent = nullptr);

    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

    QString searchFilter() const;
    SortType sortType() const;

    void setSearchFilter(const QString &searchString);
    void setSortType(SortType type);

    Q_INVOKABLE QString getSortName(SortType type) const;
    Q_INVOKABLE QString getSortIconName(SortType type) const;

    Q_INVOKABLE QItemSelection createSelection(int rowa, int rowb);

Q_SIGNALS:
    void searchFilterChanged();
    void sortTypeChanged();

private:
    FeedsModel *m_feedsModel;

    QString m_searchFilter;
    SortType m_currentSort = SortType::UnreadDescending;
};

Q_DECLARE_METATYPE(FeedsProxyModel::SortType)
