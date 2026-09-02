/**
 * SPDX-FileCopyrightText: 2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "models/abstractepisodemodel.h"

#include <QHash>
#include <QList>
#include <QModelIndex>
#include <QObject>
#include <QSqlQuery>
#include <QTimer>
#include <QVariant>

#include "database.h"
#include "datamanager.h"
#include "datatypes.h"
#include "fetcher.h"
#include "models/episodemodellogging.h"
#include "objectslogging.h"
#include "queuemodel.h"
#include "storagemanager.h"
#include "utils/entryutils.h"

AbstractEpisodeModel::AbstractEpisodeModel(const QString &feedQuery, const QString &entryQuery, const QString &enclosureQuery, QObject *parent)
    : QAbstractListModel(parent)
    , m_feedQuery(feedQuery)
    , m_entryQuery(entryQuery)
    , m_enclosureQuery(enclosureQuery)
{
    connect(&Fetcher::instance(), &Fetcher::feedDetailsUpdated, this, [this](const qint64 feeduid) {
        if (m_feeds.contains(feeduid)) {
            updateFeeds({feeduid});
            // TODO: consider using dataChanged on the relevant feed properties??
        }
    });

    connect(&Fetcher::instance(), &Fetcher::entriesUpdated, this, [this](const QList<qint64> &entryuids) {
        updateEntries(entryuids);
        for (const qint64 entryuid : std::as_const(entryuids)) {
            qsizetype idx = m_entryOrder.indexOf(entryuid);
            if (idx > -1 && m_entries.contains(entryuid)) {
                Q_EMIT dataChanged(index(idx, 0), index(idx, 0), {AbstractEpisodeModel::Roles::ReadRole});
            }
        }
    });
    // The following lambda slot function ensure that any changes signaled by
    // DataManager are also applied to the model data and propagated correctly.
    connect(&DataManager::instance(), &DataManager::entryQueueStatusChanged, this, [this](bool state, const QList<qint64> &entryuids) {
        Q_UNUSED(state)
        for (const qint64 entryuid : std::as_const(entryuids)) {
            qsizetype idx = m_entryOrder.indexOf(entryuid);
            if (idx > -1 && m_entries.contains(entryuid)) {
                Q_EMIT dataChanged(index(idx, 0), index(idx, 0), {AbstractEpisodeModel::Roles::QueueStatusRole});
            }
        }
    });
    connect(&DataManager::instance(), &DataManager::entryReadStatusChanged, this, [this](bool state, const QList<qint64> &entryuids) {
        for (const qint64 entryuid : std::as_const(entryuids)) {
            qsizetype idx = m_entryOrder.indexOf(entryuid);
            if (idx > -1 && m_entries.contains(entryuid)) {
                m_entries[entryuid].read = state;
                Q_EMIT dataChanged(index(idx, 0), index(idx, 0), {AbstractEpisodeModel::Roles::ReadRole});
            }
        }
    });
    connect(&DataManager::instance(), &DataManager::entryNewStatusChanged, this, [this](bool state, const QList<qint64> &entryuids) {
        for (const qint64 entryuid : std::as_const(entryuids)) {
            qsizetype idx = m_entryOrder.indexOf(entryuid);
            if (idx > -1 && m_entries.contains(entryuid)) {
                m_entries[entryuid].isNew = state;
                Q_EMIT dataChanged(index(idx, 0), index(idx, 0), {AbstractEpisodeModel::Roles::NewRole});
            }
        }
    });
    connect(&DataManager::instance(), &DataManager::entryFavoriteStatusChanged, this, [this](bool state, const QList<qint64> &entryuids) {
        for (const qint64 entryuid : std::as_const(entryuids)) {
            qsizetype idx = m_entryOrder.indexOf(entryuid);
            if (idx > -1 && m_entries.contains(entryuid)) {
                m_entries[entryuid].favorite = state;
                Q_EMIT dataChanged(index(idx, 0), index(idx, 0), {AbstractEpisodeModel::Roles::FavoriteRole});
            }
        }
    });
    connect(&DataManager::instance(),
            &DataManager::enclosureStatusesChanged,
            this,
            [this](const QList<DataTypes::EnclosureStatus> &statuses, const QList<qint64> &entryuids) {
                Q_ASSERT(entryuids.size() == statuses.size());
                for (int i = 0; i < entryuids.size(); i++) {
                    qsizetype idx = m_entryOrder.indexOf(entryuids[i]);
                    if (idx > -1 && m_entries[entryuids[i]].enclosureOrder.length() > 0) {
                        m_entries[entryuids[i]].enclosures[m_entries[entryuids[i]].enclosureOrder[0]].downloaded = statuses[i];
                        Q_EMIT dataChanged(index(idx, 0), index(idx, 0), {AbstractEpisodeModel::Roles::DownloadedRole});
                        // The image might also have changed, e.g. through an embedded image in the id3 tag
                        Q_EMIT dataChanged(index(idx, 0), index(idx, 0), {AbstractEpisodeModel::Roles::ImageRole});
                    }
                }
            });
    connect(&DataManager::instance(), &DataManager::entryPlayPositionsChanged, this, [this](const QList<qint64> &positions, const QList<qint64> &entryuids) {
        Q_ASSERT(entryuids.size() == positions.size());
        for (int i = 0; i < entryuids.size(); i++) {
            qsizetype idx = m_entryOrder.indexOf(entryuids[i]);
            if (idx > -1 && m_entries[entryuids[i]].enclosureOrder.length() > 0) {
                m_entries[entryuids[i]].enclosures[m_entries[entryuids[i]].enclosureOrder[0]].playPosition = positions[i];
                Q_EMIT dataChanged(index(idx, 0), index(idx, 0), {AbstractEpisodeModel::Roles::PlayPositionRole});
            }
        }
    });
    connect(&DataManager::instance(), &DataManager::enclosureDurationsChanged, this, [this](const QList<qint64> &durations, const QList<qint64> &entryuids) {
        Q_ASSERT(entryuids.size() == durations.size());
        for (int i = 0; i < entryuids.size(); i++) {
            qsizetype idx = m_entryOrder.indexOf(entryuids[i]);
            if (idx > -1 && m_entries[entryuids[i]].enclosureOrder.length() > 0) {
                m_entries[entryuids[i]].enclosures[m_entries[entryuids[i]].enclosureOrder[0]].duration = durations[i];
                Q_EMIT dataChanged(index(idx, 0), index(idx, 0), {AbstractEpisodeModel::Roles::DurationRole});
            }
        }
    });
    connect(&DataManager::instance(), &DataManager::enclosureSizesChanged, this, [this](const QList<qint64> &sizes, const QList<qint64> &entryuids) {
        Q_ASSERT(entryuids.size() == sizes.size());
        for (int i = 0; i < entryuids.size(); i++) {
            qsizetype idx = m_entryOrder.indexOf(entryuids[i]);
            if (idx > -1 && m_entries[entryuids[i]].enclosureOrder.length() > 0) {
                m_entries[entryuids[i]].enclosures[m_entries[entryuids[i]].enclosureOrder[0]].size = sizes[i];
                Q_EMIT dataChanged(index(idx, 0), index(idx, 0), {AbstractEpisodeModel::Roles::SizeRole});
            }
        }
    });
    connect(&Fetcher::instance(), &Fetcher::enclosureDownloadProgress, this, [this](const qint64 entryuid, const qint64 amount) {
        qsizetype idx = m_entryOrder.indexOf(entryuid);
        if (idx > -1 && m_entries[m_entryOrder[idx]].enclosureOrder.length() > 0) {
            m_entries[m_entryOrder[idx]].enclosures[m_entries[m_entryOrder[idx]].enclosureOrder[0]].downloadSize = amount;
            Q_EMIT dataChanged(index(idx, 0), index(idx, 0), {AbstractEpisodeModel::Roles::DownloadSizeRole});
        }
    });

    updateInternalState();

    qCDebug(kastsObjects) << "AbstractEpisodeModel constructed" << this;
}

AbstractEpisodeModel::~AbstractEpisodeModel()
{
    qCDebug(kastsObjects) << "AbstractEpisodeModel destructed" << this;
}

QHash<int, QByteArray> AbstractEpisodeModel::roleNames() const
{
    return {
        {TitleRole, "title"},
        {EntryuidRole, "entryuid"},
        {IdRole, "id"},
        {QueueStatusRole, "queueStatus"},
        {ReadRole, "read"},
        {NewRole, "isNew"},
        {FavoriteRole, "favorite"},
        {RemovedRole, "removed"},
        {ContentRole, "content"},
        {CreatedRole, "created"},
        {UpdatedRole, "updated"},
        {LinkRole, "link"},
        {ImageRole, "image"},
        {HasEnclosureRole, "hasEnclosure"},
        {EnclosureUrlRole, "enclosureUrl"},
        {PlayPositionRole, "playPosition"},
        {DurationRole, "duration"},
        {SizeRole, "size"},
        {DownloadedRole, "downloaded"},
        {DownloadedOrderRole, "downloadedorder"},
        {DownloadSizeRole, "downloadSize"},
        {FeeduidRole, "feeduid"},
        {FeedNameRole, "feedName"},
        {FeedImageRole, "feedImage"},
    };
}

QVariant AbstractEpisodeModel::data(const QModelIndex &index, int role) const
{
    qCDebug(kastsEpisodeModel) << "calling data for row" << index << "and role" << roleNames()[role];

    if (!index.isValid()) {
        return QVariant();
    }

    switch (role) {
    case AbstractEpisodeModel::Roles::TitleRole:
        return QVariant::fromValue(m_entries[m_entryOrder[index.row()]].title);
    case AbstractEpisodeModel::Roles::EntryuidRole:
        return QVariant::fromValue(m_entryOrder[index.row()]);
    case AbstractEpisodeModel::Roles::IdRole:
        return QVariant::fromValue(m_entries[m_entryOrder[index.row()]].id);
    case AbstractEpisodeModel::Roles::QueueStatusRole:
        return QVariant::fromValue(QueueModel::instance().entryInQueue(m_entryOrder[index.row()]));
    case AbstractEpisodeModel::Roles::ReadRole:
        return QVariant::fromValue(m_entries[m_entryOrder[index.row()]].read);
    case AbstractEpisodeModel::Roles::NewRole:
        return QVariant::fromValue(m_entries[m_entryOrder[index.row()]].isNew);
    case AbstractEpisodeModel::Roles::FavoriteRole:
        return QVariant::fromValue(m_entries[m_entryOrder[index.row()]].favorite);
    case AbstractEpisodeModel::Roles::RemovedRole:
        return QVariant::fromValue(m_entries[m_entryOrder[index.row()]].removed);
    case AbstractEpisodeModel::Roles::ContentRole:
        return QVariant::fromValue(m_entries[m_entryOrder[index.row()]].content);
    case AbstractEpisodeModel::Roles::CreatedRole:
        return QVariant::fromValue(QDateTime::fromSecsSinceEpoch(m_entries[m_entryOrder[index.row()]].created));
    case AbstractEpisodeModel::Roles::UpdatedRole:
        return QVariant::fromValue(QDateTime::fromSecsSinceEpoch(m_entries[m_entryOrder[index.row()]].updated));
    case AbstractEpisodeModel::Roles::LinkRole:
        return QVariant::fromValue(m_entries[m_entryOrder[index.row()]].link);
    case AbstractEpisodeModel::Roles::ImageRole: {
        const DataTypes::EnclosureStatus enclosureStatus = m_entries[m_entryOrder[index.row()]].enclosureOrder.value(0).isEmpty()
            ? DataTypes::EnclosureStatus::NoEnclosure
            : m_entries[m_entryOrder[index.row()]].enclosures[m_entries[m_entryOrder[index.row()]].enclosureOrder.value(0)].downloaded;
        return QVariant::fromValue(EntryUtils::entryImage(m_entries[m_entryOrder[index.row()]].image,
                                                          m_feeds[m_entries[m_entryOrder[index.row()]].feeduid].image,
                                                          m_entries[m_entryOrder[index.row()]].enclosureOrder.value(0),
                                                          enclosureStatus,
                                                          m_entries[m_entryOrder[index.row()]].title,
                                                          m_feeds[m_entries[m_entryOrder[index.row()]].feeduid].dirname));
    }
    case AbstractEpisodeModel::Roles::HasEnclosureRole:
        return !m_entries[m_entryOrder[index.row()]].enclosureOrder.value(0).isEmpty();
    case AbstractEpisodeModel::Roles::EnclosureUrlRole:
        if (!m_entries[m_entryOrder[index.row()]].enclosureOrder.value(0).isEmpty()) {
            return QVariant::fromValue(m_entries[m_entryOrder[index.row()]].enclosures[m_entries[m_entryOrder[index.row()]].enclosureOrder.value(0)].url);
        } else {
            return QVariant();
        }
    case AbstractEpisodeModel::Roles::PlayPositionRole:
        if (!m_entries[m_entryOrder[index.row()]].enclosureOrder.value(0).isEmpty()) {
            return QVariant::fromValue(
                m_entries[m_entryOrder[index.row()]].enclosures[m_entries[m_entryOrder[index.row()]].enclosureOrder.value(0)].playPosition);
        } else {
            return QVariant();
        }
    case AbstractEpisodeModel::Roles::DurationRole:
        if (!m_entries[m_entryOrder[index.row()]].enclosureOrder.value(0).isEmpty()) {
            return QVariant::fromValue(m_entries[m_entryOrder[index.row()]].enclosures[m_entries[m_entryOrder[index.row()]].enclosureOrder.value(0)].duration);
        } else {
            return QVariant();
        }
    case AbstractEpisodeModel::Roles::SizeRole:
        if (!m_entries[m_entryOrder[index.row()]].enclosureOrder.value(0).isEmpty()) {
            return QVariant::fromValue(m_entries[m_entryOrder[index.row()]].enclosures[m_entries[m_entryOrder[index.row()]].enclosureOrder.value(0)].size);
        } else {
            return QVariant();
        }
    case AbstractEpisodeModel::Roles::DownloadedRole: {
        const DataTypes::EnclosureStatus enclosureStatus = m_entries[m_entryOrder[index.row()]].enclosureOrder.value(0).isEmpty()
            ? DataTypes::EnclosureStatus::NoEnclosure
            : m_entries[m_entryOrder[index.row()]].enclosures[m_entries[m_entryOrder[index.row()]].enclosureOrder.value(0)].downloaded;
        return QVariant::fromValue(enclosureStatus);
    }
    case AbstractEpisodeModel::Roles::DownloadedOrderRole: {
        const DataTypes::EnclosureStatus enclosureStatus = m_entries[m_entryOrder[index.row()]].enclosureOrder.value(0).isEmpty()
            ? DataTypes::EnclosureStatus::NoEnclosure
            : m_entries[m_entryOrder[index.row()]].enclosures[m_entries[m_entryOrder[index.row()]].enclosureOrder.value(0)].downloaded;
        return QVariant::fromValue(static_cast<int>(enclosureStatus));
    }
    case AbstractEpisodeModel::Roles::DownloadSizeRole:
        if (!m_entries[m_entryOrder[index.row()]].enclosureOrder.value(0).isEmpty()) {
            if (m_entries[m_entryOrder[index.row()]].enclosures[m_entries[m_entryOrder[index.row()]].enclosureOrder.value(0)].downloadSize < 0) {
                return QVariant::fromValue(EntryUtils::checkSizeOnDisk(
                    m_entryOrder[index.row()],
                    StorageManager::enclosurePath(
                        m_entries[m_entryOrder[index.row()]].title,
                        m_entries[m_entryOrder[index.row()]].enclosures[m_entries[m_entryOrder[index.row()]].enclosureOrder.value(0)].url,
                        m_feeds[m_entries[m_entryOrder[index.row()]].feeduid].dirname),
                    m_entries[m_entryOrder[index.row()]].enclosures[m_entries[m_entryOrder[index.row()]].enclosureOrder.value(0)].size));
            } else {
                return QVariant::fromValue(
                    m_entries[m_entryOrder[index.row()]].enclosures[m_entries[m_entryOrder[index.row()]].enclosureOrder.value(0)].downloadSize);
            }
        } else {
            return QVariant::fromValue(qint64(0));
        }
    case AbstractEpisodeModel::Roles::FeeduidRole:
        return QVariant::fromValue(m_entries[m_entryOrder[index.row()]].feeduid);
    case AbstractEpisodeModel::Roles::FeedNameRole:
        return QVariant::fromValue(m_feeds[m_entries[m_entryOrder[index.row()]].feeduid].name);
    case AbstractEpisodeModel::Roles::FeedImageRole:
        return QVariant::fromValue(m_feeds[m_entries[m_entryOrder[index.row()]].feeduid].image);
    default:
        return QVariant();
    }
}

int AbstractEpisodeModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_entryOrder.count();
}

void AbstractEpisodeModel::updateInternalState()
{
    m_entryOrder.clear();
    m_entries.clear();
    m_feeds.clear();

    QSqlQuery query;
    query.prepare(m_feedQuery);
    Database::instance().execute(query);
    while (query.next()) {
        DataTypes::FeedDetails feedDetails;
        feedDetails.feeduid = query.value(QStringLiteral("feeduid")).toLongLong();
        feedDetails.name = query.value(QStringLiteral("name")).toString();
        feedDetails.image = query.value(QStringLiteral("image")).toString();
        feedDetails.dirname = query.value(QStringLiteral("dirname")).toString();
        m_feeds[feedDetails.feeduid] = feedDetails;
    }
    query.finish();

    query.prepare(m_entryQuery);
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
        entryDetails.removed = query.value(QStringLiteral("removed")).toBool();
        entryDetails.link = query.value(QStringLiteral("link")).toString();
        entryDetails.hasEnclosure = query.value(QStringLiteral("hasEnclosure")).toBool();
        entryDetails.image = query.value(QStringLiteral("image")).toString();
        m_entries[entryDetails.entryuid] = entryDetails;
        m_entryOrder += entryDetails.entryuid;
    }
    query.finish();

    query.prepare(m_enclosureQuery);
    Database::instance().execute(query);
    while (query.next()) {
        DataTypes::EnclosureDetails enclosureDetails;
        qint64 entryuid = query.value(QStringLiteral("entryuid")).toLongLong();
        enclosureDetails.enclosureuid = query.value(QStringLiteral("enclosureuid")).toLongLong();
        enclosureDetails.type = query.value(QStringLiteral("type")).toString();
        enclosureDetails.duration = query.value(QStringLiteral("duration")).toLongLong();
        enclosureDetails.size = query.value(QStringLiteral("size")).toLongLong();
        enclosureDetails.downloadSize = -1;
        enclosureDetails.url = query.value(QStringLiteral("url")).toString();
        enclosureDetails.playPosition = query.value(QStringLiteral("playposition")).toLongLong();
        enclosureDetails.downloaded = DataTypes::dbToStatus(query.value(QStringLiteral("downloaded")).toInt());
        if (m_entries.contains(entryuid) && !m_entries[entryuid].enclosures.contains(enclosureDetails.url)
            && (enclosureDetails.type.contains(QStringLiteral("audio")) || enclosureDetails.type.contains(QStringLiteral("video")))) {
            m_entries[entryuid].enclosures[enclosureDetails.url] = enclosureDetails;
            m_entries[entryuid].enclosureOrder += enclosureDetails.url;
        }
    }
    query.finish();
}

void AbstractEpisodeModel::updateEntries(const QList<qint64> &entryuids)
{
    QSqlQuery query;

    query.prepare(QStringLiteral("SELECT * FROM Entries WHERE entryuid=:entryuid;"));
    for (const qint64 entryuid : std::as_const(entryuids)) {
        if (m_entryOrder.contains(entryuid) && m_entries.contains(entryuid)) {
            query.bindValue(QStringLiteral(":entryuid"), entryuid);
            Database::instance().execute(query);
            if (query.next()) {
                m_entries[entryuid].entryuid = query.value(QStringLiteral("entryuid")).toLongLong();
                m_entries[entryuid].feeduid = query.value(QStringLiteral("feeduid")).toLongLong();
                m_entries[entryuid].id = query.value(QStringLiteral("id")).toString();
                m_entries[entryuid].title = query.value(QStringLiteral("title")).toString();
                m_entries[entryuid].content = query.value(QStringLiteral("content")).toString();
                m_entries[entryuid].created = query.value(QStringLiteral("created")).toInt();
                m_entries[entryuid].updated = query.value(QStringLiteral("updated")).toInt();
                m_entries[entryuid].read = query.value(QStringLiteral("read")).toBool();
                m_entries[entryuid].isNew = query.value(QStringLiteral("new")).toBool();
                m_entries[entryuid].favorite = query.value(QStringLiteral("favorite")).toBool();
                m_entries[entryuid].removed = query.value(QStringLiteral("removed")).toBool();
                m_entries[entryuid].link = query.value(QStringLiteral("link")).toString();
                m_entries[entryuid].hasEnclosure = query.value(QStringLiteral("hasEnclosure")).toBool();
                m_entries[entryuid].image = query.value(QStringLiteral("image")).toString();
            }
        }
    }
    query.finish();

    query.prepare(QStringLiteral("SELECT * FROM Enclosures WHERE entryuid=:entryuid;"));
    for (const qint64 entryuid : std::as_const(entryuids)) {
        if (m_entryOrder.contains(entryuid) && m_entries.contains(entryuid)) {
            m_entries[entryuid].enclosures.clear();
            m_entries[entryuid].enclosureOrder.clear();
            query.bindValue(QStringLiteral(":entryuid"), entryuid);
            Database::instance().execute(query);
            while (query.next()) {
                DataTypes::EnclosureDetails enclosureDetails;
                enclosureDetails.enclosureuid = query.value(QStringLiteral("enclosureuid")).toLongLong();
                enclosureDetails.type = query.value(QStringLiteral("type")).toString();
                enclosureDetails.duration = query.value(QStringLiteral("duration")).toLongLong();
                enclosureDetails.size = query.value(QStringLiteral("size")).toLongLong();
                enclosureDetails.url = query.value(QStringLiteral("url")).toString();
                enclosureDetails.playPosition = query.value(QStringLiteral("playposition")).toLongLong();
                enclosureDetails.downloaded = DataTypes::dbToStatus(query.value(QStringLiteral("downloaded")).toInt());
                if (!m_entries[entryuid].enclosures.contains(enclosureDetails.url)
                    && (enclosureDetails.type.contains(QStringLiteral("audio")) || enclosureDetails.type.contains(QStringLiteral("video")))) {
                    m_entries[entryuid].enclosures[enclosureDetails.url] = enclosureDetails;
                    m_entries[entryuid].enclosureOrder += enclosureDetails.url;
                }
            }
        }
    }
    query.finish();
}

void AbstractEpisodeModel::updateFeeds(const QList<qint64> &feeduids)
{
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT feeduid, name, image, dirname FROM Feeds WHERE feeduid=:feeduid"));
    for (const qint64 feeduid : std::as_const(feeduids)) {
        if (m_feeds.contains(feeduid)) {
            query.bindValue(QStringLiteral(":feeduid"), feeduid);
            Database::instance().execute(query);
            while (query.next()) {
                m_feeds[feeduid].feeduid = query.value(QStringLiteral("feeduid")).toLongLong();
                m_feeds[feeduid].name = query.value(QStringLiteral("name")).toString();
                m_feeds[feeduid].image = query.value(QStringLiteral("image")).toString();
                m_feeds[feeduid].dirname = query.value(QStringLiteral("dirname")).toString();
            }
        }
    }
}
