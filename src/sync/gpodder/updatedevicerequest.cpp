/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "updatedevicerequest.h"

#include <QNetworkReply>

#include "sync/syncutils.h"
#include "synclogging.h"

UpdateDeviceRequest::UpdateDeviceRequest(SyncUtils::Provider provider, QNetworkReply *reply, QObject *parent)
    : GenericRequest(provider, reply, parent)
{
}

bool UpdateDeviceRequest::success() const
{
    return m_success;
}

void UpdateDeviceRequest::processResults()
{
    if (m_reply->error()) {
        m_error = m_reply->error();
        m_errorString = m_reply->errorString();
        qCDebug(kastsSync) << "m_reply error" << m_reply->errorString();
    } else if (!m_abort) {
        m_success = true;
    }
    Q_EMIT finished();
    m_reply->deleteLater();
    deleteLater();
}
