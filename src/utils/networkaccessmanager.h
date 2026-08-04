/**
 * SPDX-FileCopyrightText: 2026 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QByteArray>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QObject>

class NetworkAccessManager : public QNetworkAccessManager
{
    Q_OBJECT

public:
    NetworkAccessManager(QObject *parent = nullptr);

    QNetworkReply *get(QNetworkRequest &request);
    QNetworkReply *post(QNetworkRequest &request, const QByteArray &data);
    QNetworkReply *head(QNetworkRequest &request);

private:
    void setHeader(QNetworkRequest &request) const;
};
