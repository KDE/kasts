/**
 * SPDX-FileCopyrightText: 2026 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "networkaccessmanager.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

#include "kasts-version.h"

NetworkAccessManager::NetworkAccessManager(QObject *parent)
    : QObject(parent)
{
    m_manager = new QNetworkAccessManager(this);
    m_manager->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
    m_manager->setStrictTransportSecurityEnabled(true);
    // HACK TODO: Disable hstsstore temporarily because of malloc crash deep in
    // qt6 somewhere.  This is to be reenabled once the bug is solved upstream
    m_manager->enableStrictTransportSecurityStore(false);
}

QNetworkReply *NetworkAccessManager::get(QNetworkRequest &request) const
{
    setHeader(request);
    return m_manager->get(request);
}

QNetworkReply *NetworkAccessManager::post(QNetworkRequest &request, const QByteArray &data) const
{
    setHeader(request);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QLatin1String("application/json"));
    return m_manager->post(request, data);
}

QNetworkReply *NetworkAccessManager::head(QNetworkRequest &request) const
{
    setHeader(request);
    return m_manager->head(request);
}

void NetworkAccessManager::setHeader(QNetworkRequest &request) const
{
    request.setRawHeader(QByteArray("User-Agent"), QByteArray("Kasts/") + QByteArray(KASTS_VERSION_STRING) + QByteArray(" Syndication"));
}
