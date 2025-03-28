/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "updatesyncrequest.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QStringList>
#include <QVector>

#include "synclogging.h"

UpdateSyncRequest::UpdateSyncRequest(SyncUtils::Provider provider, QNetworkReply *reply, QObject *parent)
    : GenericRequest(provider, reply, parent)
{
}

bool UpdateSyncRequest::success() const
{
    return m_success;
}

QVector<QStringList> UpdateSyncRequest::syncedDevices() const
{
    return m_syncedDevices;
}

QStringList UpdateSyncRequest::unsyncedDevices() const
{
    return m_unsyncedDevices;
}

void UpdateSyncRequest::processResults()
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
            const QJsonArray syncArray = data.object().value(QStringLiteral("synchronized")).toArray();
            for (const QJsonValue &jsonGroup : syncArray) {
                QStringList syncedGroup;
                const QJsonArray groupArray = jsonGroup.toArray();
                for (const QJsonValue &jsonDevice : groupArray) {
                    syncedGroup += jsonDevice.toString();
                }
                m_syncedDevices += syncedGroup;
            }
            const QJsonArray nonsyncArray = data.object().value(QStringLiteral("not-synchronized")).toArray();
            for (const QJsonValue &jsonDevice : nonsyncArray) {
                m_unsyncedDevices += jsonDevice.toString();
            }
            m_success = true;
        }
    }
    Q_EMIT finished();
    m_reply->deleteLater();
    deleteLater();
}
