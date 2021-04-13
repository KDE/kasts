/**
 * SPDX-FileCopyrightText: 2017 (c) Matthieu Gallien <matthieu_gallien@yahoo.fr>
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "audiomanager.h"

#include <algorithm>
#include <QTimer>
#include <QAudio>
#include <QEventLoop>

#include "powermanagementinterface.h"
#include "datamanager.h"
#include "settingsmanager.h"

static const double MAX_RATE = 1.0;
static const double MIN_RATE = 2.5;
static const qint64 SKIP_STEP = 10000;

class AudioManagerPrivate
{

private:

    PowerManagementInterface mPowerInterface;

    QMediaPlayer m_player;

    Entry* m_entry = nullptr;
    bool m_readyToPlay = false;
    bool playerOpen = false;
    bool m_isSeekable = false;
    bool m_lockPositionSaving = false; // sort of lock mutex to prevent updating the player position while changing sources (which will emit lots of playerPositionChanged signals)

    friend class AudioManager;
};

AudioManager::AudioManager(QObject *parent) : QObject(parent), d(std::make_unique<AudioManagerPrivate>())
{
    connect(&d->m_player, &QMediaPlayer::mutedChanged, this, &AudioManager::playerMutedChanged);
    connect(&d->m_player, &QMediaPlayer::volumeChanged, this, &AudioManager::playerVolumeChanged);
    connect(&d->m_player, &QMediaPlayer::mediaChanged, this, &AudioManager::sourceChanged);
    connect(&d->m_player, &QMediaPlayer::mediaStatusChanged, this, &AudioManager::statusChanged);
    connect(&d->m_player, &QMediaPlayer::mediaStatusChanged, this, &AudioManager::mediaStatusChanged);
    connect(&d->m_player, &QMediaPlayer::stateChanged, this, &AudioManager::playbackStateChanged);
    connect(&d->m_player, &QMediaPlayer::stateChanged, this, &AudioManager::playerStateChanged);
    connect(&d->m_player, &QMediaPlayer::playbackRateChanged, this, &AudioManager::playbackRateChanged);
    connect(&d->m_player, QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error), this, &AudioManager::errorChanged);
    connect(&d->m_player, &QMediaPlayer::durationChanged, this, &AudioManager::durationChanged);
    connect(&d->m_player, &QMediaPlayer::positionChanged, this, &AudioManager::positionChanged);
    connect(&d->m_player, &QMediaPlayer::positionChanged, this, &AudioManager::savePlayPosition);
    // we'll send custom seekableChanged signal to work around QMediaPlayer glitches

    // Check if an entry was playing when the program was shut down and restore it
    if (SettingsManager::self()->lastPlayingEntry() != QStringLiteral("none"))
        setEntry(DataManager::instance().getEntry(SettingsManager::self()->lastPlayingEntry()));
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

QUrl AudioManager::source() const
{
    return d->m_player.media().request().url();
}

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
    return d->m_isSeekable;
}

bool AudioManager::canPlay() const
{
    return (d->m_readyToPlay);
}

bool AudioManager::canPause() const
{
    return (d->m_readyToPlay);
}

bool AudioManager::canSkipForward() const
{
    return (d->m_readyToPlay);
}

bool AudioManager::canSkipBackward() const
{
    return (d->m_readyToPlay);
}

QMediaPlayer::State AudioManager::playbackState() const
{
    return d->m_player.state();
}

qreal AudioManager::playbackRate() const
{
    return d->m_player.playbackRate();
}

qreal AudioManager::minimumPlaybackRate() const
{
    return MIN_RATE;
}

qreal AudioManager::maximumPlaybackRate() const
{
    return MAX_RATE;
}

QMediaPlayer::MediaStatus AudioManager::status() const
{
    return d->m_player.mediaStatus();
}

void AudioManager::setEntry(Entry* entry)
{
    if (entry != nullptr) {
        qDebug() << "Going to change source";
        d->m_lockPositionSaving = true;
        d->m_entry = entry;
        d->m_player.setMedia(QUrl(QStringLiteral("file://")+d->m_entry->enclosure()->path()));
        // save the current playing track in the settingsfile for restoring on startup
        SettingsManager::self()->setLastPlayingEntry(d->m_entry->id());
        qDebug() << "Changed source to" << d->m_entry->title();

        qint64 startingPosition = d->m_entry->enclosure()->playPosition();
        // What follows is a dirty hack to get the player positioned at the
        // correct spot.  The audio only becomes seekable when the player is
        // actually playing.  So we start the playback and then set a timer to
        // wait until the stream becomes seekable; then switch position and
        // immediately pause the playback.
        // Unfortunately, this will produce an audible glitch with the current
        // QMediaPlayer backend.
        d->m_player.play();
        if (!d->m_player.isSeekable()) {
            QEventLoop loop;
            QTimer timer;
            timer.setSingleShot(true);
            timer.setInterval(2000);
            loop.connect(&timer, SIGNAL (timeout()), &loop, SLOT (quit()) );
            loop.connect(&d->m_player, SIGNAL (seekableChanged(bool)), &loop, SLOT (quit()));
            qDebug() << "Starting waiting loop";
            loop.exec();
        }
        if (d->m_player.mediaStatus() != QMediaPlayer::BufferedMedia) {
            QEventLoop loop;
            QTimer timer;
            timer.setSingleShot(true);
            timer.setInterval(2000);
            loop.connect(&timer, SIGNAL (timeout()), &loop, SLOT (quit()) );
            loop.connect(&d->m_player, SIGNAL (mediaStatusChanged(QMediaPlayer::MediaStatus)), &loop, SLOT (quit()));
            qDebug() << "Starting waiting loop on media status" << d->m_player.mediaStatus();
            loop.exec();
        }        qDebug() << "Changing position";
        if (startingPosition > 1000) d->m_player.setPosition(startingPosition);
        d->m_player.pause();
        d->m_lockPositionSaving = false;
        d->m_readyToPlay = true;
        Q_EMIT entryChanged(entry);
        Q_EMIT canPlayChanged();
        Q_EMIT canPauseChanged();
        Q_EMIT canSkipForwardChanged();
        Q_EMIT canSkipBackwardChanged();
        d->m_isSeekable = true;
        Q_EMIT seekableChanged(true);
    } else {
        d->m_readyToPlay = false;
        Q_EMIT canPlayChanged();
        Q_EMIT canPauseChanged();
        Q_EMIT canSkipForwardChanged();
        Q_EMIT canSkipBackwardChanged();
        d->m_isSeekable = false;
        Q_EMIT seekableChanged(false);
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

void AudioManager::setPosition(qint64 position)
{
    qDebug() << "AudioManager::setPosition" << position;

    d->m_player.setPosition(position);
}

void AudioManager::setPlaybackRate(const qreal rate)
{
    qDebug() << "AudioManager::setPlaybackRate" << rate;

    d->m_player.setPlaybackRate(rate);
}

void AudioManager::play()
{
    qDebug() << "AudioManager::play";

    d->m_player.play();
    d->m_isSeekable = true;
    Q_EMIT seekableChanged(d->m_isSeekable);
}

void AudioManager::pause()
{
    qDebug() << "AudioManager::pause";

    d->m_player.play();
    d->m_isSeekable = true;
    d->m_player.pause();
}

void AudioManager::playPause()
{
    if (playbackState() == QMediaPlayer::State::PausedState)
        play();
    else if (playbackState() == QMediaPlayer::State::PlayingState)
        pause();
}

void AudioManager::stop()
{
    qDebug() << "AudioManager::stop";

    d->m_player.stop();
    d->m_isSeekable = false;
    Q_EMIT seekableChanged(d->m_isSeekable);
}

void AudioManager::seek(qint64 position)
{
    qDebug() << "AudioManager::seek" << position;

    d->m_player.setPosition(position);
}

void AudioManager::skipForward()
{
    qDebug() << "AudioManager::skipForward";
    seek(std::min((position() + SKIP_STEP), duration()));
}

void AudioManager::skipBackward()
{
    qDebug() << "AudioManager::skipBackward";
    seek(std::max((qint64)0, (position() - SKIP_STEP)));
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

void AudioManager::savePlayPosition(qint64 position)
{
    if (!d->m_lockPositionSaving)
        d->m_entry->enclosure()->setPlayPosition(position);
    qDebug() << d->m_player.mediaStatus();
}
