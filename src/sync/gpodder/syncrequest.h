/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QNetworkReply>
#include <QObject>
#include <QStringList>
#include <QVector>

#include "sync/gpodder/genericrequest.h"
#include "sync/syncutils.h"

class SyncRequest : public GenericRequest
{
    Q_OBJECT

public:
    SyncRequest(SyncUtils::Provider provider, QNetworkReply *reply, QObject *parent);

    QVector<QStringList> syncedDevices() const;
    QStringList unsyncedDevices() const;

private:
    void processResults() override;

    QVector<QStringList> m_syncedDevices;
    QStringList m_unsyncedDevices;
};
