/**
 * SPDX-FileCopyrightText: 2024 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#pragma once

#include "mpvmediabackend.h"

#include <MpvAbstractItem>

#include <QQuickItem>
#include <QString>
#include <QTimer>
#include <QUrl>
#include <QVariant>

#include "kmediasession.h"

class MpvMediaBackendPrivate : public MpvAbstractItem
{
public:
    explicit MpvMediaBackendPrivate(QQuickItem *parent = nullptr);
    ~MpvMediaBackendPrivate() = default;

private:
    friend class MpvMediaBackend;

    void onFileLoaded();
    void onEndFile(const QString &reason);
    void parseMetaData();
    void onPropertyChanged(const QString &property, const QVariant &value);
    void timerUpdate();

    KMediaSession *m_kmediaSession = nullptr;
    MpvMediaBackend *m_parent = nullptr;

    const qint64 m_notifyInterval = 500; // interval for position updates (in ms)

    QTimer *m_timer = nullptr;
    QUrl m_source;
    qreal m_volume = 100.0;
    qint64 m_position = 0;
    qint64 m_duration = 0;
    bool m_isMuted = false;
    bool m_isSeekable = false;
    qreal m_playbackRate = 1.0;
    KMediaSession::PlaybackState m_playerState = KMediaSession::StoppedState;
    KMediaSession::MediaStatus m_mediaStatus = KMediaSession::NoMedia;
    KMediaSession::Error m_error = KMediaSession::NoError;

    qreal m_newVolume = 100.0;
    bool m_newIsMuted = false;
};
