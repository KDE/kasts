/**
 * SPDX-FileCopyrightText: 2026 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "models/abstractpodcastsearchmodel.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QString>
#include <QVariant>

AbstractPodcastSearchModel::AbstractPodcastSearchModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

QHash<int, QByteArray> AbstractPodcastSearchModel::roleNames() const
{
    return {
        {NameRole, "name"},
        {UrlRole, "url"},
        {ImageRole, "image"},
        {LinkRole, "link"},
        {DescriptionRole, "description"},
        {AuthorRole, "author"},
        {LastUpdatedRole, "lastUpdated"},
    };
}
