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

#include "objectslogging.h"

AbstractPodcastSearchModel::AbstractPodcastSearchModel(QObject *parent)
    : QAbstractListModel(parent)
{
    qCDebug(kastsObjects) << "AbstractPodcastSearchModel constructed" << this;
}

AbstractPodcastSearchModel::~AbstractPodcastSearchModel()
{
    qCDebug(kastsObjects) << "AbstractPodcastSearchModel destructed" << this;
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

QVariant AbstractPodcastSearchModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= rowCount(QModelIndex())) {
        // invalid index
        return QVariant();
    }

    switch (role) {
    case NameRole:
        return QVariant::fromValue(m_feeds[index.row()].name);
    case UrlRole:
        return QVariant::fromValue(m_feeds[index.row()].url);
    case ImageRole:
        return QVariant::fromValue(m_feeds[index.row()].image);
    case LinkRole:
        return QVariant::fromValue(m_feeds[index.row()].link);
    case DescriptionRole:
        return QVariant::fromValue(m_feeds[index.row()].description);
    case AuthorRole:
        return QVariant::fromValue(m_feeds[index.row()].authors);
    case LastUpdatedRole:
        return QVariant::fromValue(QDateTime::fromSecsSinceEpoch(m_feeds[index.row()].lastUpdated));
    default:
        return QVariant();
    }
}

int AbstractPodcastSearchModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_feeds.count();
}
