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

class EpisodeActionRequest : public GenericRequest
{
    Q_OBJECT

public:
    EpisodeActionRequest(SyncUtils::Provider provider, QNetworkReply *reply, QObject *parent);

    QVector<SyncUtils::EpisodeAction> episodeActions() const;
    qulonglong timestamp() const;

private:
    void processResults() override;

    QVector<SyncUtils::EpisodeAction> m_episodeActions;
    qulonglong m_timestamp = 0;
};
