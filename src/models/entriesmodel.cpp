/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <tobias.fella@kde.org>
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "models/entriesmodel.h"

#include <QSqlQuery>
#include <QString>
#include <QTimer>
#include <qhashfunctions.h>

#include "database.h"
#include "datamanager.h"
#include "datatypes.h"

EntriesModel::EntriesModel(const qint64 feeduid, QObject *parent)
    : AbstractEpisodeModel(parent) // TODO: probably needs another parent?
    , m_feeduid(feeduid)
{
    // When feed is updated, the entire model needs to be reset because we
    // cannot know where the new entries will be inserted into the list (or that
    // maybe even items have been removed.
    connect(&DataManager::instance(), &DataManager::feedEntriesUpdated, this, [this](const qint64 feeduid) {
        if (m_feeduid == feeduid) {
            beginResetModel();
            updateInternalState();
            endResetModel();
        }
    });

    QTimer::singleShot(0, this, [this]() {
        beginResetModel();
        updateInternalState();
        endResetModel();
    });
}

QVariant EntriesModel::data(const QModelIndex &index, int role) const
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
        return QVariant::fromValue(m_feedname);
    case AbstractEpisodeModel::Roles::UpdatedRole:
        return QVariant::fromValue(m_entries[index.row()].updated);
    default:
        return QVariant();
    }
}

int EntriesModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_entries.count();
}

void EntriesModel::updateInternalState()
{
    m_entries.clear();

    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT * FROM Entries WHERE feeduid=:feeduid ORDER BY updated DESC;"));
    query.bindValue(QStringLiteral(":feeduid"), m_feeduid);
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
    }
    query.finish();

    query.prepare(QStringLiteral("SELECT name FROM Feeds WHERE feeduid=:feeduid;"));
    query.bindValue(QStringLiteral(":feeduid"), m_feeduid);
    Database::instance().execute(query);
    if (query.next()) {
        m_feedname = query.value(QStringLiteral("name")).toString();
    }
    query.finish();
}
