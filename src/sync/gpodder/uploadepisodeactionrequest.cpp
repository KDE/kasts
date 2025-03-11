/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "uploadepisodeactionrequest.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QString>
#include <QUrl>
#include <QVector>

#include "synclogging.h"

UploadEpisodeActionRequest::UploadEpisodeActionRequest(SyncUtils::Provider provider, QNetworkReply *reply, QObject *parent)
    : GenericRequest(provider, reply, parent)
{
}

QVector<std::pair<QString, QString>> UploadEpisodeActionRequest::updateUrls() const
{
    return m_updateUrls;
}

qulonglong UploadEpisodeActionRequest::timestamp() const
{
    return m_timestamp;
}

void UploadEpisodeActionRequest::processResults()
{
    if (m_reply->error()) {
        m_error = m_reply->error();
        m_errorString = m_reply->errorString();
        qCDebug(kastsSync) << "m_reply error" << m_reply->errorString();
    } else if (!m_abort) {
        QJsonParseError *error = nullptr;
        QJsonDocument data = QJsonDocument::fromJson(m_reply->readAll(), error);
        if (error) {
            qCDebug(kastsSync) << "parse error" << error->errorString();
            m_error = 1;
            m_errorString = error->errorString();
        } else {
            const QJsonArray urlArray = data.object().value(QStringLiteral("update_urls")).toArray();
            for (const QJsonValue &jsonFeed : urlArray) {
                m_updateUrls += std::pair<QString, QString>(jsonFeed.toArray().at(0).toString(), jsonFeed.toArray().at(1).toString());
            }
            m_timestamp = data.object().value(QStringLiteral("timestamp")).toInt();
        }
    }
    Q_EMIT finished();
    m_reply->deleteLater();
    deleteLater();
}
