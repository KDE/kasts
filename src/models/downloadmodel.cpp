/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "models/downloadmodel.h"
#include "models/downloadmodellogging.h"

#include <QSqlQuery>

#include "database.h"
#include "datamanager.h"
#include "enclosure.h"
#include "models/abstractepisodemodel.h"

DownloadModel::DownloadModel()
    : QAbstractListModel(nullptr)
{
    updateInternalState();
}

QVariant DownloadModel::data(const QModelIndex &index, int role) const
{
    switch (role) {
    case AbstractEpisodeModel::Roles::TitleRole:
        return QVariant::fromValue(m_entries[index.row()].title);
    case AbstractEpisodeModel::Roles::EntryuidRole:
        return QVariant::fromValue(m_entries[index.row()].entryuid);
    case AbstractEpisodeModel::Roles::EntryRole:
        return QVariant::fromValue(DataManager::instance().getEntry(m_entries[index.row()].entryuid));
    case AbstractEpisodeModel::Roles::ContentRole:
        return QVariant::fromValue(m_entries[index.row()].content);
    case AbstractEpisodeModel::Roles::IdRole:
        return QVariant::fromValue(m_entries[index.row()].id);
    case AbstractEpisodeModel::Roles::ReadRole:
        return QVariant::fromValue(m_entries[index.row()].read);
    case AbstractEpisodeModel::Roles::NewRole:
        return QVariant::fromValue(m_entries[index.row()].isNew);
    case AbstractEpisodeModel::Roles::FavoriteRole:
        return QVariant::fromValue(m_entries[index.row()].favorite);
    case AbstractEpisodeModel::Roles::FeedNameRole:
        return QVariant::fromValue(m_feedNames[index.row()]);
    case AbstractEpisodeModel::Roles::UpdatedRole:
        return QVariant::fromValue(m_entries[index.row()].updated);
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> DownloadModel::roleNames() const
{
    return {
        {AbstractEpisodeModel::Roles::TitleRole, "title"},
        {AbstractEpisodeModel::Roles::EntryuidRole, "entryuid"},
        {AbstractEpisodeModel::Roles::EntryRole, "entry"},
        {AbstractEpisodeModel::Roles::IdRole, "id"},
        {AbstractEpisodeModel::Roles::ReadRole, "read"},
        {AbstractEpisodeModel::Roles::NewRole, "new"},
        {AbstractEpisodeModel::Roles::FavoriteRole, "favorite"},
        {AbstractEpisodeModel::Roles::ContentRole, "content"},
        {AbstractEpisodeModel::Roles::FeedNameRole, "feedname"},
        {AbstractEpisodeModel::Roles::UpdatedRole, "updated"},
    };
}

int DownloadModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_entries.count();
}

void DownloadModel::monitorDownloadStatus()
{
    beginResetModel();
    updateInternalState();
    endResetModel();
}

void DownloadModel::updateInternalState()
{
    m_entries.clear();
    const QList<Enclosure::Status> statuses = {Enclosure::Status::Downloading, Enclosure::Status::PartiallyDownloaded, Enclosure::Status::Downloaded};

    QSqlQuery query;
    query.prepare(
        QStringLiteral("SELECT * FROM Entries JOIN Enclosures ON Enclosures.entryuid = Entries.entryuid JOIN Feeds ON Feeds.feeduid = Entries.feeduid WHERE "
                       "Enclosures.downloaded=:downloaded "
                       "ORDER BY updated DESC;"));
    for (const Enclosure::Status status : statuses) {
        query.bindValue(QStringLiteral(":downloaded"), Enclosure::statusToDb(status));
        Database::instance().execute(query);
        while (query.next()) {
            DataTypes::EntryDetails entryDetails;
            entryDetails.entryuid = query.value(QStringLiteral("entryuid")).toLongLong();
            entryDetails.feeduid = query.value(QStringLiteral("feeduid")).toLongLong();
            entryDetails.id = query.value(QStringLiteral("id")).toString();
            entryDetails.title = query.value(QStringLiteral("title")).toString();
            entryDetails.content = query.value(QStringLiteral("content")).toString();
            entryDetails.created = query.value(QStringLiteral("created")).toInt();
            entryDetails.updated = query.value(QStringLiteral("updated")).toInt();
            entryDetails.read = query.value(QStringLiteral("read")).toBool();
            entryDetails.isNew = query.value(QStringLiteral("new")).toBool();
            entryDetails.favorite = query.value(QStringLiteral("favorite")).toBool();
            entryDetails.link = query.value(QStringLiteral("link")).toString();
            entryDetails.hasEnclosure = query.value(QStringLiteral("hasEnclosure")).toBool();
            entryDetails.image = query.value(QStringLiteral("image")).toString();
            m_entries += entryDetails;
            m_feedNames += query.value(QStringLiteral("Feeds.name")).toString();
        }
    }
}

// Hack to get a QItemSelection in QML
QItemSelection DownloadModel::createSelection(int rowa, int rowb)
{
    return QItemSelection(index(rowa, 0), index(rowb, 0));
}
