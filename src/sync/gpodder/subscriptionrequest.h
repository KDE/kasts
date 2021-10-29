/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QNetworkReply>
#include <QObject>
#include <QStringList>

#include "sync/gpodder/genericrequest.h"
#include "sync/syncutils.h"

class SubscriptionRequest : public GenericRequest
{
    Q_OBJECT

public:
    SubscriptionRequest(SyncUtils::Provider provider, QNetworkReply *reply, QObject *parent);

    QStringList addList() const;
    QStringList removeList() const;
    qulonglong timestamp() const;

private:
    void processResults() override;

    QStringList m_add;
    QStringList m_remove;
    qulonglong m_timestamp = 0;
};
