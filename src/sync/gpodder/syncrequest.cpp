/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "syncrequest.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QStringList>
#include <QVector>

#include "synclogging.h"

SyncRequest::SyncRequest(SyncUtils::Provider provider, QNetworkReply *reply, QObject *parent)
    : GenericRequest(provider, reply, parent)
{
}

QVector<QStringList> SyncRequest::syncedDevices() const
{
    return m_syncedDevices;
}

QStringList SyncRequest::unsyncedDevices() const
{
    return m_unsyncedDevices;
}

void SyncRequest::processResults()
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
        } else if (!m_abort) {
            for (auto jsonGroup : data.object().value(QStringLiteral("synchronized")).toArray()) {
                QStringList syncedGroup;
                for (auto jsonDevice : jsonGroup.toArray()) {
                    syncedGroup += jsonDevice.toString();
                }
                m_syncedDevices += syncedGroup;
            }
            for (auto jsonDevice : data.object().value(QStringLiteral("not-synchronized")).toArray()) {
                m_unsyncedDevices += jsonDevice.toString();
            }
        }
    }
    Q_EMIT finished();
    m_reply->deleteLater();
    deleteLater();
}
