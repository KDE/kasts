/**
 * SPDX-FileCopyrightText: 2017 (c) Matthieu Gallien <matthieu_gallien@yahoo.fr>
 * SPDX-FileCopyrightText: 2021-2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "audiomanager.h"

#include <QEventLoop>
#include <QTimer>
#include <QtMath>
#include <algorithm>

#include <KLocalizedString>

#include "audiologging.h"
#include "datamanager.h"
#include "feed.h"
#include "fetcher.h"
#include "models/errorlogmodel.h"
#include "settingsmanager.h"
#include "utils/networkconnectionmanager.h"

class AudioManagerPrivate
{
private:
    KMediaSession m_player = KMediaSession(QStringLiteral("kasts"), QStringLiteral("org.kde.kasts"));

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

    QTimer *m_sleepTimer = nullptr;
    qint64 m_sleepTime = -1;
    qint64 m_remainingSleepTime = -1;

    bool m_isStreaming = false;

    friend class AudioManager;
};

AudioManager::AudioManager(QObject *parent)
    : QObject(parent)
    , d(std::make_unique<AudioManagerPrivate>())
{
    d->m_player.setMpris2PauseInsteadOfStop(true);

    connect(&d->m_player, &KMediaSession::currentBackendChanged, this, &AudioManager::currentBackendChanged);
    connect(&d->m_player, &KMediaSession::mutedChanged, this, &AudioManager::playerMutedChanged);
    connect(&d->m_player, &KMediaSession::volumeChanged, this, &AudioManager::playerVolumeChanged);
    connect(&d->m_player, &KMediaSession::sourceChanged, this, &AudioManager::sourceChanged);
    connect(&d->m_player, &KMediaSession::mediaStatusChanged, this, &AudioManager::statusChanged);
    connect(&d->m_player, &KMediaSession::mediaStatusChanged, this, &AudioManager::mediaStatusChanged);
    connect(&d->m_player, &KMediaSession::playbackStateChanged, this, &AudioManager::playbackStateChanged);
    connect(&d->m_player, &KMediaSession::playbackRateChanged, this, &AudioManager::playbackRateChanged);
    connect(&d->m_player, &KMediaSession::errorChanged, this, &AudioManager::errorChanged);
    connect(&d->m_player, &KMediaSession::durationChanged, this, &AudioManager::playerDurationChanged);
    connect(&d->m_player, &KMediaSession::positionChanged, this, &AudioManager::positionChanged);
    connect(this, &AudioManager::positionChanged, this, &AudioManager::savePlayPosition);

    // connect signals for MPRIS2
    connect(this, &AudioManager::canSkipForwardChanged, this, [this]() {
        d->m_player.setCanGoNext(canSkipForward());
    });
    connect(this, &AudioManager::canSkipBackwardChanged, this, [this]() {
        d->m_player.setCanGoPrevious(canSkipBackward());
    });
    connect(&d->m_player, &KMediaSession::nextRequested, this, &AudioManager::skipForward);
    connect(&d->m_player, &KMediaSession::previousRequested, this, &AudioManager::skipBackward);
    connect(&d->m_player, &KMediaSession::raiseWindowRequested, this, &AudioManager::raiseWindowRequested);
    connect(&d->m_player, &KMediaSession::quitRequested, this, &AudioManager::quitRequested);

    connect(this, &AudioManager::playbackRateChanged, &DataManager::instance(), &DataManager::playbackRateChanged);
    connect(&DataManager::instance(), &DataManager::queueEntryMoved, this, &AudioManager::canGoNextChanged);
    connect(&DataManager::instance(), &DataManager::queueEntryAdded, this, &AudioManager::canGoNextChanged);
    connect(&DataManager::instance(), &DataManager::queueEntryRemoved, this, &AudioManager::canGoNextChanged);
    // we'll send custom seekableChanged signal to work around possible backend glitches

    connect(this, &AudioManager::logError, &ErrorLogModel::instance(), &ErrorLogModel::monitorErrorMessages);

    // Check if an entry was playing when the program was shut down and restore it
    if (DataManager::instance().lastPlayingEntry() != QStringLiteral("none")) {
        setEntry(DataManager::instance().getEntry(DataManager::instance().lastPlayingEntry()));
    }
}

AudioManager::~AudioManager() = default;

QString AudioManager::backendName(KMediaSession::MediaBackends backend) const
{
    qCDebug(kastsAudio) << "AudioManager::backendName()";
    return d->m_player.backendName(backend);
}

KMediaSession::MediaBackends AudioManager::currentBackend() const
{
    qCDebug(kastsAudio) << "AudioManager::currentBackend()";
    return d->m_player.currentBackend();
}

QList<KMediaSession::MediaBackends> AudioManager::availableBackends() const
{
    qCDebug(kastsAudio) << "AudioManager::availableBackends()";
    return d->m_player.availableBackends();
}

Entry *AudioManager::entry() const
{
    return d->m_entry;
}

bool AudioManager::muted() const
{
    return d->m_player.muted();
}

qreal AudioManager::volume() const
{
    return d->m_player.volume();
}

QUrl AudioManager::source() const
{
    return d->m_player.source();
}

KMediaSession::Error AudioManager::error() const
{
    if (d->m_player.error() != KMediaSession::NoError) {
        qCDebug(kastsAudio) << "AudioManager::error" << d->m_player.error();
        // Some error occurred: probably best to unset the lastPlayingEntry to
        // avoid a deadlock when starting up again.
        DataManager::instance().setLastPlayingEntry(QStringLiteral("none"));
    }

    return d->m_player.error();
}

qint64 AudioManager::duration() const
{
    // we fake the duration in case the track has not been properly loaded yet
    if (!d->m_readyToPlay) {
        if (d->m_entry && d->m_entry->enclosure()) {
            return d->m_entry->enclosure()->duration() * 1000;
        } else {
            return 0;
        }
    } else if (d->m_player.duration() > 0) {
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
    if (!d->m_readyToPlay) {
        if (d->m_entry && d->m_entry->enclosure()) {
            return d->m_entry->enclosure()->playPosition();
        } else {
            return 0;
        }
    } else if (d->m_pendingSeek != -1) {
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

KMediaSession::PlaybackState AudioManager::playbackState() const
{
    return d->m_player.playbackState();
}

qreal AudioManager::playbackRate() const
{
    return d->m_player.playbackRate();
}

qreal AudioManager::minimumPlaybackRate() const
{
    return d->m_player.minimumPlaybackRate();
}

qreal AudioManager::maximumPlaybackRate() const
{
    return d->m_player.maximumPlaybackRate();
}

bool AudioManager::isStreaming() const
{
    return d->m_isStreaming;
}

KMediaSession::MediaStatus AudioManager::status() const
{
    return d->m_player.mediaStatus();
}

void AudioManager::setCurrentBackend(KMediaSession::MediaBackends backend)
{
    qCDebug(kastsAudio) << "AudioManager::setCurrentBackend(" << backend << ")";

    KMediaSession::PlaybackState currentState = playbackState();
    qint64 currentRate = playbackRate();

    d->m_player.setCurrentBackend(backend);

    setEntry(d->m_entry);
    if (currentState == KMediaSession::PlaybackState::PlayingState) {
        play();
    }
    // TODO: Fix restoring the current playback rate
    setPlaybackRate(currentRate);
}

void AudioManager::setEntry(Entry *entry)
{
    qCDebug(kastsAudio) << "begin AudioManager::setEntry";
    // First unset current track and save playing state, such that any signal
    // that still fires doesn't operate on the wrong track.

    // disconnect any pending redirectUrl signals
    bool signalDisconnect = disconnect(&Fetcher::instance(), &Fetcher::foundRedirectedUrl, this, nullptr);
    qCDebug(kastsAudio) << "disconnected dangling foundRedirectedUrl signal:" << signalDisconnect;

    // reset any pending seek action and lock position saving
    d->m_pendingSeek = -1;
    d->m_lockPositionSaving = true;

    Entry *oldEntry = d->m_entry;
    d->m_entry = nullptr;

    // Check if the previous track needs to be marked as read
    if (oldEntry && !signalDisconnect) {
        qCDebug(kastsAudio) << "Checking previous track";
        qCDebug(kastsAudio) << "Left time" << (duration() - position());
        qCDebug(kastsAudio) << "MediaStatus" << d->m_player.mediaStatus();
        if (((duration() > 0) && (position() > 0) && ((duration() - position()) < SettingsManager::self()->markAsPlayedBeforeEnd() * 1000))
            || (d->m_player.mediaStatus() == KMediaSession::EndOfMedia)) {
            qCDebug(kastsAudio) << "Mark as read:" << oldEntry->title();
            oldEntry->enclosure()->setPlayPosition(0);
            oldEntry->setRead(true);
            stop();
            d->m_continuePlayback = SettingsManager::self()->continuePlayingNextEntry();
        } else {
            bool continuePlaying = d->m_continuePlayback; // saving to local bool because it will be overwritten by the stop action
            stop();
            d->m_continuePlayback = continuePlaying;
        }
    }

    // do some checks on the new entry to see whether it's valid and not corrupted
    if (entry != nullptr && entry->hasEnclosure() && entry->enclosure()
        && (entry->enclosure()->status() == Enclosure::Downloaded || NetworkConnectionManager::instance().streamingAllowed())) {
        qCDebug(kastsAudio) << "Going to change source";

        setEntryInfo(entry);

        if (entry->enclosure()->status() == Enclosure::Downloaded) { // i.e. local file
            if (d->m_isStreaming) {
                d->m_isStreaming = false;
                Q_EMIT isStreamingChanged();
            }

            // call method which will try to make sure that the stream will skip
            // to the previously save position and make sure that the duration
            // and position are reported correctly
            prepareAudio(QUrl::fromLocalFile(entry->enclosure()->path()));
        } else {
            // i.e. streaming; we first want to resolve the real URL, following
            // redirects
            QUrl loadUrl = QUrl(entry->enclosure()->url());
            Fetcher::instance().getRedirectedUrl(loadUrl);
            connect(&Fetcher::instance(), &Fetcher::foundRedirectedUrl, this, [this, entry, loadUrl](const QUrl &oldUrl, const QUrl &newUrl) {
                qCDebug(kastsAudio) << oldUrl << newUrl;
                if (loadUrl == oldUrl) {
                    bool signalDisconnect = disconnect(&Fetcher::instance(), &Fetcher::foundRedirectedUrl, this, nullptr);
                    qCDebug(kastsAudio) << "disconnected" << signalDisconnect;

                    if (!d->m_isStreaming) {
                        d->m_isStreaming = true;
                        Q_EMIT isStreamingChanged();
                    }

                    d->m_entry = entry;
                    Q_EMIT entryChanged(entry);

                    // call method which will try to make sure that the stream will skip
                    // to the previously save position and make sure that the duration
                    // and position are reported correctly
                    prepareAudio(newUrl);
                }
            });
        }

    } else {
        DataManager::instance().setLastPlayingEntry(QStringLiteral("none"));
        d->m_entry = nullptr;
        Q_EMIT entryChanged(nullptr);
        d->m_player.stop();
        d->m_player.setSource(QUrl());
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

    d->m_player.setVolume(qRound(volume));
}

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

    // if we're streaming, check that we're still connected and check for metered
    // connection
    if (isStreaming()) {
        if (!NetworkConnectionManager::instance().streamingAllowed()) {
            if (NetworkConnectionManager::instance().networkReachable()) {
                qCDebug(kastsAudio) << "Refusing to play: streaming on metered connection not allowed";
                QString feedUrl, entryId;
                if (d->m_entry) {
                    feedUrl = d->m_entry->feed()->url();
                    entryId = d->m_entry->id();
                }
                Q_EMIT logError(Error::Type::MeteredStreamingNotAllowed, feedUrl, entryId, 0, i18n("Streaming on metered connection not allowed"), QString());
                return;
            } else {
                qCDebug(kastsAudio) << "Refusing to play: no network connection";
                QString feedUrl, entryId;
                if (d->m_entry) {
                    feedUrl = d->m_entry->feed()->url();
                    entryId = d->m_entry->id();
                }
                Q_EMIT logError(Error::Type::NoNetwork, feedUrl, entryId, 0, i18n("No network connection"), QString());
                return;
            }
        }
    }

    // setting m_continuePlayback will make sure that, if the audio stream is
    // still being prepared, that the playback will start once it's ready
    d->m_continuePlayback = true;

    if (d->m_readyToPlay) {
        d->m_player.play();
        d->m_isSeekable = true;
        Q_EMIT seekableChanged(d->m_isSeekable);

        if (d->m_entry && d->m_entry->getNew()) {
            d->m_entry->setNew(false);
        }
    }
}

void AudioManager::pause()
{
    qCDebug(kastsAudio) << "AudioManager::pause";

    // setting m_continuePlayback will make sure that, if the audio stream is
    // still being prepared, that the playback will pause once it's ready
    d->m_continuePlayback = false;

    if (d->m_readyToPlay) {
        d->m_isSeekable = true;
        d->m_player.pause();
    }
}

void AudioManager::playPause()
{
    if (playbackState() == KMediaSession::PlaybackState::PlayingState) {
        pause();
    } else {
        play();
    }
}

void AudioManager::stop()
{
    qCDebug(kastsAudio) << "AudioManager::stop";

    d->m_player.stop();
    d->m_continuePlayback = false;
    d->m_isSeekable = false;
    Q_EMIT seekableChanged(d->m_isSeekable);
}

void AudioManager::seek(qint64 position)
{
    qCDebug(kastsAudio) << "AudioManager::seek" << position;

    // if there is still a pending seek, then we simply update that pending
    // value, and then manually send the positionChanged signal to have the UI
    // updated
    // NOTE: this can also happen while the streaming URL is still resolving, so
    // we also allow seeking even when the track is not yet readyToPlay.
    if (d->m_pendingSeek != -1 || !d->m_readyToPlay) {
        d->m_pendingSeek = position;
        Q_EMIT positionChanged(position);
    } else if (d->m_pendingSeek == -1 && d->m_readyToPlay) {
        d->m_player.setPosition(position);
    }
}

void AudioManager::skipForward()
{
    qCDebug(kastsAudio) << "AudioManager::skipForward";
    seek(std::min((position() + (1000 * SettingsManager::skipForward())), duration()));
}

void AudioManager::skipBackward()
{
    qCDebug(kastsAudio) << "AudioManager::skipBackward";
    seek(std::max((qint64)0, (position() - (1000 * SettingsManager::skipBackward()))));
}

bool AudioManager::canGoNext() const
{
    if (d->m_entry) {
        int index = DataManager::instance().queue().indexOf(d->m_entry->id());
        if (index >= 0) {
            // check if there is a next track
            if (index < DataManager::instance().queue().count() - 1) {
                Entry *next_entry = DataManager::instance().getEntry(DataManager::instance().queue()[index + 1]);
                if (next_entry && next_entry->enclosure()) {
                    qCDebug(kastsAudio) << "Enclosure status" << next_entry->enclosure()->path() << next_entry->enclosure()->status();
                    if (next_entry->enclosure()->status() == Enclosure::Downloaded) {
                        return true;
                    } else {
                        if (NetworkConnectionManager::instance().streamingAllowed()) {
                            return true;
                        }
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
    if (d->m_player.mediaStatus() == KMediaSession::EndOfMedia) {
        next();
    }

    // if there is a problem with the current track, make sure that it's not
    // loaded again when the application is restarted, skip to next track and
    // delete the enclosure
    if (d->m_player.mediaStatus() == KMediaSession::InvalidMedia) {
        // save pointer to this bad entry to allow
        // us to delete the enclosure after the track has been unloaded
        Entry *badEntry = d->m_entry;
        DataManager::instance().setLastPlayingEntry(QStringLiteral("none"));
        stop();
        next();
        if (badEntry && badEntry->enclosure()) {
            badEntry->enclosure()->deleteFile();
            Q_EMIT logError(Error::Type::InvalidMedia, badEntry->feed()->url(), badEntry->id(), KMediaSession::InvalidMedia, i18n("Invalid Media"), QString());
        }
    }
}

void AudioManager::playerDurationChanged(qint64 duration)
{
    qCDebug(kastsAudio) << "AudioManager::playerDurationChanged" << duration;

    // Check if duration mentioned in enclosure corresponds to real duration
    if (d->m_entry && d->m_entry->enclosure()) {
        if (duration > 0 && (duration / 1000) != d->m_entry->enclosure()->duration()) {
            qCDebug(kastsAudio) << "Correcting duration of" << d->m_entry->id() << "to" << duration / 1000 << "(was" << d->m_entry->enclosure()->duration()
                                << ")";
            d->m_entry->enclosure()->setDuration(duration / 1000);
        }
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
        if (d->m_entry && d->m_entry->enclosure()) {
            if (d->m_entry->enclosure()) {
                d->m_entry->enclosure()->setPlayPosition(position());
            }
        }
    }
    qCDebug(kastsAudio) << d->m_player.mediaStatus();
}

void AudioManager::setEntryInfo(Entry *entry)
{
    // Set info for next track in preparation for the actual audio player to be
    // set up and configured.  We set all the info based on what's in the entry
    // and disable all the controls until the track is ready to be played

    d->m_player.setSource(QUrl());
    d->m_entry = entry;
    Q_EMIT entryChanged(entry);

    qint64 newDuration = entry->enclosure()->duration() * 1000;
    qint64 newPosition = entry->enclosure()->playPosition();
    if (newPosition > newDuration && newPosition < 0) {
        newPosition = 0;
    }

    // Emit positionChanged and durationChanged signals to make sure that
    // the GUI can see the faked values (if needed)
    Q_EMIT durationChanged(newDuration);
    Q_EMIT positionChanged(newPosition);

    d->m_readyToPlay = false;
    Q_EMIT canPlayChanged();
    Q_EMIT canPauseChanged();
    Q_EMIT canSkipForwardChanged();
    Q_EMIT canSkipBackwardChanged();
    Q_EMIT canGoNextChanged();
    d->m_isSeekable = false;
    Q_EMIT seekableChanged(false);
}

void AudioManager::prepareAudio(const QUrl &loadUrl)
{
    d->m_player.setSource(loadUrl);

    // save the current playing track in the settingsfile for restoring on startup
    DataManager::instance().setLastPlayingEntry(d->m_entry->id());
    qCDebug(kastsAudio) << "Changed source to" << d->m_entry->title();

    d->m_player.pause();

    qint64 newDuration = duration();

    qint64 startingPosition = d->m_entry->enclosure()->playPosition();
    qCDebug(kastsAudio) << "Changing position to" << startingPosition / 1000 << "sec";
    // if a seek is still pending then we don't set the position here
    // this can happen e.g. if a chapter marker was clicked on a non-playing entry
    if (d->m_pendingSeek == -1) {
        if (startingPosition <= newDuration) {
            d->m_pendingSeek = startingPosition;
        } else {
            d->m_pendingSeek = -1;
        }
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
    } else {
        pause();
    }

    d->m_lockPositionSaving = false;

    // set metadata for MPRIS2
    updateMetaData();
}

void AudioManager::checkForPendingSeek()
{
    qint64 position = d->m_player.position();
    qCDebug(kastsAudio) << "Seek pending?" << d->m_pendingSeek;
    qCDebug(kastsAudio) << "Current position" << position;

    // Check if we're supposed to skip to a new position
    if (d->m_pendingSeek != -1 && d->m_player.mediaStatus() == KMediaSession::BufferedMedia && d->m_player.duration() > 0) {
        if (abs(d->m_pendingSeek - position) > 2000) {
            qCDebug(kastsAudio) << "Position seek still pending to position" << d->m_pendingSeek;
            qCDebug(kastsAudio) << "Current reported position and duration" << d->m_player.position() << d->m_player.duration();
            // be very careful because this setPosition call will trigger
            // a positionChanged signal, which will be nested, so we call it in
            // a QTimer::singleShot
            if (playbackState() == KMediaSession::PlaybackState::PlayingState) {
                qint64 seekPosition = d->m_pendingSeek;
                QTimer::singleShot(0, this, [this, seekPosition]() {
                    d->m_player.setPosition(seekPosition);
                });
            }
        } else {
            qCDebug(kastsAudio) << "Pending position seek has been executed; to position" << d->m_pendingSeek;
            d->m_pendingSeek = -1;
            // d->m_player.setNotifyInterval(1000);
        }
    }
}

void AudioManager::updateMetaData()
{
    // set metadata for MPRIS2
    if (!d->m_entry->title().isEmpty()) {
        d->m_player.metaData()->setTitle(d->m_entry->title());
    }
    // TODO: set URL??  d->m_entry->enclosure()->path();
    if (!d->m_entry->feed()->name().isEmpty()) {
        d->m_player.metaData()->setAlbum(d->m_entry->feed()->name());
    }
    if (d->m_entry->authors().count() > 0) {
        QString authors;
        for (auto &author : d->m_entry->authors())
            authors.append(author->name());
        d->m_player.metaData()->setArtist(authors);
    }
    if (!d->m_entry->image().isEmpty()) {
        d->m_player.metaData()->setArtworkUrl(QUrl(d->m_entry->cachedImage()));
    }
}

QString AudioManager::formattedDuration() const
{
    return m_kformat.formatDuration(duration());
}

QString AudioManager::formattedLeftDuration() const
{
    qreal rate = 1.0;
    if (SettingsManager::self()->adjustTimeLeft()) {
        rate = playbackRate();
        rate = (rate > 0.0) ? rate : 1.0;
    }
    qint64 diff = duration() - position();
    return m_kformat.formatDuration(diff / rate);
}

QString AudioManager::formattedPosition() const
{
    return m_kformat.formatDuration(position());
}

qint64 AudioManager::sleepTime() const
{
    if (d->m_sleepTimer) {
        return d->m_sleepTime;
    } else {
        return -1;
    }
}

qint64 AudioManager::remainingSleepTime() const
{
    if (d->m_sleepTimer) {
        return d->m_remainingSleepTime;
    } else {
        return -1;
    }
}

void AudioManager::setSleepTimer(qint64 duration)
{
    if (duration > 0) {
        if (d->m_sleepTimer) {
            stopSleepTimer();
        }

        d->m_sleepTime = duration;
        d->m_remainingSleepTime = duration;

        d->m_sleepTimer = new QTimer(this);
        connect(d->m_sleepTimer, &QTimer::timeout, this, [this]() {
            (d->m_remainingSleepTime)--;
            if (d->m_remainingSleepTime > 0) {
                Q_EMIT remainingSleepTimeChanged(remainingSleepTime());
            } else {
                pause();
                stopSleepTimer();
            }
        });
        d->m_sleepTimer->start(1000);

        Q_EMIT sleepTimerChanged(duration);
        Q_EMIT remainingSleepTimeChanged(remainingSleepTime());
    } else {
        stopSleepTimer();
    }
}

void AudioManager::stopSleepTimer()
{
    if (d->m_sleepTimer) {
        d->m_sleepTime = -1;
        d->m_remainingSleepTime = -1;

        delete d->m_sleepTimer;
        d->m_sleepTimer = nullptr;

        Q_EMIT sleepTimerChanged(-1);
        Q_EMIT remainingSleepTimeChanged(-1);
    }
}

QString AudioManager::formattedRemainingSleepTime() const
{
    qint64 timeLeft = remainingSleepTime() * 1000;
    if (timeLeft < 0) {
        timeLeft = 0;
    }
    return m_kformat.formatDuration(timeLeft);
}
