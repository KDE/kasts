/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "loginrequest.h"

#include <QNetworkReply>

#include "synclogging.h"

LoginRequest::LoginRequest(SyncUtils::Provider provider, QNetworkReply *reply, QObject *parent)
    : GenericRequest(provider, reply, parent)
{
}

bool LoginRequest::success() const
{
    return m_success;
}

void LoginRequest::processResults()
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
