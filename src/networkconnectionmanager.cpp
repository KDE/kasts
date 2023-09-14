/**
 * SPDX-FileCopyrightText: 2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "networkconnectionmanager.h"
#include "networkconnectionmanagerlogging.h"

#include <QNetworkInformation>

#include "settingsmanager.h"

NetworkConnectionManager::NetworkConnectionManager(QObject *parent)
    : QObject(parent)
{
    m_backendAvailable = QNetworkInformation::loadDefaultBackend();

    if (m_backendAvailable) {
        connect(QNetworkInformation::instance(), &QNetworkInformation::reachabilityChanged, this, [this]() {
            Q_EMIT feedUpdatesAllowedChanged();
            Q_EMIT episodeDownloadsAllowedChanged();
            Q_EMIT imageDownloadsAllowedChanged();
            Q_EMIT streamingAllowedChanged();
        });
        connect(QNetworkInformation::instance(), &QNetworkInformation::isMeteredChanged, this, [this]() {
            Q_EMIT feedUpdatesAllowedChanged();
            Q_EMIT episodeDownloadsAllowedChanged();
            Q_EMIT imageDownloadsAllowedChanged();
            Q_EMIT streamingAllowedChanged();
        });
    }

    connect(SettingsManager::self(), &SettingsManager::allowMeteredFeedUpdatesChanged, this, &NetworkConnectionManager::feedUpdatesAllowedChanged);
    connect(SettingsManager::self(), &SettingsManager::allowMeteredEpisodeDownloadsChanged, this, &NetworkConnectionManager::episodeDownloadsAllowedChanged);
    connect(SettingsManager::self(), &SettingsManager::allowMeteredImageDownloadsChanged, this, &NetworkConnectionManager::imageDownloadsAllowedChanged);
    connect(SettingsManager::self(), &SettingsManager::allowMeteredStreamingChanged, this, &NetworkConnectionManager::streamingAllowedChanged);
}

bool NetworkConnectionManager::feedUpdatesAllowed() const
{
    bool allowed = true;
    if (m_backendAvailable) {
        allowed = (QNetworkInformation::instance()->reachability() != QNetworkInformation::Reachability::Disconnected
                   && (!QNetworkInformation::instance()->isMetered() || SettingsManager::self()->allowMeteredFeedUpdates()));
    }

    qCDebug(kastsNetworkConnectionManager) << "FeedUpdatesAllowed()" << allowed;

    return allowed;
}

bool NetworkConnectionManager::episodeDownloadsAllowed() const
{
    bool allowed = true;
    if (m_backendAvailable) {
        allowed = (QNetworkInformation::instance()->reachability() != QNetworkInformation::Reachability::Disconnected
                   && (!QNetworkInformation::instance()->isMetered() || SettingsManager::self()->allowMeteredEpisodeDownloads()));
    }

    qCDebug(kastsNetworkConnectionManager) << "EpisodeDownloadsAllowed()" << allowed;

    return allowed;
}

bool NetworkConnectionManager::imageDownloadsAllowed() const
{
    bool allowed = true;
    if (m_backendAvailable) {
        allowed = (QNetworkInformation::instance()->reachability() != QNetworkInformation::Reachability::Disconnected
                   && (!QNetworkInformation::instance()->isMetered() || SettingsManager::self()->allowMeteredImageDownloads()));
    }

    qCDebug(kastsNetworkConnectionManager) << "ImageDownloadsAllowed()" << allowed;

    return allowed;
}

bool NetworkConnectionManager::streamingAllowed() const
{
    bool allowed = true;
    if (m_backendAvailable) {
        allowed = (QNetworkInformation::instance()->reachability() != QNetworkInformation::Reachability::Disconnected
                   && (!QNetworkInformation::instance()->isMetered() || SettingsManager::self()->allowMeteredStreaming()));
    }

    qCDebug(kastsNetworkConnectionManager) << "StreamingAllowed()" << allowed;

    return allowed;
}
