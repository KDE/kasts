/**
 * SPDX-FileCopyrightText: 2022-2023 Bart De Vries <bart@mogwai.be>
 * SPDX-FileCopyrightText: 2017 Matthieu Gallien <matthieu_gallien@yahoo.fr>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "vlcmediabackend.h"
#include "vlcmediabackendlogging.h"
#include "vlcsignalslogging.h"

#include <QAudio>
#include <QDir>
#include <QGuiApplication>
#include <QTimer>

#include "kmediasession.h"

#if defined Q_OS_WIN
#include <basetsd.h>
typedef SSIZE_T ssize_t;
#endif

#include <vlc/vlc.h>

class VlcMediaBackendPrivate
{
public:
    KMediaSession *mKMediaSession = nullptr;

    VlcMediaBackend *mParent = nullptr;

    libvlc_instance_t *mInstance = nullptr;

    libvlc_media_player_t *mPlayer = nullptr;

    libvlc_event_manager_t *mPlayerEventManager = nullptr;
    libvlc_event_manager_t *mMediaEventManager = nullptr;

    libvlc_media_t *mMedia = nullptr;

    qint64 mMediaDuration = 0;

    KMediaSession::PlaybackState mPreviousPlayerState = KMediaSession::StoppedState;

    KMediaSession::MediaStatus mPreviousMediaStatus = KMediaSession::NoMedia;

    qreal mPreviousVolume = 100.0;

    qint64 mPreviousPosition = 0;

    KMediaSession::Error mError = KMediaSession::NoError;

    bool mIsMuted = false;

    bool mIsSeekable = false;

    qreal mPlaybackRate = 1.0;

    void vlcEventCallback(const struct libvlc_event_t *p_event);

    void mediaIsEnded();

    bool signalPlaybackChange(KMediaSession::PlaybackState newPlayerState);

    void signalMediaStatusChange(KMediaSession::MediaStatus newMediaStatus);

    void signalVolumeChange(int newVolume);

    void signalMutedChange(bool isMuted);

    void signalDurationChange(libvlc_time_t newDuration);

    void signalPositionChange(float newPosition);

    void signalSeekableChange(bool isSeekable);

    void signalErrorChange(KMediaSession::Error errorCode);

    void parseMetaData();
};

static void vlc_callback(const struct libvlc_event_t *p_event, void *p_data)
{
    reinterpret_cast<VlcMediaBackendPrivate *>(p_data)->vlcEventCallback(p_event);
}

VlcMediaBackend::VlcMediaBackend(QObject *parent)
    : AbstractMediaBackend(parent)
    , d(std::make_unique<VlcMediaBackendPrivate>())
{
    qCDebug(VlcMediaBackendLog) << "VlcMediaBackend::VlcMediaBackend";
    d->mKMediaSession = static_cast<KMediaSession *>(parent);
    d->mParent = this;

    // TODO: handle video playback
    const char *cmdLineOption = "--no-video";
    d->mInstance = libvlc_new(1, &cmdLineOption);

    libvlc_set_user_agent(d->mInstance, d->mKMediaSession->playerName().toUtf8().constData(), d->mKMediaSession->playerName().toUtf8().constData());
    libvlc_set_app_id(d->mInstance, d->mKMediaSession->desktopEntryName().toUtf8().constData(), "1.0", d->mKMediaSession->playerName().toUtf8().constData());

    connect(d->mKMediaSession, &KMediaSession::playerNameChanged, this, &VlcMediaBackend::setPlayerName);
    connect(d->mKMediaSession, &KMediaSession::desktopEntryNameChanged, this, &VlcMediaBackend::setDesktopEntryName);

    d->mPlayer = libvlc_media_player_new(d->mInstance);

    if (!d->mPlayer) {
        qCDebug(VlcMediaBackendLog) << "VlcMediaBackend::VlcMediaBackend"
                                    << "failed creating player" << libvlc_errmsg();
        return;
    }

    d->mPlayerEventManager = libvlc_media_player_event_manager(d->mPlayer);

    libvlc_event_attach(d->mPlayerEventManager, libvlc_MediaPlayerOpening, &vlc_callback, d.get());
    libvlc_event_attach(d->mPlayerEventManager, libvlc_MediaPlayerBuffering, &vlc_callback, d.get());
    libvlc_event_attach(d->mPlayerEventManager, libvlc_MediaPlayerPlaying, &vlc_callback, d.get());
    libvlc_event_attach(d->mPlayerEventManager, libvlc_MediaPlayerPaused, &vlc_callback, d.get());
    libvlc_event_attach(d->mPlayerEventManager, libvlc_MediaPlayerStopped, &vlc_callback, d.get());
    libvlc_event_attach(d->mPlayerEventManager, libvlc_MediaPlayerEndReached, &vlc_callback, d.get());
    libvlc_event_attach(d->mPlayerEventManager, libvlc_MediaPlayerEncounteredError, &vlc_callback, d.get());
    libvlc_event_attach(d->mPlayerEventManager, libvlc_MediaPlayerPositionChanged, &vlc_callback, d.get());
    libvlc_event_attach(d->mPlayerEventManager, libvlc_MediaPlayerSeekableChanged, &vlc_callback, d.get());
    libvlc_event_attach(d->mPlayerEventManager, libvlc_MediaPlayerLengthChanged, &vlc_callback, d.get());
    libvlc_event_attach(d->mPlayerEventManager, libvlc_MediaPlayerMuted, &vlc_callback, d.get());
    libvlc_event_attach(d->mPlayerEventManager, libvlc_MediaPlayerUnmuted, &vlc_callback, d.get());
    libvlc_event_attach(d->mPlayerEventManager, libvlc_MediaPlayerAudioVolume, &vlc_callback, d.get());
    libvlc_event_attach(d->mPlayerEventManager, libvlc_MediaPlayerAudioDevice, &vlc_callback, d.get());
}

VlcMediaBackend::~VlcMediaBackend()
{
    qCDebug(VlcMediaBackendLog) << "VlcMediaBackend::~VlcMediaBackend";
    if (d->mInstance) {
        if (d->mPlayer && d->mPreviousPlayerState != KMediaSession::StoppedState) {
            libvlc_media_player_stop(d->mPlayer);
        }
        libvlc_release(d->mInstance);
    }
}

KMediaSession::MediaBackends VlcMediaBackend::backend() const
{
    qCDebug(VlcMediaBackendLog) << "VlcMediaBackend::backend()";
    return KMediaSession::MediaBackends::Vlc;
}

bool VlcMediaBackend::muted() const
{
    qCDebug(VlcMediaBackendLog) << "VlcMediaBackend::muted()";
    if (!d->mPlayer) {
        return false;
    }

    qCDebug(VlcMediaBackendLog) << "muted" << d->mIsMuted;
    return d->mIsMuted;
}

qreal VlcMediaBackend::volume() const
{
    qCDebug(VlcMediaBackendLog) << "VlcMediaBackend::volume()";
    if (!d->mPlayer) {
        return 100.0;
    }

    qCDebug(VlcMediaBackendLog) << "volume" << d->mPreviousVolume;
    return d->mPreviousVolume;
}

QUrl VlcMediaBackend::source() const
{
    qCDebug(VlcMediaBackendLog) << "VlcMediaBackend::source()";
    if (!d->mPlayer) {
        return {};
    }
    if (d->mMedia) {
        auto filePath = QString::fromUtf8(libvlc_media_get_mrl(d->mMedia));
        return QUrl::fromUserInput(filePath);
    }
    return {};
}

KMediaSession::Error VlcMediaBackend::error() const
{
    qCDebug(VlcMediaBackendLog) << "VlcMediaBackend::error()";
    return d->mError;
}

qint64 VlcMediaBackend::duration() const
{
    qCDebug(VlcMediaBackendLog) << "VlcMediaBackend::duration()";
    return d->mMediaDuration;
}

qint64 VlcMediaBackend::position() const
{
    qCDebug(VlcMediaBackendLog) << "VlcMediaBackend::position()";
    if (!d->mPlayer) {
        return 0;
    }

    if (d->mMediaDuration == -1) {
        return 0;
    }

    qint64 currentPosition = qRound64(libvlc_media_player_get_position(d->mPlayer) * d->mMediaDuration);

    if (currentPosition < 0) {
        return 0;
    }

    return currentPosition;
}

qreal VlcMediaBackend::playbackRate() const
{
    qCDebug(VlcMediaBackendLog) << "VlcMediaBackend::playbackRate()";
    if (d->mPlayer) {
        return libvlc_media_player_get_rate(d->mPlayer);
    }
    return 1.0;
}

bool VlcMediaBackend::seekable() const
{
    qCDebug(VlcMediaBackendLog) << "VlcMediaBackend::seekable()";
    return d->mIsSeekable;
}

KMediaSession::PlaybackState VlcMediaBackend::playbackState() const
{
    qCDebug(VlcMediaBackendLog) << "VlcMediaBackend::playbackState()";
    return d->mPreviousPlayerState;
}

KMediaSession::MediaStatus VlcMediaBackend::mediaStatus() const
{
    qCDebug(VlcMediaBackendLog) << "VlcMediaBackend::mediaStatus()";
    return d->mPreviousMediaStatus;
}

void VlcMediaBackend::setMuted(bool muted)
{
    qCDebug(VlcMediaBackendLog) << "VlcMediaBackend::setMuted(" << muted << ")";

    if (d->mPlayer) {
        libvlc_audio_set_mute(d->mPlayer, muted);
    } else {
        d->mIsMuted = muted;
        Q_EMIT mutedChanged(muted);
    }
}

void VlcMediaBackend::setVolume(qreal volume)
{
    qCDebug(VlcMediaBackendLog) << "VlcMediaBackend::setVolume(" << volume << ")";

    if (d->mPlayer && d->mPreviousPlayerState != KMediaSession::PlaybackState::StoppedState) {
        libvlc_audio_set_volume(d->mPlayer, qRound(volume));
    }
}

void VlcMediaBackend::setSource(const QUrl &source)
{
    if (playbackState() != KMediaSession::PlaybackState::StoppedState) {
        stop();
    }

    d->mMediaDuration = 0;
    d->mIsSeekable = false;
    d->mPlaybackRate = 1.0;
    d->mPreviousPosition = 0;
    d->mPreviousPlayerState = KMediaSession::PlaybackState::StoppedState;

    qCDebug(VlcMediaBackendLog) << "VlcMediaBackend::setSource(" << source << ")";
    if (source.isLocalFile()) {
        qCDebug(VlcMediaBackendLog) << "VlcMediaBackend::setSource reading local resource";
        d->mMedia = libvlc_media_new_path(d->mInstance, QDir::toNativeSeparators(source.toLocalFile()).toUtf8().constData());
    } else {
        qCDebug(VlcMediaBackendLog) << "VlcMediaBackend::setSource reading remote resource";
        d->mMedia = libvlc_media_new_location(d->mInstance, source.url().toUtf8().constData());
    }

    if (!d->mMedia) {
        qCDebug(VlcMediaBackendLog) << "VlcMediaBackend::setSource"
                                    << "failed creating media" << libvlc_errmsg() << QDir::toNativeSeparators(source.toLocalFile()).toUtf8().constData();

        d->mMedia = libvlc_media_new_path(d->mInstance, QDir::toNativeSeparators(source.toLocalFile()).toLatin1().constData());
        if (!d->mMedia) {
            qCDebug(VlcMediaBackendLog) << "VlcMediaBackend::setSource"
                                        << "failed creating media" << libvlc_errmsg() << QDir::toNativeSeparators(source.toLocalFile()).toLatin1().constData();
            return;
        }
    }

    // By default, libvlc caches only next 1000 (ms, 0..60000) of the playback,
    // which is unreasonable given our usecase of sequential playback.
    libvlc_media_add_option(d->mMedia, ":file-caching=10000");
    libvlc_media_add_option(d->mMedia, ":live-caching=10000");
    libvlc_media_add_option(d->mMedia, ":disc-caching=10000");
    libvlc_media_add_option(d->mMedia, ":network-caching=10000");

    libvlc_media_player_set_media(d->mPlayer, d->mMedia);

    d->signalMediaStatusChange(KMediaSession::LoadingMedia);
    d->signalMediaStatusChange(KMediaSession::LoadedMedia);
    d->signalMediaStatusChange(KMediaSession::BufferedMedia);

    d->mMediaEventManager = libvlc_media_event_manager(d->mMedia);

    libvlc_event_attach(d->mMediaEventManager, libvlc_MediaParsedChanged, &vlc_callback, d.get());
    libvlc_event_attach(d->mMediaEventManager, libvlc_MediaDurationChanged, &vlc_callback, d.get());

    libvlc_media_parse_with_options(
        d->mMedia,
        static_cast<libvlc_media_parse_flag_t>(libvlc_media_parse_local | libvlc_media_parse_network | libvlc_media_fetch_local | libvlc_media_fetch_network),
        0);

    qCDebug(VlcMediaBackendLog) << "VlcMediaBackend::sourceChanged(" << source << ")";
    QTimer::singleShot(0, this, [this, source]() {
        Q_EMIT sourceChanged(source);
    });
}

void VlcMediaBackend::setPosition(qint64 position)
{
    qCDebug(VlcMediaBackendLog) << "VlcMediaBackend::setPosition(" << position << ")";

    if (!d->mPlayer) {
        return;
    }

    if (d->mMediaDuration == -1 || d->mMediaDuration == 0) {
        return;
    }

    libvlc_media_player_set_position(d->mPlayer, static_cast<float>(position) / d->mMediaDuration);
}

void VlcMediaBackend::setPlaybackRate(qreal rate)
{
    qCDebug(VlcMediaBackendLog) << "VlcMediaBackend::setPlaybackRate(" << rate << ")";
    if (d->mPlayer) {
        if (libvlc_media_player_set_rate(d->mPlayer, static_cast<float>(rate)) == 0) {
            d->mPlaybackRate = rate;
            QTimer::singleShot(0, this, [this, rate]() {
                Q_EMIT playbackRateChanged(rate);
            });
        }
    }
}

void VlcMediaBackend::play()
{
    qCDebug(VlcMediaBackendLog) << "VlcMediaBackend::play()";
    if (!d->mPlayer) {
        return;
    }

    libvlc_media_player_play(d->mPlayer);
}

void VlcMediaBackend::pause()
{
    qCDebug(VlcMediaBackendLog) << "VlcMediaBackend::pause()";
    if (!d->mPlayer) {
        return;
    }

    // need to check playback state first, because the pause function will
    // actually toggle pause, i.e. it will start playing when the current track
    // has already been paused
    if (playbackState() == KMediaSession::PlaybackState::PlayingState) {
        libvlc_media_player_pause(d->mPlayer);
    }
}

void VlcMediaBackend::stop()
{
    qCDebug(VlcMediaBackendLog) << "VlcMediaBackend::stop()";
    if (!d->mPlayer) {
        return;
    }

    d->mIsSeekable = false;
    QTimer::singleShot(0, this, [this]() {
        Q_EMIT seekableChanged(d->mIsSeekable);
    });

    libvlc_media_player_stop(d->mPlayer);
}

void VlcMediaBackend::playerStateSignalChanges(KMediaSession::PlaybackState newState)
{
    qCDebug(VlcMediaBackendLog) << "VlcMediaBackend::playerStateSignalChanges(" << newState << ")";
    QTimer::singleShot(0, this, [this, newState]() {
        Q_EMIT playbackStateChanged(newState);
        if (newState == KMediaSession::PlaybackState::StoppedState) {
            Q_EMIT positionChanged(position());
        } else {
            Q_EMIT mutedChanged(muted());
        }
    });
}

void VlcMediaBackend::mediaStatusSignalChanges(KMediaSession::MediaStatus newStatus)
{
    qCDebug(VlcMediaBackendLog) << "VlcMediaBackend::mediaStatusSignalChanges(" << newStatus << ")";
    QTimer::singleShot(0, this, [this, newStatus]() {
        Q_EMIT mediaStatusChanged(newStatus);
    });
}

void VlcMediaBackend::playerErrorSignalChanges(KMediaSession::Error error)
{
    qCDebug(VlcMediaBackendLog) << "VlcMediaBackend::playerErrorSignalChanges(" << error << ")";
    QTimer::singleShot(0, this, [this, error]() {
        Q_EMIT errorChanged(error);
    });
}

void VlcMediaBackend::playerDurationSignalChanges(qint64 newDuration)
{
    qCDebug(VlcMediaBackendLog) << "VlcMediaBackend::playerDurationSignalChanges(" << newDuration << ")";
    d->mMediaDuration = newDuration;
    QTimer::singleShot(0, this, [this, newDuration]() {
        Q_EMIT durationChanged(newDuration);
    });
}

void VlcMediaBackend::playerPositionSignalChanges(qint64 newPosition)
{
    qCDebug(VlcMediaBackendLog) << "VlcMediaBackend::playerPositionSignalChanges(" << newPosition << ")";
    QTimer::singleShot(0, this, [this, newPosition]() {
        Q_EMIT positionChanged(newPosition);
    });
}

void VlcMediaBackend::playerVolumeSignalChanges(qreal volume)
{
    qCDebug(VlcMediaBackendLog) << "VlcMediaBackend::playerVolumeSignalChanges(" << volume << ")";
    QTimer::singleShot(0, this, [this, volume]() {
        Q_EMIT volumeChanged(volume);
    });
}

void VlcMediaBackend::playerMutedSignalChanges(bool isMuted)
{
    qCDebug(VlcMediaBackendLog) << "VlcMediaBackend::playerMutedSignalChanges(" << isMuted << ")";
    QTimer::singleShot(0, this, [this, isMuted]() {
        Q_EMIT mutedChanged(isMuted);
    });
}

void VlcMediaBackend::playerSeekableSignalChanges(bool isSeekable)
{
    qCDebug(VlcMediaBackendLog) << "VlcMediaBackend::playerSeekableSignalChanges(" << isSeekable << ")";
    QTimer::singleShot(0, this, [this, isSeekable]() {
        Q_EMIT seekableChanged(isSeekable);
    });
}

void VlcMediaBackend::setPlayerName(const QString &name)
{
    qCDebug(VlcMediaBackendLog) << "VlcMediaBackend::setPlayerName(" << name << ")";
    libvlc_set_user_agent(d->mInstance, name.toUtf8().constData(), name.toUtf8().constData());
    libvlc_set_app_id(d->mInstance, d->mKMediaSession->desktopEntryName().toUtf8().constData(), "1.0", name.toUtf8().constData());
}

void VlcMediaBackend::setDesktopEntryName(const QString &name)
{
    qCDebug(VlcMediaBackendLog) << "VlcMediaBackend::setDesktopEntryName(" << name << ")";
    libvlc_set_app_id(d->mInstance, name.toUtf8().constData(), "1.0", d->mKMediaSession->playerName().toUtf8().constData());
}

void VlcMediaBackendPrivate::vlcEventCallback(const struct libvlc_event_t *p_event)
{
    qCDebug(VlcSignalsLog) << "VlcMediaBackendPrivate::vlcEventCallback()";
    const auto eventType = static_cast<libvlc_event_e>(p_event->type);

    switch (eventType) {
    case libvlc_MediaPlayerOpening:
        qCDebug(VlcSignalsLog) << "VlcMediaBackendPrivate::vlcEventCallback"
                               << "libvlc_MediaPlayerOpening";
        signalMediaStatusChange(KMediaSession::LoadedMedia);
        break;
    case libvlc_MediaPlayerBuffering:
        qCDebug(VlcSignalsLog) << "VlcMediaBackendPrivate::vlcEventCallback"
                               << "libvlc_MediaPlayerBuffering";
        signalMediaStatusChange(KMediaSession::BufferedMedia);
        break;
    case libvlc_MediaPlayerPlaying:
        qCDebug(VlcSignalsLog) << "VlcMediaBackendPrivate::vlcEventCallback"
                               << "libvlc_MediaPlayerPlaying";
        signalPlaybackChange(KMediaSession::PlayingState);
        break;
    case libvlc_MediaPlayerPaused:
        qCDebug(VlcSignalsLog) << "VlcMediaBackendPrivate::vlcEventCallback"
                               << "libvlc_MediaPlayerPaused";
        signalPlaybackChange(KMediaSession::PausedState);
        break;
    case libvlc_MediaPlayerStopped:
        qCDebug(VlcSignalsLog) << "VlcMediaBackendPrivate::vlcEventCallback"
                               << "libvlc_MediaPlayerStopped";
        signalPlaybackChange(KMediaSession::StoppedState);
        break;
    case libvlc_MediaPlayerEndReached:
        qCDebug(VlcSignalsLog) << "VlcMediaBackendPrivate::vlcEventCallback"
                               << "libvlc_MediaPlayerEndReached";
        signalMediaStatusChange(KMediaSession::BufferedMedia);
        signalMediaStatusChange(KMediaSession::NoMedia);
        signalMediaStatusChange(KMediaSession::EndOfMedia);
        mediaIsEnded();
        break;
    case libvlc_MediaPlayerEncounteredError:
        qCDebug(VlcSignalsLog) << "VlcMediaBackendPrivate::vlcEventCallback"
                               << "libvlc_MediaPlayerEncounteredError";
        signalErrorChange(KMediaSession::ResourceError);
        mediaIsEnded();
        signalMediaStatusChange(KMediaSession::InvalidMedia);
        break;
    case libvlc_MediaPlayerPositionChanged:
        qCDebug(VlcSignalsLog) << "VlcMediaBackendPrivate::vlcEventCallback"
                               << "libvlc_MediaPlayerPositionChanged";
        signalPositionChange(p_event->u.media_player_position_changed.new_position);
        break;
    case libvlc_MediaPlayerSeekableChanged:
        qCDebug(VlcSignalsLog) << "VlcMediaBackendPrivate::vlcEventCallback"
                               << "libvlc_MediaPlayerSeekableChanged";
        signalSeekableChange(p_event->u.media_player_seekable_changed.new_seekable);
        break;
    case libvlc_MediaPlayerLengthChanged:
        qCDebug(VlcSignalsLog) << "VlcMediaBackendPrivate::vlcEventCallback"
                               << "libvlc_MediaPlayerLengthChanged";
        signalDurationChange(p_event->u.media_player_length_changed.new_length);
        break;
    case libvlc_MediaPlayerMuted:
        qCDebug(VlcSignalsLog) << "VlcMediaBackendPrivate::vlcEventCallback"
                               << "libvlc_MediaPlayerMuted";
        signalMutedChange(true);
        break;
    case libvlc_MediaPlayerUnmuted:
        qCDebug(VlcSignalsLog) << "VlcMediaBackendPrivate::vlcEventCallback"
                               << "libvlc_MediaPlayerUnmuted";
        signalMutedChange(false);
        break;
    case libvlc_MediaPlayerAudioVolume:
        qCDebug(VlcSignalsLog) << "VlcMediaBackendPrivate::vlcEventCallback"
                               << "libvlc_MediaPlayerAudioVolume";
        signalVolumeChange(qRound(p_event->u.media_player_audio_volume.volume * 100));
        break;
    case libvlc_MediaPlayerAudioDevice:
        qCDebug(VlcSignalsLog) << "VlcMediaBackendPrivate::vlcEventCallback"
                               << "libvlc_MediaPlayerAudioDevice";
        break;
    case libvlc_MediaDurationChanged:
        qCDebug(VlcSignalsLog) << "VlcMediaBackendPrivate::vlcEventCallback"
                               << "libvlc_MediaDurationChanged";
        signalDurationChange(p_event->u.media_duration_changed.new_duration);
        break;
    case libvlc_MediaParsedChanged:
        if (p_event->u.media_parsed_changed.new_status == libvlc_media_parsed_status_done) {
            parseMetaData();
        }
        break;
    default:
        qCDebug(VlcSignalsLog) << "VlcMediaBackendPrivate::vlcEventCallback"
                               << "eventType" << eventType;
        break;
    }
}

void VlcMediaBackendPrivate::mediaIsEnded()
{
    qCDebug(VlcSignalsLog) << "VlcMediaBackendPrivate::mediaIsEnded()";

    mIsSeekable = false;
    Q_EMIT mParent->seekableChanged(mIsSeekable);

    libvlc_media_release(mMedia);
    mMedia = nullptr;
}

bool VlcMediaBackendPrivate::signalPlaybackChange(KMediaSession::PlaybackState newPlayerState)
{
    qCDebug(VlcSignalsLog) << "VlcMediaBackendPrivate::signalPlaybackChange(" << newPlayerState << ")";
    if (mPreviousPlayerState != newPlayerState) {
        mPreviousPlayerState = newPlayerState;

        mParent->playerStateSignalChanges(mPreviousPlayerState);
        return true;
    }

    return false;
}

void VlcMediaBackendPrivate::signalMediaStatusChange(KMediaSession::MediaStatus newMediaStatus)
{
    qCDebug(VlcSignalsLog) << "VlcMediaBackendPrivate::signalMediaStatusChange(" << newMediaStatus << ")";
    if (mPreviousMediaStatus != newMediaStatus) {
        mPreviousMediaStatus = newMediaStatus;

        mParent->mediaStatusSignalChanges(mPreviousMediaStatus);
    }
}

void VlcMediaBackendPrivate::signalVolumeChange(int newVolume)
{
    qCDebug(VlcSignalsLog) << "VlcMediaBackendPrivate::signalVolumeChange(" << newVolume << ")";
    if (newVolume == -100) {
        return;
    }

    if (abs(int(mPreviousVolume - newVolume)) > 0.01) {
        mPreviousVolume = newVolume;

        mParent->playerVolumeSignalChanges(newVolume);
    }
}

void VlcMediaBackendPrivate::signalMutedChange(bool isMuted)
{
    qCDebug(VlcSignalsLog) << "VlcMediaBackendPrivate::signalMutedChange(" << isMuted << ")";
    if (mIsMuted != isMuted) {
        mIsMuted = isMuted;

        mParent->playerMutedSignalChanges(mIsMuted);
    }
}

void VlcMediaBackendPrivate::signalDurationChange(libvlc_time_t newDuration)
{
    qCDebug(VlcSignalsLog) << "VlcMediaBackendPrivate::signalDurationChange(" << newDuration << ")";
    if (mMediaDuration != newDuration) {
        mMediaDuration = newDuration;

        mParent->playerDurationSignalChanges(mMediaDuration);
    }
}

void VlcMediaBackendPrivate::signalPositionChange(float newPosition)
{
    qCDebug(VlcSignalsLog) << "VlcMediaBackendPrivate::signalPositionChange(" << newPosition << ")";
    if (mMediaDuration == -1) {
        return;
    }

    if (newPosition < 0) {
        mPreviousPosition = 0;
        mParent->playerPositionSignalChanges(mPreviousPosition);
        return;
    }

    auto computedPosition = qRound64(newPosition * mMediaDuration);

    if (mPreviousPosition != computedPosition) {
        mPreviousPosition = computedPosition;

        mParent->playerPositionSignalChanges(mPreviousPosition);
    }
}

void VlcMediaBackendPrivate::signalSeekableChange(bool isSeekable)
{
    qCDebug(VlcSignalsLog) << "VlcMediaBackendPrivate::signalSeekableChange(" << isSeekable << ")";
    if (mIsSeekable != isSeekable) {
        mIsSeekable = isSeekable;

        mParent->playerSeekableSignalChanges(isSeekable);
    }
}

void VlcMediaBackendPrivate::signalErrorChange(KMediaSession::Error errorCode)
{
    qCDebug(VlcSignalsLog) << "VlcMediaBackendPrivate::signalErrorChange(" << errorCode << ")";
    if (mError != errorCode) {
        mError = errorCode;

        mParent->playerErrorSignalChanges(errorCode);
    }
}

void VlcMediaBackendPrivate::parseMetaData()
{
    qCDebug(VlcSignalsLog) << "VlcMediaBackendPrivate::parseMetaData()";
    if (mMedia && mKMediaSession->metaData()->title().isEmpty()) {
        mKMediaSession->metaData()->setTitle(QString::fromUtf8(libvlc_media_get_meta(mMedia, libvlc_meta_Title)));
    }
    if (mMedia && mKMediaSession->metaData()->artist().isEmpty()) {
        mKMediaSession->metaData()->setArtist(QString::fromUtf8(libvlc_media_get_meta(mMedia, libvlc_meta_Artist)));
    }
    if (mMedia && mKMediaSession->metaData()->album().isEmpty()) {
        mKMediaSession->metaData()->setAlbum(QString::fromUtf8(libvlc_media_get_meta(mMedia, libvlc_meta_Album)));
    }
    if (mMedia && mKMediaSession->metaData()->artworkUrl().isEmpty()) {
        mKMediaSession->metaData()->setArtworkUrl(QUrl(QString::fromUtf8(libvlc_media_get_meta(mMedia, libvlc_meta_ArtworkURL))));
    }
}
