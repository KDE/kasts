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
            Q_EMIT networkReachableChanged();
            Q_EMIT feedUpdatesAllowedChanged();
            Q_EMIT episodeDownloadsAllowedChanged();
            Q_EMIT imageDownloadsAllowedChanged();
            Q_EMIT streamingAllowedChanged();
        });
        connect(QNetworkInformation::instance(), &QNetworkInformation::isMeteredChanged, this, [this]() {
            Q_EMIT networkReachableChanged();
            Q_EMIT feedUpdatesAllowedChanged();
            Q_EMIT episodeDownloadsAllowedChanged();
            Q_EMIT imageDownloadsAllowedChanged();
            Q_EMIT streamingAllowedChanged();
        });
    }

    connect(SettingsManager::self(), &SettingsManager::checkNetworkStatusChanged, this, [this]() {
        Q_EMIT networkReachableChanged();
        Q_EMIT feedUpdatesAllowedChanged();
        Q_EMIT episodeDownloadsAllowedChanged();
        Q_EMIT imageDownloadsAllowedChanged();
        Q_EMIT streamingAllowedChanged();
    });

    connect(SettingsManager::self(), &SettingsManager::allowMeteredFeedUpdatesChanged, this, &NetworkConnectionManager::feedUpdatesAllowedChanged);
    connect(SettingsManager::self(), &SettingsManager::allowMeteredEpisodeDownloadsChanged, this, &NetworkConnectionManager::episodeDownloadsAllowedChanged);
    connect(SettingsManager::self(), &SettingsManager::allowMeteredImageDownloadsChanged, this, &NetworkConnectionManager::imageDownloadsAllowedChanged);
    connect(SettingsManager::self(), &SettingsManager::allowMeteredStreamingChanged, this, &NetworkConnectionManager::streamingAllowedChanged);
}

bool NetworkConnectionManager::networkReachable() const
{
    bool reachable = true;
    if (m_backendAvailable) {
        reachable = (QNetworkInformation::instance()->reachability() != QNetworkInformation::Reachability::Disconnected
                     || !SettingsManager::self()->checkNetworkStatus());
    }

    qCDebug(kastsNetworkConnectionManager) << "networkReachable()" << reachable;

    return reachable;
}

bool NetworkConnectionManager::feedUpdatesAllowed() const
{
    bool allowed = true;
    if (m_backendAvailable) {
        allowed = (networkReachable()
                   && (!QNetworkInformation::instance()->isMetered() || SettingsManager::self()->allowMeteredFeedUpdates()
                       || !SettingsManager::self()->checkNetworkStatus()));
    }

    qCDebug(kastsNetworkConnectionManager) << "FeedUpdatesAllowed()" << allowed;

    return allowed;
}

bool NetworkConnectionManager::episodeDownloadsAllowed() const
{
    bool allowed = true;
    if (m_backendAvailable) {
        allowed = (networkReachable()
                   && (!QNetworkInformation::instance()->isMetered() || SettingsManager::self()->allowMeteredEpisodeDownloads()
                       || !SettingsManager::self()->checkNetworkStatus()));
    }

    qCDebug(kastsNetworkConnectionManager) << "EpisodeDownloadsAllowed()" << allowed;

    return allowed;
}

bool NetworkConnectionManager::imageDownloadsAllowed() const
{
    bool allowed = true;
    if (m_backendAvailable) {
        allowed = (networkReachable()
                   && (!QNetworkInformation::instance()->isMetered() || SettingsManager::self()->allowMeteredImageDownloads()
                       || !SettingsManager::self()->checkNetworkStatus()));
    }

    qCDebug(kastsNetworkConnectionManager) << "ImageDownloadsAllowed()" << allowed;

    return allowed;
}

bool NetworkConnectionManager::streamingAllowed() const
{
    bool allowed = true;
    if (m_backendAvailable) {
        allowed = (networkReachable()
                   && (!QNetworkInformation::instance()->isMetered() || SettingsManager::self()->allowMeteredStreaming()
                       || !SettingsManager::self()->checkNetworkStatus()));
    }

    qCDebug(kastsNetworkConnectionManager) << "StreamingAllowed()" << allowed;

    return allowed;
}
