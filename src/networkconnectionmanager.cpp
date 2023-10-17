/**
 * SPDX-FileCopyrightText: 2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "networkconnectionmanager.h"
#include "networkconnectionmanagerlogging.h"

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QNetworkInformation>
#endif

#include "settingsmanager.h"

NetworkConnectionManager::NetworkConnectionManager(QObject *parent)
    : QObject(parent)
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    , m_networkStatus(SolidExtras::NetworkStatus())
#endif
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
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
#else
    connect(&m_networkStatus, &SolidExtras::NetworkStatus::connectivityChanged, this, [this]() {
        Q_EMIT feedUpdatesAllowedChanged();
        Q_EMIT episodeDownloadsAllowedChanged();
        Q_EMIT imageDownloadsAllowedChanged();
        Q_EMIT streamingAllowedChanged();
    });
    connect(&m_networkStatus, &SolidExtras::NetworkStatus::meteredChanged, this, [this]() {
        Q_EMIT feedUpdatesAllowedChanged();
        Q_EMIT episodeDownloadsAllowedChanged();
        Q_EMIT imageDownloadsAllowedChanged();
        Q_EMIT streamingAllowedChanged();
    });
#endif

    connect(SettingsManager::self(), &SettingsManager::allowMeteredFeedUpdatesChanged, this, &NetworkConnectionManager::feedUpdatesAllowedChanged);
    connect(SettingsManager::self(), &SettingsManager::allowMeteredEpisodeDownloadsChanged, this, &NetworkConnectionManager::episodeDownloadsAllowedChanged);
    connect(SettingsManager::self(), &SettingsManager::allowMeteredImageDownloadsChanged, this, &NetworkConnectionManager::imageDownloadsAllowedChanged);
    connect(SettingsManager::self(), &SettingsManager::allowMeteredStreamingChanged, this, &NetworkConnectionManager::streamingAllowedChanged);
}

bool NetworkConnectionManager::feedUpdatesAllowed() const
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    bool allowed = true;
    if (m_backendAvailable) {
        allowed = (!QNetworkInformation::instance()->isMetered() || SettingsManager::self()->allowMeteredFeedUpdates());
    }
#else
    bool allowed = (m_networkStatus.metered() != SolidExtras::NetworkStatus::Yes || SettingsManager::self()->allowMeteredFeedUpdates());
#endif

    qCDebug(kastsNetworkConnectionManager) << "FeedUpdatesAllowed()" << allowed;

    return allowed;
}

bool NetworkConnectionManager::episodeDownloadsAllowed() const
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    bool allowed = true;
    if (m_backendAvailable) {
        allowed = (!QNetworkInformation::instance()->isMetered() || SettingsManager::self()->allowMeteredEpisodeDownloads());
    }
#else
    bool allowed = (m_networkStatus.metered() != SolidExtras::NetworkStatus::Yes || SettingsManager::self()->allowMeteredEpisodeDownloads());
#endif

    qCDebug(kastsNetworkConnectionManager) << "EpisodeDownloadsAllowed()" << allowed;

    return allowed;
}

bool NetworkConnectionManager::imageDownloadsAllowed() const
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    bool allowed = true;
    if (m_backendAvailable) {
        allowed = (!QNetworkInformation::instance()->isMetered() || SettingsManager::self()->allowMeteredImageDownloads());
    }
#else
    bool allowed = (m_networkStatus.metered() != SolidExtras::NetworkStatus::Yes || SettingsManager::self()->allowMeteredImageDownloads());
#endif

    qCDebug(kastsNetworkConnectionManager) << "ImageDownloadsAllowed()" << allowed;

    return allowed;
}

bool NetworkConnectionManager::streamingAllowed() const
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    bool allowed = true;
    if (m_backendAvailable) {
        allowed = (!QNetworkInformation::instance()->isMetered() || SettingsManager::self()->allowMeteredStreaming());
    }
#else
    bool allowed = (m_networkStatus.metered() != SolidExtras::NetworkStatus::Yes || SettingsManager::self()->allowMeteredStreaming());
#endif

    qCDebug(kastsNetworkConnectionManager) << "StreamingAllowed()" << allowed;

    return allowed;
}
