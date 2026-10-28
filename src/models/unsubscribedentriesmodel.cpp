/**
 * SPDX-FileCopyrightText: 2026 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "models/unsubscribedentriesmodel.h"

#include <QObject>

#include <ThreadWeaver/Queue>
#include <ThreadWeaver/ThreadWeaver>

#include "datatypes.h"
#include "models/entriesmodellogging.h"
#include "objectslogging.h"
#include "utils/updatefeedjob.h"

UnsubscribedEntriesModel::UnsubscribedEntriesModel(const QString &url, QObject *parent)
    : AbstractEpisodeModel(parent)
    , m_url(url)

{
    // TODO: should this go through the metered network checks first?
    UpdateFeedJob *updateFeedJob = new UpdateFeedJob(m_url, this);
    connect(updateFeedJob, &UpdateFeedJob::feedDetailsRetrieved, this, &UnsubscribedEntriesModel::updateEntries);

    ThreadWeaver::Queue::instance()->stream() << updateFeedJob;
    qCDebug(kastsEntriesModel) << "Enqueued updateFeedJob for feed" << m_url;

    qCDebug(kastsObjects) << "UnsubscribedEntriesModel constructed" << m_url << this;
}

UnsubscribedEntriesModel::~UnsubscribedEntriesModel()
{
    qCDebug(kastsObjects) << "UnsubscribedEntriesModel destructed" << m_url << this;
}

QVariant UnsubscribedEntriesModel::data(const QModelIndex &index, int role) const
{
    qCDebug(kastsEntriesModel) << "calling data for row" << index << "and role" << roleNames()[role];

    if (!index.isValid()) {
        return QVariant();
    }

    switch (role) {
    case AbstractEpisodeModel::Roles::TitleRole:
        return QVariant::fromValue(m_feed.entries[m_feed.entryOrder[index.row()]].title);
    case AbstractEpisodeModel::Roles::EntryuidRole:
        return QVariant::fromValue(0);
    case AbstractEpisodeModel::Roles::IdRole:
        return QVariant::fromValue(m_feed.entries[m_feed.entryOrder[index.row()]].id);
    case AbstractEpisodeModel::Roles::QueueStatusRole:
        return QVariant::fromValue(false);
    case AbstractEpisodeModel::Roles::ReadRole:
        return QVariant::fromValue(false);
    case AbstractEpisodeModel::Roles::NewRole:
        return QVariant::fromValue(false);
    case AbstractEpisodeModel::Roles::FavoriteRole:
        return QVariant::fromValue(false);
    case AbstractEpisodeModel::Roles::RemovedRole:
        return QVariant::fromValue(false);
    case AbstractEpisodeModel::Roles::ContentRole:
        return QVariant::fromValue(m_feed.entries[m_feed.entryOrder[index.row()]].content);
    case AbstractEpisodeModel::Roles::CreatedRole:
        return QVariant::fromValue(QDateTime::fromSecsSinceEpoch(m_feed.entries[m_feed.entryOrder[index.row()]].created));
    case AbstractEpisodeModel::Roles::UpdatedRole:
        return QVariant::fromValue(QDateTime::fromSecsSinceEpoch(m_feed.entries[m_feed.entryOrder[index.row()]].updated));
    case AbstractEpisodeModel::Roles::LinkRole:
        return QVariant::fromValue(m_feed.entries[m_feed.entryOrder[index.row()]].link);
    case AbstractEpisodeModel::Roles::ImageRole: {
        if (!m_feed.entries[m_feed.entryOrder[index.row()]].image.isEmpty()) {
            return QVariant::fromValue(m_feed.entries[m_feed.entryOrder[index.row()]].image);
        } else {
            return QVariant::fromValue(m_feed.image);
        }
    }
    case AbstractEpisodeModel::Roles::HasEnclosureRole:
        return QVariant::fromValue(m_feed.entries[m_feed.entryOrder[index.row()]].hasEnclosure);
    case AbstractEpisodeModel::Roles::EnclosureUrlRole: {
        if (m_feed.entries[m_feed.entryOrder[index.row()]].hasEnclosure) {
            QString key = m_feed.entries[m_feed.entryOrder[index.row()]].enclosureOrder[0];
            return QVariant::fromValue(m_feed.entries[m_feed.entryOrder[index.row()]].enclosures[key].url);
        } else {
            return QVariant::fromValue(QStringLiteral(""));
        }
    }
    case AbstractEpisodeModel::Roles::PlayPositionRole:
        return QVariant::fromValue(0);
    case AbstractEpisodeModel::Roles::DurationRole: {
        if (m_feed.entries[m_feed.entryOrder[index.row()]].hasEnclosure) {
            QString key = m_feed.entries[m_feed.entryOrder[index.row()]].enclosureOrder[0];
            return QVariant::fromValue(m_feed.entries[m_feed.entryOrder[index.row()]].enclosures[key].duration);
        } else {
            return QVariant::fromValue(QStringLiteral(""));
        }
    }
    case AbstractEpisodeModel::Roles::SizeRole: {
        if (m_feed.entries[m_feed.entryOrder[index.row()]].hasEnclosure) {
            QString key = m_feed.entries[m_feed.entryOrder[index.row()]].enclosureOrder[0];
            return QVariant::fromValue(m_feed.entries[m_feed.entryOrder[index.row()]].enclosures[key].size);
        } else {
            return QVariant::fromValue(QStringLiteral(""));
        }
    }
    case AbstractEpisodeModel::Roles::EnclosureStatusRole: {
        if (m_feed.entries[m_feed.entryOrder[index.row()]].hasEnclosure) {
            QString key = m_feed.entries[m_feed.entryOrder[index.row()]].enclosureOrder[0];
            return QVariant::fromValue(m_feed.entries[m_feed.entryOrder[index.row()]].enclosures[key].downloaded);
        } else {
            return QVariant::fromValue(QStringLiteral(""));
        }
    }
    case AbstractEpisodeModel::Roles::EnclosureStatusOrderRole: {
        if (m_feed.entries[m_feed.entryOrder[index.row()]].hasEnclosure) {
            QString key = m_feed.entries[m_feed.entryOrder[index.row()]].enclosureOrder[0];
            return QVariant::fromValue(static_cast<int>(m_feed.entries[m_feed.entryOrder[index.row()]].enclosures[key].downloaded));
        } else {
            return QVariant::fromValue(static_cast<int>(DataTypes::EnclosureStatus::NoEnclosure));
        }
    }
    case AbstractEpisodeModel::Roles::DownloadSizeRole:
        return QVariant::fromValue(0);
    case AbstractEpisodeModel::Roles::FeeduidRole:
        return QVariant::fromValue(0);
    case AbstractEpisodeModel::Roles::FeedNameRole:
        return QVariant::fromValue(m_feed.name);
    case AbstractEpisodeModel::Roles::FeedImageRole:
        return QVariant::fromValue(m_feed.image);
    default:
        return QVariant();
    }
}

int UnsubscribedEntriesModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_feed.entryOrder.count();
}

void UnsubscribedEntriesModel::updateEntries(const DataTypes::FeedUpdateDetails &feedDetails)
{
    beginResetModel();
    m_feed = feedDetails;
    endResetModel();

    qCDebug(kastsEntriesModel) << "Received updates for unsubscribed feed" << m_url;
}
