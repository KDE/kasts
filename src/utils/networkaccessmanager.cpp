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
#include "objectslogging.h"
#include "utils/networkconnectionmanager.h"

NetworkAccessManager::NetworkAccessManager(NetworkAccessManager::RequestType defaultType, QObject *parent)
    : QNetworkAccessManager(parent)
    , m_defaultType(defaultType)
{
    this->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
    this->setStrictTransportSecurityEnabled(true);
    // TODO: Disable hstsstore temporarily because of malloc crash deep in
    // qt6 somewhere.  This is to be reenabled once the bug is solved upstream
    this->enableStrictTransportSecurityStore(false);
    this->setTransferTimeout();

    qCDebug(kastsObjects) << "NetworkAccessManager constructed" << this;
}

NetworkAccessManager::~NetworkAccessManager()
{
    qCDebug(kastsObjects) << "NetworkAccessManager destructed" << this;
}

void NetworkAccessManager::setRequestType(QNetworkRequest &request, RequestType type)
{
    request.setAttribute(QNetworkRequest::User, QVariant::fromValue(type));
}

QNetworkReply *NetworkAccessManager::createRequest(QNetworkAccessManager::Operation op, const QNetworkRequest &origRequest, QIODevice *outData)
{
    QNetworkRequest modifRequest(origRequest);

    // add a proper user agent to all request headers
    modifRequest.setRawHeader(QByteArray("User-Agent"), QByteArray("Kasts/") + QByteArray(KASTS_VERSION_STRING) + QByteArray(" Syndication"));

    // TODO: post is only used in Sync right now; it's probably better to move
    // it into Sync somewhere since it might interfere with future other uses
    // of post requests
    if (op == QNetworkAccessManager::PostOperation) {
        modifRequest.setHeader(QNetworkRequest::ContentTypeHeader, QLatin1String("application/json"));
    }

    // determine the type of network request this is, so we can decide whether
    // to allow it or not; this will fall back to the default if not specified
    NetworkAccessManager::RequestType type = m_defaultType;
    if (origRequest.attribute(QNetworkRequest::User).isValid()) {
        type = static_cast<NetworkAccessManager::RequestType>(origRequest.attribute(QNetworkRequest::User).toInt());
    }

    // now check if the request is allowed for the current type, and block it
    // if needed.  Note that blocking is done by only allowing access to the
    // cache
    if (type == NetworkAccessManager::ImageDownload && !NetworkConnectionManager::instance().imageDownloadsAllowed()) {
        modifRequest.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::CacheLoadControl::AlwaysCache);
    } else if (type == NetworkAccessManager::FeedUpdate && !NetworkConnectionManager::instance().feedUpdatesAllowed()) {
        modifRequest.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::CacheLoadControl::AlwaysCache);
    } else if (type == NetworkAccessManager::EpisodeDownload && !NetworkConnectionManager::instance().episodeDownloadsAllowed()) {
        modifRequest.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::CacheLoadControl::AlwaysCache);
    } else if (type == NetworkAccessManager::AudioStreaming && !NetworkConnectionManager::instance().streamingAllowed()) {
        modifRequest.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::CacheLoadControl::AlwaysCache);
    }
    QNetworkReply *reply = QNetworkAccessManager::createRequest(op, modifRequest, outData);

    return reply;
}
