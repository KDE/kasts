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
#include "synclogging.h"

EpisodeActionRequest::EpisodeActionRequest(SyncUtils::Provider provider, QNetworkReply *reply, QObject *parent)
    : GenericRequest(provider, reply, parent)
{
}

QVector<SyncUtils::EpisodeAction> EpisodeActionRequest::episodeActions() const
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
            for (const auto &jsonAction : data.object().value(QStringLiteral("actions")).toArray()) {
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

                // Finally we try to find the id for the entry based on the episode URL if
                // no GUID is available.
                // We also retrieve the feedUrl from the database to avoid problems with
                // different URLs pointing to the same feed (e.g. http vs https)
                if (!episodeAction.id.isEmpty()) {
                    // First check if the GUID we got from the service is in the DB
                    QSqlQuery query;
                    query.prepare(QStringLiteral("SELECT id, feed FROM Entries WHERE id=:id;"));
                    query.bindValue(QStringLiteral(":id"), episodeAction.id);
                    Database::instance().execute(query);
                    if (!query.next()) {
                        qCDebug(kastsSync) << "cannot find episode with id:" << episodeAction.id;
                        episodeAction.id.clear();
                    } else {
                        episodeAction.podcast = query.value(QStringLiteral("feed")).toString();
                    }
                }

                if (episodeAction.id.isEmpty()) {
                    // There either was no GUID specified or we couldn't find it in the DB
                    // Try to find the episode based on the enclosure URL
                    QSqlQuery query;
                    query.prepare(QStringLiteral("SELECT id, feed FROM Enclosures WHERE url=:url;"));
                    query.bindValue(QStringLiteral(":url"), episodeAction.url);
                    Database::instance().execute(query);
                    if (query.next()) {
                        episodeAction.id = query.value(QStringLiteral("id")).toString();
                        episodeAction.podcast = query.value(QStringLiteral("feed")).toString();
                    } else {
                        // try again with percent DEcoded URL
                        QSqlQuery query;
                        query.prepare(QStringLiteral("SELECT id, feed FROM Enclosures WHERE url=:url;"));
                        query.bindValue(QStringLiteral(":url"), cleanupUrl(episodeAction.url));
                        Database::instance().execute(query);
                        if (query.next()) {
                            episodeAction.url = cleanupUrl(episodeAction.url);
                            episodeAction.id = query.value(QStringLiteral("id")).toString();
                            episodeAction.podcast = query.value(QStringLiteral("feed")).toString();
                        } else {
                            qCDebug(kastsSync) << "cannot find episode with url:" << episodeAction.url;
                            continue;
                        }
                    }
                }
                m_episodeActions += episodeAction;
            }
            m_timestamp = data.object().value(QStringLiteral("timestamp")).toInt();
        }
    }
    Q_EMIT finished();
    m_reply->deleteLater();
    deleteLater();
}
