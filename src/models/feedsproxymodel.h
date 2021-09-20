/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QItemSelection>
#include <QSortFilterProxyModel>

#include "models/feedsmodel.h"

class Entry;

class FeedsProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit FeedsProxyModel(QObject *parent = nullptr);

    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

    Q_INVOKABLE QItemSelection createSelection(int rowa, int rowb);

private:
    FeedsModel *m_feedsModel;
};
