/**
 * SPDX-FileCopyrightText: 2020 Tobias Fella <tobias.fella@kde.org>
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "enclosure.h"
#include "enclosurelogging.h"

#include <KLocalizedString>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QImage>
#include <QMimeDatabase>
#include <QNetworkReply>
#include <QSqlQuery>

#include <attachedpictureframe.h>
#include <fileref.h>
#include <id3v2frame.h>
#include <id3v2tag.h>
#include <mpegfile.h>

#include "audiomanager.h"
#include "database.h"
#include "datamanager.h"
#include "datatypes.h"
#include "entry.h"
#include "fetcher.h"
#include "objectslogging.h"
#include "utils/storagemanager.h"

Enclosure::Enclosure(Entry *entry)
    : QObject(entry)
    , m_entry(entry)
{
    connect(&Fetcher::instance(), &Fetcher::entriesUpdated, this, [this](const QList<qint64> &entryuids) {
        if (entryuids.contains(m_entryuid)) {
            updateFromDb();
        }
    });
    connect(&DataManager::instance(), &DataManager::entryPlayPositionsChanged, this, [this](const QList<qint64> &positions, const QList<qint64> &entryuids) {
        if (entryuids.contains(m_entryuid)) {
            qint64 index = entryuids.indexOf(m_entryuid);
            Q_ASSERT(index > -1);
            m_playposition = positions[index];
            Q_EMIT playPositionChanged();
        }
    });
    connect(&DataManager::instance(), &DataManager::enclosureDurationsChanged, this, [this](const QList<qint64> &durations, const QList<qint64> &entryuids) {
        if (entryuids.contains(m_entryuid)) {
            qint64 index = entryuids.indexOf(m_entryuid);
            Q_ASSERT(index > -1);
            m_duration = durations[index];
            Q_EMIT durationChanged();
        }
    });
    connect(&DataManager::instance(), &DataManager::enclosureSizesChanged, this, [this](const QList<qint64> &sizes, const QList<qint64> &entryuids) {
        if (entryuids.contains(m_entryuid)) {
            qint64 index = entryuids.indexOf(m_entryuid);
            Q_ASSERT(index > -1);
            m_size = sizes[index];
            Q_EMIT sizeChanged();
        }
    });
    connect(&DataManager::instance(),
            &DataManager::enclosureStatusesChanged,
            this,
            [this](const QList<DataTypes::EnclosureStatus> &statuses, const QList<qint64> &entryuids) {
                if (entryuids.contains(m_entryuid)) {
                    qint64 index = entryuids.indexOf(m_entryuid);
                    Q_ASSERT(index > -1);
                    m_status = statuses[index];
                    Q_EMIT statusChanged(m_entry, m_status);
                }
            });
    // This connection is there to keep other objects of the currently playing
    // enclosure in sync without writing the position to the DB (yet)
    connect(&AudioManager::instance(), &AudioManager::positionChanged, this, [this](const qint64 position, const qint64 entryuid) {
        if (entryuid == m_entryuid) {
            m_playposition = position;
            Q_EMIT playPositionChanged();
        }
    });

    // TODO: this will just take the first enclosure found; we should handle
    // multiple ones
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT * FROM Enclosures WHERE entryuid=:entryuid AND (type LIKE '%audio%' OR type LIKE '%video%')"));
    query.bindValue(QStringLiteral(":entryuid"), entry->entryuid());
    Database::instance().execute(query);

    if (!query.next()) {
        return;
    }

    m_entryuid = query.value(QStringLiteral("entryuid")).toLongLong();
    m_enclosureuid = query.value(QStringLiteral("enclosureuid")).toLongLong();
    m_duration = query.value(QStringLiteral("duration")).toInt();
    m_size = query.value(QStringLiteral("size")).toInt();
    m_type = query.value(QStringLiteral("type")).toString();
    m_url = query.value(QStringLiteral("url")).toString();
    m_playposition = query.value(QStringLiteral("playposition")).toLongLong();
    m_status = DataTypes::dbToStatus(query.value(QStringLiteral("downloaded")).toInt());
    m_playposition_dbsave = m_playposition;

    qCDebug(kastsObjects) << "Enclosure object" << m_enclosureuid << "constructed (corresponding entryuid is" << m_entryuid << ")";
}

Enclosure::~Enclosure()
{
    qCDebug(kastsObjects) << "Enclosure object" << m_enclosureuid << "destructed (corresponding entryuid is" << m_entryuid << ")";
}

void Enclosure::updateFromDb()
{
    // This method is used to update the most relevant fields from the RSS feed,
    // most notably the download URL.  It's deliberatly only updating the
    // duration and size if the URL has changed, since these values are
    // notably untrustworthy.  We generally get them from the files themselves
    // at the time they are downloaded.
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT * FROM Enclosures WHERE enclosureuid=:enclosureuid;"));
    query.bindValue(QStringLiteral(":enclosureuid"), m_enclosureuid);
    Database::instance().execute(query);

    while (query.next()) {
        return;
    }

    if (m_url != query.value(QStringLiteral("url")).toString() && m_status != DataTypes::EnclosureStatus::Downloaded) {
        // this means that the audio file has changed, or at least its location
        // let's only do something if the file isn't downloaded.
        // try to delete the file first (it actually shouldn't exist)
        DataManager::instance().bulkDownloadEnclosures(QList<qint64>({m_entryuid}));

        m_url = query.value(QStringLiteral("url")).toString();
        Q_EMIT urlChanged(m_url);
        Q_EMIT pathChanged(path());

        if (m_duration != query.value(QStringLiteral("duration")).toInt()) {
            m_duration = query.value(QStringLiteral("duration")).toInt();
            Q_EMIT durationChanged();
        }
        if (m_size != query.value(QStringLiteral("size")).toInt()) {
            m_size = query.value(QStringLiteral("size")).toInt();
            Q_EMIT sizeChanged();
        }
        if (m_type != query.value(QStringLiteral("type")).toString()) {
            m_type = query.value(QStringLiteral("type")).toString();
            Q_EMIT typeChanged(m_type);
        }
    }
}

qint64 Enclosure::enclosureuid() const
{
    return m_enclosureuid;
}

QString Enclosure::url() const
{
    return m_url;
}

QString Enclosure::path() const
{
    return StorageManager::enclosurePath(m_entry->title(), m_url, m_entry->feed()->dirname());
}

DataTypes::EnclosureStatus Enclosure::status() const
{
    return m_status;
}

qint64 Enclosure::playPosition() const
{
    return m_playposition;
}

qint64 Enclosure::duration() const
{
    return m_duration;
}

qint64 Enclosure::size() const
{
    return m_size;
}

void Enclosure::setPlayPosition(const qint64 &position)
{
    if (m_playposition != position) {
        m_playposition = position;
        DataManager::instance().bulkSetPlayPositions(QList<qint64>({position}), QList<qint64>({m_entryuid}));
        qCDebug(kastsEnclosure) << "save playPosition" << position << m_entry->title();
    }
}
