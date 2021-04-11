/*
   SPDX-FileCopyrightText: 2017 (c) Matthieu Gallien <matthieu_gallien@yahoo.fr>

   SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "audiomanager.h"
#include "powermanagementinterface.h"

#include <QTimer>
#include <QAudio>

class AudioManagerPrivate
{

private:

    PowerManagementInterface mPowerInterface;

    QMediaPlayer mPlayer;

    Entry* entry = nullptr;

    friend class AudioManager;

};

AudioManager::AudioManager(QObject *parent) : QObject(parent), d(std::make_unique<AudioManagerPrivate>())
{
    connect(&d->mPlayer, &QMediaPlayer::mutedChanged, this, &AudioManager::playerMutedChanged);
    connect(&d->mPlayer, &QMediaPlayer::volumeChanged, this, &AudioManager::playerVolumeChanged);
    connect(&d->mPlayer, &QMediaPlayer::mediaChanged, this, &AudioManager::sourceChanged);
    connect(&d->mPlayer, &QMediaPlayer::mediaStatusChanged, this, &AudioManager::statusChanged);
    connect(&d->mPlayer, &QMediaPlayer::mediaStatusChanged, this, &AudioManager::mediaStatusChanged);
    connect(&d->mPlayer, &QMediaPlayer::stateChanged, this, &AudioManager::playbackStateChanged);
    connect(&d->mPlayer, &QMediaPlayer::stateChanged, this, &AudioManager::playerStateChanged);
    connect(&d->mPlayer, QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error), this, &AudioManager::errorChanged);
    connect(&d->mPlayer, &QMediaPlayer::durationChanged, this, &AudioManager::durationChanged);
    connect(&d->mPlayer, &QMediaPlayer::positionChanged, this, &AudioManager::positionChanged);
    connect(&d->mPlayer, &QMediaPlayer::seekableChanged, this, &AudioManager::seekableChanged);
}

AudioManager::~AudioManager()
{
    d->mPowerInterface.setPreventSleep(false);
}

Entry* AudioManager::entry () const
{
    return d->entry;
}

bool AudioManager::muted() const
{
    return d->mPlayer.isMuted();
}

qreal AudioManager::volume() const
{
    auto realVolume = static_cast<qreal>(d->mPlayer.volume() / 100.0);
    auto userVolume = static_cast<qreal>(QAudio::convertVolume(realVolume, QAudio::LinearVolumeScale, QAudio::LogarithmicVolumeScale));

    return userVolume * 100.0;
}

QUrl AudioManager::source() const
{
    return d->mPlayer.media().request().url();
}

QMediaPlayer::Error AudioManager::error() const
{
    if (d->mPlayer.error() != QMediaPlayer::NoError) {
        qDebug() << "AudioManager::error" << d->mPlayer.errorString();
    }

    return d->mPlayer.error();
}

qint64 AudioManager::duration() const
{
    return d->mPlayer.duration();
}

qint64 AudioManager::position() const
{
    return d->mPlayer.position();
}

bool AudioManager::seekable() const
{
    return d->mPlayer.isSeekable();
}

QMediaPlayer::State AudioManager::playbackState() const
{
    return d->mPlayer.state();
}

QMediaPlayer::MediaStatus AudioManager::status() const
{
    return d->mPlayer.mediaStatus();
}

void AudioManager::setEntry(Entry* entry)
{
    d->entry = entry;
    Q_EMIT entryChanged();
}

void AudioManager::setMuted(bool muted)
{
    d->mPlayer.setMuted(muted);
}

void AudioManager::setVolume(qreal volume)
{
    qDebug() << "AudioManager::setVolume" << volume;

    auto realVolume = static_cast<qreal>(QAudio::convertVolume(volume / 100.0, QAudio::LogarithmicVolumeScale, QAudio::LinearVolumeScale));
    d->mPlayer.setVolume(qRound(realVolume * 100));
}

void AudioManager::setSource(const QUrl &source)
{
    qDebug() << "AudioManager::setSource" << source;

    d->mPlayer.setMedia({source});
}

void AudioManager::setPosition(qint64 position)
{
    qDebug() << "AudioManager::setPosition" << position;

    d->mPlayer.setPosition(position);
}

void AudioManager::play()
{
    qDebug() << "AudioManager::play";

    d->mPlayer.play();
}

void AudioManager::pause()
{
    qDebug() << "AudioManager::pause";

    d->mPlayer.pause();
}

void AudioManager::stop()
{
    qDebug() << "AudioManager::stop";

    d->mPlayer.stop();
}

void AudioManager::seek(qint64 position)
{
    qDebug() << "AudioManager::seek" << position;

    d->mPlayer.setPosition(position);
}

void AudioManager::mediaStatusChanged()
{
    qDebug() << "AudioManager::mediaStatusChanged" << d->mPlayer.mediaStatus();
}

void AudioManager::playerStateChanged()
{
    qDebug() << "AudioManager::playerStateChanged" << d->mPlayer.state();

    switch(d->mPlayer.state())
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
    qDebug() << "AudioManager::playerVolumeChanged" << d->mPlayer.volume();

    QTimer::singleShot(0, [this]() {Q_EMIT volumeChanged();});
}

void AudioManager::playerMutedChanged()
{
    qDebug() << "AudioManager::playerMutedChanged";

    QTimer::singleShot(0, [this]() {Q_EMIT mutedChanged(muted());});
}
