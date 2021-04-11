/**
 * SPDX-FileCopyrightText: 2017 (c) Matthieu Gallien <matthieu_gallien@yahoo.fr>
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "audiomanager.h"
#include "powermanagementinterface.h"

#include <QTimer>
#include <QAudio>

class AudioManagerPrivate
{

private:

    PowerManagementInterface mPowerInterface;

    QMediaPlayer m_player;

    Entry* m_entry = nullptr;
    bool playerOpen = false;

    friend class AudioManager;
};

AudioManager::AudioManager(QObject *parent) : QObject(parent), d(std::make_unique<AudioManagerPrivate>())
{
    connect(&d->m_player, &QMediaPlayer::mutedChanged, this, &AudioManager::playerMutedChanged);
    connect(&d->m_player, &QMediaPlayer::volumeChanged, this, &AudioManager::playerVolumeChanged);
    //connect(&d->m_player, &QMediaPlayer::mediaChanged, this, &AudioManager::sourceChanged);
    connect(&d->m_player, &QMediaPlayer::mediaStatusChanged, this, &AudioManager::statusChanged);
    connect(&d->m_player, &QMediaPlayer::mediaStatusChanged, this, &AudioManager::mediaStatusChanged);
    connect(&d->m_player, &QMediaPlayer::stateChanged, this, &AudioManager::playbackStateChanged);
    connect(&d->m_player, &QMediaPlayer::stateChanged, this, &AudioManager::playerStateChanged);
    connect(&d->m_player, &QMediaPlayer::playbackRateChanged, this, &AudioManager::playbackRateChanged);
    connect(&d->m_player, QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error), this, &AudioManager::errorChanged);
    connect(&d->m_player, &QMediaPlayer::durationChanged, this, &AudioManager::durationChanged);
    connect(&d->m_player, &QMediaPlayer::positionChanged, this, &AudioManager::positionChanged);
    connect(&d->m_player, &QMediaPlayer::seekableChanged, this, &AudioManager::seekableChanged);
}

AudioManager::~AudioManager()
{
    d->mPowerInterface.setPreventSleep(false);
}

Entry* AudioManager::entry () const
{
    return d->m_entry;
}

bool AudioManager::playerOpen() const
{
    return d->playerOpen;
}

bool AudioManager::muted() const
{
    return d->m_player.isMuted();
}

qreal AudioManager::volume() const
{
    auto realVolume = static_cast<qreal>(d->m_player.volume() / 100.0);
    auto userVolume = static_cast<qreal>(QAudio::convertVolume(realVolume, QAudio::LinearVolumeScale, QAudio::LogarithmicVolumeScale));

    return userVolume * 100.0;
}

/*
QUrl AudioManager::source() const
{
    return d->m_player.media().request().url();
}
*/

QMediaPlayer::Error AudioManager::error() const
{
    if (d->m_player.error() != QMediaPlayer::NoError) {
        qDebug() << "AudioManager::error" << d->m_player.errorString();
    }

    return d->m_player.error();
}

qint64 AudioManager::duration() const
{
    return d->m_player.duration();
}

qint64 AudioManager::position() const
{
    return d->m_player.position();
}

bool AudioManager::seekable() const
{
    return d->m_player.isSeekable();
}

QMediaPlayer::State AudioManager::playbackState() const
{
    return d->m_player.state();
}

qreal AudioManager::playbackRate() const
{
    return d->m_player.playbackRate();
}

QMediaPlayer::MediaStatus AudioManager::status() const
{
    return d->m_player.mediaStatus();
}

void AudioManager::setEntry(Entry* entry)
{
    if (entry != nullptr) {
        d->m_entry = entry;
        Q_EMIT entryChanged(entry);
    }
}

void AudioManager::setPlayerOpen(bool state)
{
    d->playerOpen = state;
    Q_EMIT playerOpenChanged(state);
}

void AudioManager::setMuted(bool muted)
{
    d->m_player.setMuted(muted);
}

void AudioManager::setVolume(qreal volume)
{
    qDebug() << "AudioManager::setVolume" << volume;

    auto realVolume = static_cast<qreal>(QAudio::convertVolume(volume / 100.0, QAudio::LogarithmicVolumeScale, QAudio::LinearVolumeScale));
    d->m_player.setVolume(qRound(realVolume * 100));
}

/*
void AudioManager::setSource(const QUrl &source)
{
    qDebug() << "AudioManager::setSource" << source;

    d->m_player.setMedia({source});
}
*/

void AudioManager::setPlaybackRate(const qreal rate)
{
    qDebug() << "AudioManager::setPlaybackRate" << rate;

    d->m_player.setPlaybackRate(rate);
}

void AudioManager::setPosition(qint64 position)
{
    qDebug() << "AudioManager::setPosition" << position;

    d->m_player.setPosition(position);
}

void AudioManager::play()
{
    qDebug() << "AudioManager::play";

    d->m_player.play();
}

void AudioManager::pause()
{
    qDebug() << "AudioManager::pause";

    d->m_player.pause();
}

void AudioManager::stop()
{
    qDebug() << "AudioManager::stop";

    d->m_player.stop();
}

void AudioManager::seek(qint64 position)
{
    qDebug() << "AudioManager::seek" << position;

    d->m_player.setPosition(position);
}

void AudioManager::mediaStatusChanged()
{
    qDebug() << "AudioManager::mediaStatusChanged" << d->m_player.mediaStatus();
}

void AudioManager::playerStateChanged()
{
    qDebug() << "AudioManager::playerStateChanged" << d->m_player.state();

    switch(d->m_player.state())
    {
    case QMediaPlayer::State::StoppedState:
        Q_EMIT stopped();
        d->mPowerInterface.setPreventSleep(false);
        break;
    case QMediaPlayer::State::PlayingState:
        Q_EMIT playing();
        d->mPowerInterface.setPreventSleep(true);
        break;
    case QMediaPlayer::State::PausedState:
        Q_EMIT paused();
        d->mPowerInterface.setPreventSleep(false);
        break;
    }
}

void AudioManager::playerVolumeChanged()
{
    qDebug() << "AudioManager::playerVolumeChanged" << d->m_player.volume();

    QTimer::singleShot(0, [this]() {Q_EMIT volumeChanged();});
}

void AudioManager::playerMutedChanged()
{
    qDebug() << "AudioManager::playerMutedChanged";

    QTimer::singleShot(0, [this]() {Q_EMIT mutedChanged(muted());});
}
