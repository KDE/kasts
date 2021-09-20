/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "models/feedsproxymodel.h"

FeedsProxyModel::FeedsProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    m_feedsModel = new FeedsModel(this);
    setSourceModel(m_feedsModel);
    setDynamicSortFilter(true);
    sort(0, Qt::AscendingOrder);
}

bool FeedsProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    QString leftTitle = sourceModel()->data(left, FeedsModel::TitleRole).toString();
    QString rightTitle = sourceModel()->data(right, FeedsModel::TitleRole).toString();
    int leftUnreadCount = sourceModel()->data(left, FeedsModel::UnreadCountRole).toInt();
    int rightUnreadCount = sourceModel()->data(right, FeedsModel::UnreadCountRole).toInt();

    if (leftUnreadCount == rightUnreadCount) {
        return QString::localeAwareCompare(leftTitle, rightTitle) < 0;
    } else {
        return leftUnreadCount > rightUnreadCount;
    }
}

// Hack to get a QItemSelection in QML
QItemSelection FeedsProxyModel::createSelection(int rowa, int rowb)
{
    return QItemSelection(index(rowa, 0), index(rowb, 0));
}
