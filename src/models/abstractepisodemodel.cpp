/**
 * SPDX-FileCopyrightText: 2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "models/abstractepisodemodel.h"

AbstractEpisodeModel::AbstractEpisodeModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

QHash<int, QByteArray> AbstractEpisodeModel::roleNames() const
{
    return {
        {TitleRole, "title"},
        {EntryRole, "entry"},
        {IdRole, "id"},
        {ReadRole, "read"},
        {NewRole, "new"},
        {FavoriteRole, "favorite"},
        {ContentRole, "content"},
        {FeedNameRole, "feedname"},
    };
}
