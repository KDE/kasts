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

AbstractEpisodeModel::AbstractEpisodeModel(const QString &feedQuery, const QString &entryQuery, const QString &enclosureQuery, QObject *parent)
    : QAbstractListModel(parent)
    , m_feedQuery(feedQuery)
    , m_entryQuery(entryQuery)
    , m_enclosureQuery(enclosureQuery)
{
    // TODO: We should probably connect to feedUpdated to also get updates of e.g. the feed name?

    connect(&Fetcher::instance(), &Fetcher::entriesUpdated, this, [this](const QList<qint64> &entryuids) {
        updateEntries(entryuids);
        for (const qint64 entryuid : std::as_const(entryuids)) {
            qsizetype idx = m_entryOrder.indexOf(entryuid);
            if (idx > -1 && m_entries.contains(entryuid)) {
                Q_EMIT dataChanged(index(idx, 0), index(idx, 0), QList<int>(AbstractEpisodeModel::Roles::ReadRole));
            }
        }
    });
    // The following lambda slot function ensure that any changes signaled by
    // DataManager are also applied to the model data and propagated correctly.
    connect(&DataManager::instance(), &DataManager::entryReadStatusChanged, this, [this](bool state, const QList<qint64> &entryuids) {
        for (const qint64 entryuid : std::as_const(entryuids)) {
            qsizetype idx = m_entryOrder.indexOf(entryuid);
            if (idx > -1 && m_entries.contains(entryuid)) {
                m_entries[entryuid].read = state;
                Q_EMIT dataChanged(index(idx, 0), index(idx, 0), QList<int>(AbstractEpisodeModel::Roles::ReadRole));
            }
        }
    });
    connect(&DataManager::instance(), &DataManager::entryNewStatusChanged, this, [this](bool state, const QList<qint64> &entryuids) {
        for (const qint64 entryuid : std::as_const(entryuids)) {
            qsizetype idx = m_entryOrder.indexOf(entryuid);
            if (idx > -1 && m_entries.contains(entryuid)) {
                m_entries[entryuid].isNew = state;
                Q_EMIT dataChanged(index(idx, 0), index(idx, 0), QList<int>(AbstractEpisodeModel::Roles::NewRole));
            }
        }
    });
    connect(&DataManager::instance(), &DataManager::entryFavoriteStatusChanged, this, [this](bool state, const QList<qint64> &entryuids) {
        for (const qint64 entryuid : std::as_const(entryuids)) {
            qsizetype idx = m_entryOrder.indexOf(entryuid);
            if (idx > -1 && m_entries.contains(entryuid)) {
                m_entries[entryuid].favorite = state;
                Q_EMIT dataChanged(index(idx, 0), index(idx, 0), QList<int>(AbstractEpisodeModel::Roles::FavoriteRole));
            }
        }
    });
    connect(&DataManager::instance(),
            &DataManager::enclosureStatusesChanged,
            this,
            [this](const QList<Enclosure::Status> &statuses, const QList<qint64> &entryuids) {
                Q_ASSERT(entryuids.size() == statuses.size());
                for (int i = 0; i < entryuids.size(); i++) {
                    qsizetype idx = m_entryOrder.indexOf(entryuids[i]);
                    if (idx > -1 && m_entries[entryuids[i]].enclosureOrder.length() > 0) {
                        m_entries[entryuids[i]].enclosures[m_entries[entryuids[i]].enclosureOrder[0]].downloaded = statuses[i];
                        Q_EMIT dataChanged(index(idx, 0), index(idx, 0), QList<int>(AbstractEpisodeModel::Roles::DownloadedRole));
                    }
                }
            });
    // TODO: implement
    //  - queue status
    //  - playpositions
    //  - duration
    //  - size
    //  - image
    //  - other enclosure properties

    QTimer::singleShot(0, this, [this]() {
        beginResetModel();
        updateInternalState();
        endResetModel();
    });
}

QHash<int, QByteArray> AbstractEpisodeModel::roleNames() const
{
    return {
        {TitleRole, "title"},
        {EntryuidRole, "entryuid"},
        {EntryRole, "entry"},
        {IdRole, "id"},
        {ReadRole, "read"},
        {NewRole, "new"},
        {FavoriteRole, "favorite"},
        {ContentRole, "content"},
        {CreatedRole, "created"},
        {UpdatedRole, "updated"},
        {LinkRole, "link"},
        {DownloadedRole, "downloaded"},
        {DownloadedOrderRole, "downloadedorder"},
        {FeeduidRole, "feeduid"},
        {FeedNameRole, "feedname"},
    };
}

QVariant AbstractEpisodeModel::data(const QModelIndex &index, int role) const
{
    switch (role) {
    case AbstractEpisodeModel::Roles::TitleRole:
        return QVariant::fromValue(m_entries[m_entryOrder[index.row()]].title);
    case AbstractEpisodeModel::Roles::EntryuidRole:
        return QVariant::fromValue(m_entryOrder[index.row()]);
    case AbstractEpisodeModel::Roles::EntryRole:
        return QVariant::fromValue(DataManager::instance().getEntry(m_entries[m_entryOrder[index.row()]].entryuid));
    case AbstractEpisodeModel::Roles::IdRole:
        return QVariant::fromValue(m_entries[m_entryOrder[index.row()]].id);
    case AbstractEpisodeModel::Roles::ReadRole:
        return QVariant::fromValue(m_entries[m_entryOrder[index.row()]].read);
    case AbstractEpisodeModel::Roles::NewRole:
        return QVariant::fromValue(m_entries[m_entryOrder[index.row()]].isNew);
    case AbstractEpisodeModel::Roles::FavoriteRole:
        return QVariant::fromValue(m_entries[m_entryOrder[index.row()]].favorite);
    case AbstractEpisodeModel::Roles::ContentRole:
        return QVariant::fromValue(m_entries[m_entryOrder[index.row()]].content);
    case AbstractEpisodeModel::Roles::CreatedRole:
        return QVariant::fromValue(m_entries[m_entryOrder[index.row()]].created);
    case AbstractEpisodeModel::Roles::UpdatedRole:
        return QVariant::fromValue(m_entries[m_entryOrder[index.row()]].updated);
    case AbstractEpisodeModel::Roles::LinkRole:
        return QVariant::fromValue(m_entries[m_entryOrder[index.row()]].link);
    case AbstractEpisodeModel::Roles::DownloadedRole:
        if (m_entries[m_entryOrder[index.row()]].enclosureOrder.length() > 0) {
            return QVariant::fromValue(m_entries[m_entryOrder[index.row()]].enclosures[m_entries[m_entryOrder[index.row()]].enclosureOrder[0]].downloaded);
        } else {
            return QVariant::fromValue(Enclosure::Status::NoEnclosure);
        }
    case AbstractEpisodeModel::Roles::DownloadedOrderRole:
        if (m_entries[m_entryOrder[index.row()]].enclosureOrder.length() > 0) {
            return QVariant::fromValue(
                static_cast<int>(m_entries[m_entryOrder[index.row()]].enclosures[m_entries[m_entryOrder[index.row()]].enclosureOrder[0]].downloaded));
        } else {
            return QVariant::fromValue(static_cast<int>(Enclosure::Status::NoEnclosure));
        }
    case AbstractEpisodeModel::Roles::FeeduidRole:
        return QVariant::fromValue(m_entries[m_entryOrder[index.row()]].feeduid);
    case AbstractEpisodeModel::Roles::FeedNameRole:
        return QVariant::fromValue(m_feeds[m_entries[m_entryOrder[index.row()]].feeduid].name);
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
        enclosureDetails.duration = query.value(QStringLiteral("duration")).toLongLong();
        enclosureDetails.size = query.value(QStringLiteral("size")).toLongLong();
        enclosureDetails.url = query.value(QStringLiteral("url")).toString();
        enclosureDetails.playPosition = query.value(QStringLiteral("playposition")).toLongLong();
        enclosureDetails.downloaded = Enclosure::dbToStatus(query.value(QStringLiteral("downloaded")).toInt());
        if (m_entries.contains(entryuid) && !m_entries[entryuid].enclosures.contains(enclosureDetails.url)) {
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
                enclosureDetails.duration = query.value(QStringLiteral("duration")).toLongLong();
                enclosureDetails.size = query.value(QStringLiteral("size")).toLongLong();
                enclosureDetails.url = query.value(QStringLiteral("url")).toString();
                enclosureDetails.playPosition = query.value(QStringLiteral("playposition")).toLongLong();
                enclosureDetails.downloaded = Enclosure::dbToStatus(query.value(QStringLiteral("downloaded")).toInt());
                if (!m_entries[entryuid].enclosures.contains(enclosureDetails.url)) {
                    m_entries[entryuid].enclosures[enclosureDetails.url] = enclosureDetails;
                    m_entries[entryuid].enclosureOrder += enclosureDetails.url;
                }
            }
        }
    }
    query.finish();
}
