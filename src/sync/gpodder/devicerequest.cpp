/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "devicerequest.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>

#include "synclogging.h"

DeviceRequest::DeviceRequest(SyncUtils::Provider provider, QNetworkReply *reply, QObject *parent)
    : GenericRequest(provider, reply, parent)
{
}

QVector<SyncUtils::Device> DeviceRequest::devices() const
{
    return m_devices;
}

void DeviceRequest::processResults()
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
            for (auto jsonDevice : data.array()) {
                SyncUtils::Device device;
                device.id = jsonDevice.toObject().value(QStringLiteral("id")).toString();
                device.caption = jsonDevice.toObject().value(QStringLiteral("caption")).toString();
                device.type = jsonDevice.toObject().value(QStringLiteral("type")).toString();
                device.subscriptions = jsonDevice.toObject().value(QStringLiteral("subscriptions")).toInt();

                m_devices += device;
            }
        }
    }
    Q_EMIT finished();
    m_reply->deleteLater();
    deleteLater();
}
