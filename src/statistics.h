/**
 * SPDX-FileCopyrightText: 2026 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QObject>
#include <QQmlEngine>

#include "audiomanager.h"

class Statistics : public QObject
{
    Q_OBJECT

public:
    Statistics(AudioManager *audioManager);
    ~Statistics();

private:
    void logEvent(const KMediaSession::PlaybackState state);

    AudioManager *m_audioManager;
    KMediaSession::PlaybackState m_state = KMediaSession::PlaybackState::StoppedState;
    qint64 m_entryuid = 0;
    std::chrono::time_point<std::chrono::steady_clock> m_start;
};
