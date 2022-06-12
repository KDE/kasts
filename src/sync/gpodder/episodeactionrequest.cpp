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

                // Now we try to find the id for the entry based on either the GUID
                // or the episode download URL.  We also try to match with a percent-
                // decoded version of the URL.
                // There can be several hits (e.g. different entries pointing to the
                // same download URL; we add all of them to make sure everything's
                // consistent.
                // We also retrieve the feedUrl from the database to avoid problems with
                // different URLs pointing to the same feed (e.g. http vs https)
                QSqlQuery query;
                query.prepare(QStringLiteral("SELECT id, feed, url FROM Enclosures WHERE url=:url OR url=:decodeurl OR id=:id;"));
                query.bindValue(QStringLiteral(":url"), episodeAction.url);
                query.bindValue(QStringLiteral(":decodeurl"), cleanupUrl(episodeAction.url));
                query.bindValue(QStringLiteral(":id"), episodeAction.id);
                Database::instance().execute(query);
                if (!query.next()) {
                    qCDebug(kastsSync) << "cannot find episode with url:" << episodeAction.url;
                    continue;
                }
                do {
                    SyncUtils::EpisodeAction action = episodeAction;
                    action.id = query.value(QStringLiteral("id")).toString();
                    action.podcast = query.value(QStringLiteral("feed")).toString();
                    action.url = query.value(QStringLiteral("url")).toString();
                    m_episodeActions += action;
                } while (query.next());
            }
            m_timestamp = data.object().value(QStringLiteral("timestamp")).toInt();
        }
    }
    Q_EMIT finished();
    m_reply->deleteLater();
    deleteLater();
}
