/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QNetworkReply>
#include <QObject>
#include <QPair>
#include <QString>
#include <QVector>

#include "sync/gpodder/genericrequest.h"
#include "sync/syncutils.h"

class UploadSubscriptionRequest : public GenericRequest
{
    Q_OBJECT

public:
    UploadSubscriptionRequest(SyncUtils::Provider provider, QNetworkReply *reply, QObject *parent);

    QVector<QPair<QString, QString>> updateUrls() const;
    qulonglong timestamp() const;

private:
    void processResults() override;

    QVector<QPair<QString, QString>> m_updateUrls;
    qulonglong m_timestamp = 0;
};
