/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QItemSelection>
#include <QSortFilterProxyModel>

#include "models/episodemodel.h"

class Entry;

class EpisodeProxyModel : public QSortFilterProxyModel
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

    Q_PROPERTY(FilterType filterType READ filterType WRITE setFilterType NOTIFY filterTypeChanged)
    Q_PROPERTY(QString filterName READ filterName NOTIFY filterTypeChanged)

    explicit EpisodeProxyModel();
    ~EpisodeProxyModel();

    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

    FilterType filterType() const;
    QString filterName() const;
    void setFilterType(FilterType type);

    Q_INVOKABLE QString getFilterName(FilterType type) const;

    Q_INVOKABLE QItemSelection createSelection(int rowa, int rowb);

Q_SIGNALS:
    void filterTypeChanged();

private:
    EpisodeModel *m_episodeModel;
    FilterType m_currentFilter;
};
