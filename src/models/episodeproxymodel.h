/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QSortFilterProxyModel>

#include "models/episodemodel.h"

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

    explicit EpisodeProxyModel();
    ~EpisodeProxyModel();

    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

    FilterType filterType() const;
    void setFilterType(FilterType type);

Q_SIGNALS:
    void filterTypeChanged(FilterType filter);

private:
    EpisodeModel *m_episodeModel;
    FilterType m_currentFilter;
};
