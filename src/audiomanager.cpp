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
    bool m_isSeekable = false;
    bool m_lockPositionSaving = false; // sort of lock mutex to prevent updating the player position while changing sources (which will emit lots of playerPositionChanged signals)

    void prepareAudioStream();

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

    connect(&DataManager::instance(), &DataManager::queueEntryMoved, this, &AudioManager::canGoNextChanged);
    connect(&DataManager::instance(), &DataManager::queueEntryAdded, this, &AudioManager::canGoNextChanged);
    connect(&DataManager::instance(), &DataManager::queueEntryRemoved, this, &AudioManager::canGoNextChanged);
    // we'll send custom seekableChanged signal to work around QMediaPlayer glitches

    // Check if an entry was playing when the program was shut down and restore it
    if (DataManager::instance().lastPlayingEntry() != QStringLiteral("none"))
        setEntry(DataManager::instance().getEntry(DataManager::instance().lastPlayingEntry()));
}

AudioManager::~AudioManager()
{
    d->mPowerInterface.setPreventSleep(false);
}

Entry* AudioManager::entry () const
{
    return d->m_entry;
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
        // Some error occured: probably best to unset the lastPlayingEntry to
        // avoid a deadlock when starting up again.
        DataManager::instance().setLastPlayingEntry(QStringLiteral("none"));
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
    d->m_lockPositionSaving = true;

    // First check if the previous track needs to be marked as read
    // TODO: make grace time a setting in SettingsManager
    if (d->m_entry) {
        //qDebug() << "Checking previous track";
        //qDebug() << "Left time" << (duration()-position());
        //qDebug() << "MediaStatus" << d->m_player.mediaStatus();
        if (( (duration()-position()) < 15000)
               || (d->m_player.mediaStatus() == QMediaPlayer::EndOfMedia) ) {
            //qDebug() << "Mark as read:" << d->m_entry->title();
            d->m_entry->setRead(true);
            d->m_entry->enclosure()->setPlayPosition(0);
            d->m_entry->setQueueStatus(false); // i.e. remove from queue TODO: make this a choice in settings
        }
    }

    //qDebug() << entry->hasEnclosure() << entry->enclosure() << entry->enclosure()->status();

    // do some checks on the new entry to see whether it's valid and not corrupted
    if (entry != nullptr && entry->hasEnclosure() && entry->enclosure() && entry->enclosure()->status() == Enclosure::Downloaded) {
        //qDebug() << "Going to change source";
        d->m_entry = entry;
        Q_EMIT entryChanged(entry);
        // the gst-pipeline is required to make sure that the pitch is not
        // changed when speeding up the audio stream
        // TODO: find a solution for Android (GStreamer not available on android by default)
#if !defined Q_OS_ANDROID && !defined Q_OS_WIN
        //qDebug() << "use custom pipeline";
        d->m_player.setMedia(QUrl(QStringLiteral("gst-pipeline: playbin uri=file://") + d->m_entry->enclosure()->path() + QStringLiteral(" audio_sink=\"scaletempo ! audioconvert ! audioresample ! autoaudiosink\" video_sink=\"fakevideosink\"")));
#else
        //qDebug() << "regular audio backend";
        d->m_player.setMedia(QUrl(QStringLiteral("file://")+d->m_entry->enclosure()->path()));
#endif
        // save the current playing track in the settingsfile for restoring on startup
        DataManager::instance().setLastPlayingEntry(d->m_entry->id());
        //qDebug() << "Changed source to" << d->m_entry->title();

        d->prepareAudioStream();
        d->m_readyToPlay = true;
        Q_EMIT canPlayChanged();
        Q_EMIT canPauseChanged();
        Q_EMIT canSkipForwardChanged();
        Q_EMIT canSkipBackwardChanged();
        Q_EMIT canGoNextChanged();
        d->m_isSeekable = true;
        Q_EMIT seekableChanged(true);
        //qDebug() << "Duration" << d->m_player.duration()/1000 << d->m_entry->enclosure()->duration();
        // Finally, check if duration mentioned in enclosure corresponds to real duration
        if ((d->m_player.duration()/1000) != d->m_entry->enclosure()->duration()) {
            d->m_entry->enclosure()->setDuration(d->m_player.duration()/1000);
            //qDebug() << "Correcting duration of" << d->m_entry->id() << "to" << d->m_player.duration()/1000;
        }
    } else {
        DataManager::instance().setLastPlayingEntry(QStringLiteral("none"));
        d->m_entry = nullptr;
        Q_EMIT entryChanged(nullptr);
        d->m_readyToPlay = false;
        Q_EMIT durationChanged(0);
        Q_EMIT positionChanged(0);
        Q_EMIT canPlayChanged();
        Q_EMIT canPauseChanged();
        Q_EMIT canSkipForwardChanged();
        Q_EMIT canSkipBackwardChanged();
        Q_EMIT canGoNextChanged();
        d->m_isSeekable = false;
        Q_EMIT seekableChanged(false);
    }
    // Unlock the position saving lock
    d->m_lockPositionSaving = false;
}

void AudioManager::setMuted(bool muted)
{
    d->m_player.setMuted(muted);
}

void AudioManager::setVolume(qreal volume)
{
    //qDebug() << "AudioManager::setVolume" << volume;

    auto realVolume = static_cast<qreal>(QAudio::convertVolume(volume / 100.0, QAudio::LogarithmicVolumeScale, QAudio::LinearVolumeScale));
    d->m_player.setVolume(qRound(realVolume * 100));
}

/*
void AudioManager::setSource(const QUrl &source)
{
    //qDebug() << "AudioManager::setSource" << source;

    d->m_player.setMedia({source});
}
*/

void AudioManager::setPosition(qint64 position)
{
    //qDebug() << "AudioManager::setPosition" << position;

    d->m_player.setPosition(position);
}

void AudioManager::setPlaybackRate(const qreal rate)
{
    //qDebug() << "AudioManager::setPlaybackRate" << rate;

    d->m_player.setPlaybackRate(rate);
}

void AudioManager::play()
{
    //qDebug() << "AudioManager::play";

    d->prepareAudioStream();
    d->m_player.play();
    d->m_isSeekable = true;
    Q_EMIT seekableChanged(d->m_isSeekable);
}

void AudioManager::pause()
{
    //qDebug() << "AudioManager::pause";

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
    //qDebug() << "AudioManager::stop";

    d->m_player.stop();
    d->m_isSeekable = false;
    Q_EMIT seekableChanged(d->m_isSeekable);
}

void AudioManager::seek(qint64 position)
{
    //qDebug() << "AudioManager::seek" << position;

    d->m_player.setPosition(position);
}

void AudioManager::skipForward()
{
    //qDebug() << "AudioManager::skipForward";
    seek(std::min((position() + SKIP_STEP), duration()));
}

void AudioManager::skipBackward()
{
    //qDebug() << "AudioManager::skipBackward";
    seek(std::max((qint64)0, (position() - SKIP_STEP)));
}

bool AudioManager::canGoNext() const
{
    // TODO: extend with streaming capability
    if (d->m_entry) {
        int index = DataManager::instance().getQueue().indexOf(d->m_entry->id());
        if (index >= 0) {
            // check if there is a next track
            if (index < DataManager::instance().getQueue().count()-1) {
                Entry* next_entry = DataManager::instance().getEntry(DataManager::instance().getQueue()[index+1]);
                if (next_entry->enclosure()) {
                    //qDebug() << "Enclosure status" << next_entry->enclosure()->path() << next_entry->enclosure()->status();
                    if (next_entry->enclosure()->status() == Enclosure::Downloaded) {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

void AudioManager::next()
{
    if (canGoNext()) {
        QMediaPlayer::State previousTrackState = playbackState();
        int index = DataManager::instance().getQueue().indexOf(d->m_entry->id());
        //qDebug() << "Skipping to" << DataManager::instance().getQueue()[index+1];
        setEntry(DataManager::instance().getEntry(DataManager::instance().getQueue()[index+1]));
        if (previousTrackState == QMediaPlayer::PlayingState) play();
    } else {
        //qDebug() << "Next track cannot be played, changing entry to nullptr";
        setEntry(nullptr);
    }
}

void AudioManager::mediaStatusChanged()
{
    //qDebug() << "AudioManager::mediaStatusChanged" << d->m_player.mediaStatus();

    // File has reached the end and has stopped
    if (d->m_player.mediaStatus() == QMediaPlayer::EndOfMedia) {
        next();
    }

    // if there is a problem with the current track, make sure that it's not
    // loaded again when the application is restarted, skip to next track and
    // delete the enclosure
    if (d->m_player.mediaStatus() == QMediaPlayer::InvalidMedia) {
        // save pointer to this bad entry to allow
        // us to delete the enclosure after the track has been unloaded
        Entry* badEntry = d->m_entry;
        DataManager::instance().setLastPlayingEntry(QStringLiteral("none"));
        stop();
        next();
        if (badEntry && badEntry->enclosure())
            badEntry->enclosure()->deleteFile();
        // TODO: show error overlay?
    }
}

void AudioManager::playerStateChanged()
{
    //qDebug() << "AudioManager::playerStateChanged" << d->m_player.state();

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
    //qDebug() << "AudioManager::playerVolumeChanged" << d->m_player.volume();

    QTimer::singleShot(0, [this]() {Q_EMIT volumeChanged();});
}

void AudioManager::playerMutedChanged()
{
    //qDebug() << "AudioManager::playerMutedChanged";

    QTimer::singleShot(0, [this]() {Q_EMIT mutedChanged(muted());});
}

void AudioManager::savePlayPosition(qint64 position)
{
    if (!d->m_lockPositionSaving) {
        if (d->m_entry) {
            if (d->m_entry->enclosure()) {
                d->m_entry->enclosure()->setPlayPosition(position);
            }
        }
    }
    //qDebug() << d->m_player.mediaStatus();
}

void AudioManagerPrivate::prepareAudioStream()
{
    /**
     * What follows is a dirty hack to get the player positioned at the
     * correct spot.  The audio only becomes seekable when the player is
     * actually playing and the stream is fully buffered.  So we start the
     * playback and then set a timer to wait until the stream becomes
     * seekable; then switch position and immediately pause the playback.
     * Unfortunately, this will produce an audible glitch with the current
     * QMediaPlayer backend.
     */
    qDebug() << "voodoo happening";
    qint64 startingPosition = m_entry->enclosure()->playPosition();
    m_player.play();
    if (!m_player.isSeekable()) {
        QEventLoop loop;
        QTimer timer;
        timer.setSingleShot(true);
        timer.setInterval(2000);
        loop.connect(&timer, SIGNAL (timeout()), &loop, SLOT (quit()) );
        loop.connect(&m_player, SIGNAL (seekableChanged(bool)), &loop, SLOT (quit()));
        //qDebug() << "Starting waiting loop";
        loop.exec();
    }
    if (m_player.mediaStatus() != QMediaPlayer::BufferedMedia) {
        QEventLoop loop;
        QTimer timer;
        timer.setSingleShot(true);
        timer.setInterval(2000);
        loop.connect(&timer, SIGNAL (timeout()), &loop, SLOT (quit()) );
        loop.connect(&m_player, SIGNAL (mediaStatusChanged(QMediaPlayer::MediaStatus)), &loop, SLOT (quit()));
        //qDebug() << "Starting waiting loop on media status" << d->m_player.mediaStatus();
        loop.exec();
    }        //qDebug() << "Changing position";
    if (startingPosition > 1000) m_player.setPosition(startingPosition);
    m_player.pause();
}
