/**
 * SPDX-FileCopyrightText: 2017 (c) Matthieu Gallien <matthieu_gallien@yahoo.fr>
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "audiomanager.h"

#include <QAudio>
#include <QEventLoop>
#include <QTimer>
#include <QtMath>
#include <algorithm>

#include <KLocalizedString>

#include "audiologging.h"
#include "datamanager.h"
#include "feed.h"
#include "powermanagementinterface.h"
#include "settingsmanager.h"

static const double MAX_RATE = 1.0;
static const double MIN_RATE = 2.5;
static const qint64 SKIP_STEP = 10000;
static const qint64 SKIP_TRACK_END = 15000;

class AudioManagerPrivate
{
private:
    PowerManagementInterface mPowerInterface;

    QMediaPlayer m_player;

    Entry *m_entry = nullptr;
    bool m_readyToPlay = false;
    bool m_isSeekable = false;
    bool m_continuePlayback = false;

    // sort of lock mutex to prevent updating the player position while changing
    // sources (which will emit lots of playerPositionChanged signals)
    bool m_lockPositionSaving = false;

    // m_pendingSeek is used to indicate whether a seek action is still pending
    //   * -1 corresponds to no seek action pending
    //   * any positive value indicates that a seek to position=m_pendingSeek is
    //     still pending
    qint64 m_pendingSeek = -1;

    friend class AudioManager;
};

AudioManager::AudioManager(QObject *parent)
    : QObject(parent)
    , d(std::make_unique<AudioManagerPrivate>())
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
    connect(&d->m_player, &QMediaPlayer::durationChanged, this, &AudioManager::playerDurationChanged);
    connect(&d->m_player, &QMediaPlayer::positionChanged, this, &AudioManager::positionChanged);
    connect(this, &AudioManager::positionChanged, this, &AudioManager::savePlayPosition);

    connect(&DataManager::instance(), &DataManager::queueEntryMoved, this, &AudioManager::canGoNextChanged);
    connect(&DataManager::instance(), &DataManager::queueEntryAdded, this, &AudioManager::canGoNextChanged);
    connect(&DataManager::instance(), &DataManager::queueEntryRemoved, this, &AudioManager::canGoNextChanged);
    // we'll send custom seekableChanged signal to work around QMediaPlayer glitches

    // Check if an entry was playing when the program was shut down and restore it
    if (DataManager::instance().lastPlayingEntry() != QStringLiteral("none")) {
        setEntry(DataManager::instance().getEntry(DataManager::instance().lastPlayingEntry()));
    }
}

AudioManager::~AudioManager()
{
    d->mPowerInterface.setPreventSleep(false);
}

Entry *AudioManager::entry() const
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
        qCDebug(kastsAudio) << "AudioManager::error" << d->m_player.errorString();
        // Some error occured: probably best to unset the lastPlayingEntry to
        // avoid a deadlock when starting up again.
        DataManager::instance().setLastPlayingEntry(QStringLiteral("none"));
    }

    return d->m_player.error();
}

qint64 AudioManager::duration() const
{
    // we fake the duration in case the track has not been properly loaded yet
    if (d->m_player.duration() > 0) {
        return d->m_player.duration();
    } else if (d->m_entry && d->m_entry->enclosure()) {
        return d->m_entry->enclosure()->duration() * 1000;
    } else {
        return 0;
    }
}

qint64 AudioManager::position() const
{
    // we fake the player position in case there is still a pending seek
    if (d->m_pendingSeek != -1) {
        return d->m_pendingSeek;
    } else {
        return d->m_player.position();
    }
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

void AudioManager::setEntry(Entry *entry)
{
    // reset any pending seek action, lock position saving and notify interval
    d->m_pendingSeek = -1;
    d->m_lockPositionSaving = true;
    d->m_player.setNotifyInterval(1000);

    // First check if the previous track needs to be marked as read
    // TODO: make grace time a setting in SettingsManager
    if (d->m_entry) {
        qCDebug(kastsAudio) << "Checking previous track";
        qCDebug(kastsAudio) << "Left time" << (duration() - position());
        qCDebug(kastsAudio) << "MediaStatus" << d->m_player.mediaStatus();
        if (((duration() > 0) && (position() > 0) && ((duration() - position()) < SKIP_TRACK_END)) || (d->m_player.mediaStatus() == QMediaPlayer::EndOfMedia)) {
            qCDebug(kastsAudio) << "Mark as read:" << d->m_entry->title();
            d->m_entry->setRead(true);
            d->m_entry->enclosure()->setPlayPosition(0);
            d->m_entry->setQueueStatus(false); // i.e. remove from queue TODO: make this a choice in settings
            d->m_continuePlayback = SettingsManager::self()->continuePlayingNextEntry();
        }
    }

    // do some checks on the new entry to see whether it's valid and not corrupted
    if (entry != nullptr && entry->hasEnclosure() && entry->enclosure() && entry->enclosure()->status() == Enclosure::Downloaded) {
        qCDebug(kastsAudio) << "Going to change source";
        d->m_entry = entry;
        Q_EMIT entryChanged(entry);
        // the gst-pipeline is required to make sure that the pitch is not
        // changed when speeding up the audio stream
        // TODO: find a solution for Android (GStreamer not available on android by default)
#if !defined Q_OS_ANDROID && !defined Q_OS_WIN
        qCDebug(kastsAudio) << "use custom pipeline";
        d->m_player.setMedia(QUrl(QStringLiteral("gst-pipeline: playbin uri=file://") + d->m_entry->enclosure()->path()
                                  + QStringLiteral(" audio_sink=\"scaletempo ! audioconvert ! audioresample ! autoaudiosink\" video_sink=\"fakevideosink\"")));
#else
        qCDebug(kastsAudio) << "regular audio backend";
        d->m_player.setMedia(QUrl(QStringLiteral("file://") + d->m_entry->enclosure()->path()));
#endif
        // save the current playing track in the settingsfile for restoring on startup
        DataManager::instance().setLastPlayingEntry(d->m_entry->id());
        qCDebug(kastsAudio) << "Changed source to" << d->m_entry->title();

        // call method which will try to make sure that the stream will skip to
        // the previously save position and make sure that the duration and
        // position are reported correctly
        prepareAudio();
    } else {
        DataManager::instance().setLastPlayingEntry(QStringLiteral("none"));
        d->m_entry = nullptr;
        Q_EMIT entryChanged(nullptr);
        d->m_player.stop();
        d->m_player.setMedia(nullptr);
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
}

void AudioManager::setMuted(bool muted)
{
    d->m_player.setMuted(muted);
}

void AudioManager::setVolume(qreal volume)
{
    qCDebug(kastsAudio) << "AudioManager::setVolume" << volume;

    auto realVolume = static_cast<qreal>(QAudio::convertVolume(volume / 100.0, QAudio::LogarithmicVolumeScale, QAudio::LinearVolumeScale));
    d->m_player.setVolume(qRound(realVolume * 100));
}

/*
void AudioManager::setSource(const QUrl &source)
{
    //qCDebug(kastsAudio) << "AudioManager::setSource" << source;

    d->m_player.setMedia({source});
}
*/

void AudioManager::setPosition(qint64 position)
{
    qCDebug(kastsAudio) << "AudioManager::setPosition" << position;

    seek(position);
}

void AudioManager::setPlaybackRate(const qreal rate)
{
    qCDebug(kastsAudio) << "AudioManager::setPlaybackRate" << rate;

    d->m_player.setPlaybackRate(rate);
}

void AudioManager::play()
{
    qCDebug(kastsAudio) << "AudioManager::play";

    // setting m_continuePlayback will make sure that, if the audio stream is
    // still being prepared, that the playback will start once it's ready
    d->m_continuePlayback = true;

    d->m_player.play();
    d->m_isSeekable = true;
    Q_EMIT seekableChanged(d->m_isSeekable);
    d->mPowerInterface.setPreventSleep(true);
}

void AudioManager::pause()
{
    qCDebug(kastsAudio) << "AudioManager::pause";

    // setting m_continuePlayback will make sure that, if the audio stream is
    // still being prepared, that the playback will pause once it's ready
    d->m_continuePlayback = false;

    d->m_isSeekable = true;
    d->m_player.pause();
    d->mPowerInterface.setPreventSleep(false);
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
    qCDebug(kastsAudio) << "AudioManager::stop";

    d->m_player.stop();
    d->m_continuePlayback = false;
    d->m_isSeekable = false;
    Q_EMIT seekableChanged(d->m_isSeekable);
    d->mPowerInterface.setPreventSleep(false);
}

void AudioManager::seek(qint64 position)
{
    qCDebug(kastsAudio) << "AudioManager::seek" << position;

    // if there is still a pending seek, then we simply update that pending
    // value, and then manually send the positionChanged signal to have the UI
    // updated
    if (d->m_pendingSeek != -1) {
        d->m_pendingSeek = position;
        Q_EMIT positionChanged(position);
    } else {
        d->m_player.setPosition(position);
    }
}

void AudioManager::skipForward()
{
    qCDebug(kastsAudio) << "AudioManager::skipForward";
    seek(std::min((position() + SKIP_STEP), duration()));
}

void AudioManager::skipBackward()
{
    qCDebug(kastsAudio) << "AudioManager::skipBackward";
    seek(std::max((qint64)0, (position() - SKIP_STEP)));
}

bool AudioManager::canGoNext() const
{
    // TODO: extend with streaming capability
    if (d->m_entry) {
        int index = DataManager::instance().queue().indexOf(d->m_entry->id());
        if (index >= 0) {
            // check if there is a next track
            if (index < DataManager::instance().queue().count() - 1) {
                Entry *next_entry = DataManager::instance().getEntry(DataManager::instance().queue()[index + 1]);
                if (next_entry->enclosure()) {
                    qCDebug(kastsAudio) << "Enclosure status" << next_entry->enclosure()->path() << next_entry->enclosure()->status();
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
        qCDebug(kastsAudio) << "Current playbackStatus before next() is:" << playbackState();
        d->m_continuePlayback = playbackState() == QMediaPlayer::State::PlayingState;

        int index = DataManager::instance().queue().indexOf(d->m_entry->id());
        qCDebug(kastsAudio) << "Skipping to" << DataManager::instance().queue()[index + 1];
        setEntry(DataManager::instance().getEntry(DataManager::instance().queue()[index + 1]));
    } else {
        qCDebug(kastsAudio) << "Next track cannot be played, changing entry to nullptr";
        setEntry(nullptr);
    }
}

void AudioManager::mediaStatusChanged()
{
    qCDebug(kastsAudio) << "AudioManager::mediaStatusChanged" << d->m_player.mediaStatus();

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
        Entry *badEntry = d->m_entry;
        DataManager::instance().setLastPlayingEntry(QStringLiteral("none"));
        stop();
        next();
        if (badEntry && badEntry->enclosure()) {
            badEntry->enclosure()->deleteFile();
            Q_EMIT logError(Error::Type::InvalidMedia, badEntry->feed()->url(), badEntry->id(), QMediaPlayer::InvalidMedia, i18n("Invalid Media"));
        }
    }
}

void AudioManager::playerStateChanged()
{
    qCDebug(kastsAudio) << "AudioManager::playerStateChanged" << d->m_player.state();

    switch (d->m_player.state()) {
    case QMediaPlayer::State::StoppedState:
        Q_EMIT stopped();
        d->mPowerInterface.setPreventSleep(false);
        break;
    case QMediaPlayer::State::PlayingState:
        // setPreventSleep is set in play() to avoid it toggling too rapidly
        // see d->prepareAudioStream() for details
        Q_EMIT playing();
        break;
    case QMediaPlayer::State::PausedState:
        // setPreventSleep is set in pause() to avoid it toggling too rapidly
        // see d->prepareAudioStream() for details
        Q_EMIT paused();
        break;
    }
}

void AudioManager::playerDurationChanged(qint64 duration)
{
    qCDebug(kastsAudio) << "AudioManager::playerDurationChanged" << duration;

    // Check if duration mentioned in enclosure corresponds to real duration
    if (duration > 0 && (duration / 1000) != d->m_entry->enclosure()->duration()) {
        qCDebug(kastsAudio) << "Correcting duration of" << d->m_entry->id() << "to" << duration / 1000 << "(was" << d->m_entry->enclosure()->duration() << ")";
        d->m_entry->enclosure()->setDuration(duration / 1000);
    }

    qint64 correctedDuration = duration;
    QTimer::singleShot(0, this, [this, correctedDuration]() {
        Q_EMIT durationChanged(correctedDuration);
    });
}

void AudioManager::playerVolumeChanged()
{
    qCDebug(kastsAudio) << "AudioManager::playerVolumeChanged" << d->m_player.volume();

    QTimer::singleShot(0, this, [this]() {
        Q_EMIT volumeChanged();
    });
}

void AudioManager::playerMutedChanged()
{
    qCDebug(kastsAudio) << "AudioManager::playerMutedChanged" << muted();

    QTimer::singleShot(0, this, [this]() {
        Q_EMIT mutedChanged(muted());
    });
}

void AudioManager::savePlayPosition()
{
    qCDebug(kastsAudio) << "AudioManager::savePlayPosition";

    // First check if there is still a pending seek
    checkForPendingSeek();

    if (!d->m_lockPositionSaving) {
        if (d->m_entry) {
            if (d->m_entry->enclosure()) {
                d->m_entry->enclosure()->setPlayPosition(position());
            }
        }
    }
    qCDebug(kastsAudio) << d->m_player.mediaStatus();
}

void AudioManager::prepareAudio()
{
    d->m_player.pause();

    qint64 newDuration = duration();

    qint64 startingPosition = d->m_entry->enclosure()->playPosition();
    qCDebug(kastsAudio) << "Changing position to" << startingPosition / 1000 << "sec";
    if (startingPosition <= newDuration) {
        d->m_pendingSeek = startingPosition;
        // Change notify interval temporarily.  This will help with reducing the
        // startup audio glitch to a minimum.
        d->m_player.setNotifyInterval(50);
        // do not call d->m_player.setPosition() here since it might start
        // sending signals with a.o. incorrect duration and position
    } else {
        d->m_pendingSeek = -1;
    }

    // Emit positionChanged and durationChanged signals to make sure that
    // the GUI can see the faked values (if needed)
    qint64 newPosition = position();
    Q_EMIT durationChanged(newDuration);
    Q_EMIT positionChanged(newPosition);

    d->m_readyToPlay = true;
    Q_EMIT canPlayChanged();
    Q_EMIT canPauseChanged();
    Q_EMIT canSkipForwardChanged();
    Q_EMIT canSkipBackwardChanged();
    Q_EMIT canGoNextChanged();
    d->m_isSeekable = true;
    Q_EMIT seekableChanged(true);

    qCDebug(kastsAudio) << "Duration reported by d->m_player" << d->m_player.duration();
    qCDebug(kastsAudio) << "Duration reported by enclosure (in ms)" << d->m_entry->enclosure()->duration() * 1000;
    qCDebug(kastsAudio) << "Duration reported by AudioManager" << newDuration;
    qCDebug(kastsAudio) << "Position reported by d->m_player" << d->m_player.position();
    qCDebug(kastsAudio) << "Saved position stored in enclosure (in ms)" << startingPosition;
    qCDebug(kastsAudio) << "Position reported by AudioManager" << newPosition;

    if (d->m_continuePlayback) {
        // we call play() and not d->m_player.play() because we want to trigger
        // things like inhibit suspend
        play();
        d->m_continuePlayback = false;
    }

    d->m_lockPositionSaving = false;
}

void AudioManager::checkForPendingSeek()
{
    qint64 position = d->m_player.position();
    qCDebug(kastsAudio) << "Seek pending?" << d->m_pendingSeek;
    qCDebug(kastsAudio) << "Current position" << position;

    // Check if we're supposed to skip to a new position
    if (d->m_pendingSeek != -1 && d->m_player.mediaStatus() == QMediaPlayer::BufferedMedia && d->m_player.duration() > 0) {
        if (abs(d->m_pendingSeek - position) > 2000) {
            qCDebug(kastsAudio) << "Position seek still pending to position" << d->m_pendingSeek;
            qCDebug(kastsAudio) << "Current reported position and duration" << d->m_player.position() << d->m_player.duration();
            // be very careful because this setPosition call will trigger
            // a positionChanged signal, which will be nested, so we call it in
            // a QTimer::singleShot
            qint64 seekPosition = d->m_pendingSeek;
            QTimer::singleShot(0, this, [this, seekPosition]() {
                d->m_player.setPosition(seekPosition);
            });
        } else {
            qCDebug(kastsAudio) << "Pending position seek has been executed; to position" << d->m_pendingSeek;
            d->m_pendingSeek = -1;
            d->m_player.setNotifyInterval(1000);
        }
    }
}

QString AudioManager::formattedDuration() const
{
    return m_kformat.formatDuration(duration());
}

QString AudioManager::formattedLeftDuration() const
{
    return m_kformat.formatDuration(duration() - position());
}

QString AudioManager::formattedPosition() const
{
    return m_kformat.formatDuration(position());
}
