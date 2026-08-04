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
    : QNetworkAccessManager(parent)
{
    this->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
    this->setStrictTransportSecurityEnabled(true);
    // HACK TODO: Disable hstsstore temporarily because of malloc crash deep in
    // qt6 somewhere.  This is to be reenabled once the bug is solved upstream
    this->enableStrictTransportSecurityStore(false);
}

QNetworkReply *NetworkAccessManager::get(QNetworkRequest &request)
{
    setHeader(request);
    return QNetworkAccessManager::get(request);
}

QNetworkReply *NetworkAccessManager::post(QNetworkRequest &request, const QByteArray &data)
{
    setHeader(request);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QLatin1String("application/json"));
    return QNetworkAccessManager::post(request, data);
}

QNetworkReply *NetworkAccessManager::head(QNetworkRequest &request)
{
    setHeader(request);
    return QNetworkAccessManager::head(request);
}

void NetworkAccessManager::setHeader(QNetworkRequest &request) const
{
    request.setRawHeader(QByteArray("User-Agent"), QByteArray("Kasts/") + QByteArray(KASTS_VERSION_STRING) + QByteArray(" Syndication"));
}
