/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "genericrequest.h"

#include <QNetworkReply>
#include <QObject>

GenericRequest::GenericRequest(SyncUtils::Provider provider, QNetworkReply *reply, QObject *parent)
    : QObject(parent)
    , m_reply(reply)
    , m_provider(provider)
{
    connect(m_reply, &QNetworkReply::finished, this, &GenericRequest::processResults);
    connect(this, &GenericRequest::aborting, m_reply, &QNetworkReply::abort);
}

int GenericRequest::error() const
{
    return m_error;
}

QString GenericRequest::errorString() const
{
    return m_errorString;
}

bool GenericRequest::aborted() const
{
    return m_abort;
}

void GenericRequest::abort()
{
    m_abort = true;
    Q_EMIT aborting();
}

QString GenericRequest::cleanupUrl(const QString &url) const
{
    if (m_provider == SyncUtils::Provider::GPodderNet) {
        return QUrl::fromPercentEncoding(url.toUtf8());
    } else {
        return url;
    }
}
