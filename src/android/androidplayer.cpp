/*
   SPDX-FileCopyrightText: 2025 (c) Pedro Nishiyama <nishiyama.v3@gmail.com>

   SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "androidplayer.h"
#include "mediaplaylist.h"

#include <QObject>
#include <QString>

AndroidPlayer::AndroidPlayer(QObject *parent)
    : QObject(parent)
{
}

AndroidPlayer::~AndroidPlayer() = default;

AudioManager *AndroidPlayer::audioManager() const
{
    return m_audiomanager;
}

void AndroidPlayer::setAudioManager(AudioManager *manageAudioPlayer)
{
    if (m_audiomanager == manageAudioPlayer) {
        return;
    }

    m_audiomanager = manageAudioPlayer;

    Q_EMIT audioManagerChanged();
}

void AndroidPlayer::initialize()
{
    if (!m_audiomanager) {
        return;
    }

    connect(m_audiomanager, &AudioManager::seek, this, &AndroidPlayer::playerPositionChanged);
    connect(m_audiomanager, &AudioManager::statusChanged, this, &AndroidPlayer::playerStatusChanged);
    connect(m_audiomanager, &AudioManager::playbackStateChanged, this, &AndroidPlayer::playerPlaybackStateChanged);

    connect(m_androidplayerjni, &AndroidPlayerJni::Next, this, &AndroidPlayer::next);
    // connect(m_androidplayerjni, &AndroidPlayerJni::Previous, this, &AndroidPlayer::previous);
    connect(m_androidplayerjni, &AndroidPlayerJni::PlayPause, this, &AndroidPlayer::playPause);
    connect(m_androidplayerjni, &AndroidPlayerJni::Stop, this, &AndroidPlayer::stop);
    connect(m_androidplayerjni, &AndroidPlayerJni::Seek, this, &AndroidPlayer::seek);
}

void AndroidPlayer::playerStatusChanged()
{
    if (m_audiomanager->status() != KMediaSession::LoadedMedia)
        return;

    QVariantMap metadata;

    if (m_audiomanager->entryuid() > 0 && m_audiomanager->entry()) {
        Entry *currentEntry = m_audiomanager->entry();

        metadata[QStringLiteral("title")] = currentEntry->title();
        metadata[QStringLiteral("artist")] = currentEntry->authors()[0];
        metadata[QStringLiteral("albumName")] = currentEntry->feed()->name();
        metadata[QStringLiteral("albumCover")] = currentEntry->cachedImage();
        metadata[QStringLiteral("duration")] = currentEntry->enclosure()->duration();
    } else {
    }

    AndroidPlayerJni::propertyChanged(QStringLiteral("Metadata"), metadata);
}

void AndroidPlayer::playerPlaybackStateChanged()
{
    AndroidPlayerJni::propertyChanged(QStringLiteral("PlaybackState"), m_audiomanager->playbackState());
}

void AndroidPlayer::playerPositionChanged(qint64 position)
{
    AndroidPlayerJni::propertyChanged(QStringLiteral("Position"), position);
}

void AndroidPlayer::next()
{
    m_audiomanager->next();
}

// TODO: remove previous and add skip actions instead
void AndroidPlayer::previous()
{
    // m_audiomanager->skipPreviousTrack(m_audiomanager->playerPosition());
    playerPositionChanged(m_audiomanager->position());
}

void AndroidPlayer::playPause()
{
    m_audiomanager->playPause();
    playerPositionChanged(m_audiomanager->position());
}

void AndroidPlayer::stop()
{
    m_audiomanager->stop();
}

void AndroidPlayer::seek(qlonglong offset)
{
    m_audiomanager->seek(int(offset));
}
