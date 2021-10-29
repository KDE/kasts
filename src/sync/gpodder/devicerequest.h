/**
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QNetworkReply>
#include <QObject>
#include <QVector>

#include "sync/gpodder/genericrequest.h"
#include "sync/syncutils.h"

class DeviceRequest : public GenericRequest
{
    Q_OBJECT

public:
    DeviceRequest(SyncUtils::Provider provider, QNetworkReply *reply, QObject *parent);

    QVector<SyncUtils::Device> devices() const;

private:
    void processResults() override;

    QVector<SyncUtils::Device> m_devices;
};
