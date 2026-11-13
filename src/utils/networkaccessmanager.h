/**
 * SPDX-FileCopyrightText: 2026 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QByteArray>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QObject>

class NetworkAccessManager : public QNetworkAccessManager
{
    Q_OBJECT

public:
    // enum holding the different type of network requests, this annotation will
    // be used to check whether this type of traffic is allowed on e.g. metered
    // connections
    enum RequestType {
        Invalid = -1,
        FeedUpdate = 0,
        EpisodeDownload,
        AudioStreaming,
        ImageDownload,
    };
    Q_ENUM(RequestType);

    NetworkAccessManager(RequestType defaultType, QObject *parent = nullptr);
    ~NetworkAccessManager();

    static void setRequestType(QNetworkRequest &request, RequestType type);

protected:
    virtual QNetworkReply *createRequest(QNetworkAccessManager::Operation op, const QNetworkRequest &origRequest, QIODevice *outData = nullptr) override;

    RequestType m_defaultType = RequestType::Invalid;
};
