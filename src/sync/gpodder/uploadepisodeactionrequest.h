/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <utility>

#include <QNetworkReply>
#include <QObject>
#include <QString>
#include <QVector>

#include "sync/gpodder/genericrequest.h"
#include "sync/syncutils.h"

class UploadEpisodeActionRequest : public GenericRequest
{
    Q_OBJECT

public:
    UploadEpisodeActionRequest(SyncUtils::Provider provider, QNetworkReply *reply, QObject *parent);

    QVector<std::pair<QString, QString>> updateUrls() const;
    qulonglong timestamp() const;

private:
    void processResults() override;

    QVector<std::pair<QString, QString>> m_updateUrls;
    qulonglong m_timestamp = 0;
};
