/**
 * SPDX-FileCopyrightText: 2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QObject>

class NetworkConnectionManager : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool networkReachable READ networkReachable NOTIFY networkReachableChanged)
    Q_PROPERTY(bool feedUpdatesAllowed READ feedUpdatesAllowed NOTIFY feedUpdatesAllowedChanged)
    Q_PROPERTY(bool episodeDownloadsAllowed READ episodeDownloadsAllowed NOTIFY episodeDownloadsAllowedChanged)
    Q_PROPERTY(bool imageDownloadsAllowed READ imageDownloadsAllowed NOTIFY imageDownloadsAllowedChanged)
    Q_PROPERTY(bool streamingAllowed READ streamingAllowed NOTIFY streamingAllowedChanged)

public:
    static NetworkConnectionManager &instance()
    {
        static NetworkConnectionManager _instance;
        return _instance;
    }

    [[nodiscard]] bool networkReachable() const;
    [[nodiscard]] bool feedUpdatesAllowed() const;
    [[nodiscard]] bool episodeDownloadsAllowed() const;
    [[nodiscard]] bool imageDownloadsAllowed() const;
    [[nodiscard]] bool streamingAllowed() const;

Q_SIGNALS:
    void networkReachableChanged();
    void feedUpdatesAllowedChanged();
    void episodeDownloadsAllowedChanged();
    void imageDownloadsAllowedChanged();
    void streamingAllowedChanged();

private:
    NetworkConnectionManager(QObject *parent = nullptr);

    bool m_backendAvailable = false;
};
