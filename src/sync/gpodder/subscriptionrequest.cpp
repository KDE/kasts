/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "subscriptionrequest.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QString>
#include <QStringList>
#include <QUrl>

#include "synclogging.h"

SubscriptionRequest::SubscriptionRequest(SyncUtils::Provider provider, QNetworkReply *reply, QObject *parent)
    : GenericRequest(provider, reply, parent)
{
}

QStringList SubscriptionRequest::addList() const
{
    return m_add;
}

QStringList SubscriptionRequest::removeList() const
{
    return m_remove;
}

qulonglong SubscriptionRequest::timestamp() const
{
    return m_timestamp;
}

void SubscriptionRequest::processResults()
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
        } else {
            for (auto jsonFeed : data.object().value(QStringLiteral("add")).toArray()) {
                m_add += cleanupUrl(jsonFeed.toString());
            }
            for (auto jsonFeed : data.object().value(QStringLiteral("remove")).toArray()) {
                m_remove += cleanupUrl(jsonFeed.toString());
            }
            m_timestamp = data.object().value(QStringLiteral("timestamp")).toInt();
        }
    }
    Q_EMIT finished();
    m_reply->deleteLater();
    deleteLater();
}
