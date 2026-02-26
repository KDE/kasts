/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "episodeactionrequest.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QSqlQuery>
#include <QString>
#include <QVector>

#include "database.h"
#include "datamanager.h"
#include "synclogging.h"

EpisodeActionRequest::EpisodeActionRequest(SyncUtils::Provider provider, QNetworkReply *reply, QObject *parent)
    : GenericRequest(provider, reply, parent)
{
}

QList<SyncUtils::EpisodeAction> EpisodeActionRequest::episodeActions() const
{
    return m_episodeActions;
}

qulonglong EpisodeActionRequest::timestamp() const
{
    return m_timestamp;
}

void EpisodeActionRequest::processResults()
{
    if (m_reply->error()) {
        m_error = m_reply->error();
        m_errorString = m_reply->errorString();
        qCDebug(kastsSync) << "m_reply error" << m_reply->errorString();
    } else {
        QJsonParseError *error = nullptr;
        QJsonDocument data = QJsonDocument::fromJson(m_reply->readAll(), error);
        if (error) {
            qCDebug(kastsSync) << "parse error" << error->errorString();
            m_error = 1;
            m_errorString = error->errorString();
        } else if (!m_abort) {
            const QJsonArray array = data.object().value(QStringLiteral("actions")).toArray();
            QList<SyncUtils::EpisodeAction> episodeActions;
            QStringList ids, enclosureUrls;
            for (const QJsonValue &jsonAction : array) {
                SyncUtils::EpisodeAction episodeAction;
                episodeAction.id = jsonAction.toObject().value(QStringLiteral("guid")).toString();
                episodeAction.url = jsonAction.toObject().value(QStringLiteral("episode")).toString();
                episodeAction.podcast = cleanupUrl(jsonAction.toObject().value(QStringLiteral("podcast")).toString());
                episodeAction.device = jsonAction.toObject().value(QStringLiteral("device")).toString();
                episodeAction.action = jsonAction.toObject().value(QStringLiteral("action")).toString().toLower();
                if (episodeAction.action == QStringLiteral("play")) {
                    episodeAction.started = jsonAction.toObject().value(QStringLiteral("started")).toInt();
                    episodeAction.position = jsonAction.toObject().value(QStringLiteral("position")).toInt();
                    episodeAction.total = jsonAction.toObject().value(QStringLiteral("total")).toInt();
                } else {
                    episodeAction.started = 0;
                    episodeAction.position = 0;
                    episodeAction.total = 0;
                }
                QString actionTimestamp = jsonAction.toObject().value(QStringLiteral("timestamp")).toString();
                episodeAction.timestamp = static_cast<qulonglong>(
                    QDateTime::fromString(actionTimestamp.section(QStringLiteral("."), 0, 0), QStringLiteral("yyyy-MM-dd'T'hh:mm:ss")).toMSecsSinceEpoch()
                    / 1000);
                episodeActions += episodeAction;

                // We will use these lists to find the entryuids of these episodes later on
                ids += episodeAction.id;
                enclosureUrls += episodeAction.url;
            }
            Q_ASSERT(ids.count() == episodeActions.count());
            Q_ASSERT(ids.count() == enclosureUrls.count());

            // Fuzzy match ids and enclosure urls to find entryuids
            // There might be more than one result from each query, so we add
            // all of them to keep things consistent and to not lose any information
            QList<QList<qint64>> entryuids = DataManager::instance().findEntryuids(ids, enclosureUrls);
            Q_ASSERT(ids.count() == entryuids.count());

            // Now update the entryuid, enclosure url, feeduid and feed url
            for (qint64 i = 0; i < episodeActions.count(); ++i) {
                for (const qint64 entryuid : std::as_const(entryuids[i])) {
                    if (entryuid > 0) {
                        // Let's retrieve the info from the DB for the entryuids
                        QSqlQuery query;
                        query.prepare(
                            QStringLiteral("SELECT Entries.entryuid, Entries.feeduid, Entries.id, Enclosures.url, Feeds.url, Enclosures.duration "
                                           "FROM Enclosures "
                                           "    JOIN Entries ON Entries.entryuid=Enclosures.entryuid "
                                           "    JOIN Feeds ON Feeds.feeduid=Entries.feeduid "
                                           "WHERE "
                                           "    Enclosures.entryuid=:entryuid;"));
                        query.bindValue(QStringLiteral(":entryuid"), entryuid);
                        Database::instance().execute(query);
                        if (query.next()) {
                            SyncUtils::EpisodeAction action = episodeActions[i];
                            action.entryuid = query.value(QStringLiteral("Entries.entryuid")).toLongLong();
                            action.feeduid = query.value(QStringLiteral("Entries.feeduid")).toLongLong();
                            action.id = query.value(QStringLiteral("Entries.id")).toString();
                            action.podcast = query.value(QStringLiteral("Feeds.url")).toString();
                            action.url = query.value(QStringLiteral("Enclosures.url")).toString();
                            action.durationdb = query.value(QStringLiteral("Enclosures.duration")).toLongLong();
                            m_episodeActions += action;
                            qCDebug(kastsSync) << "Found matching entryuids" << entryuid << "for" << action.id;
                            Q_ASSERT(entryuid == action.entryuid);
                        }
                    }
                }
            }
            m_timestamp = data.object().value(QStringLiteral("timestamp")).toInt();
        }
    }
    Q_EMIT finished();
    m_reply->deleteLater();
    deleteLater();
}
