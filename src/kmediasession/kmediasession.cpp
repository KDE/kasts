/**
 * SPDX-FileCopyrightText: 2022-2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#include "kmediasession.h"
#include "kmediasessionlogging.h"

#include <QDebug>
#include <QTimer>
#include <QUrl>

#include <KAboutData>

#include "config-kmediasession.h"
#include "mediabackends/abstractmediabackend.h"
#include "mediabackends/qtmediabackend.h"
#include "mpris2/mpris2.h"
#include "powermanagement/powermanagementinterface.h"
#ifdef HAVE_LIBVLC
#include "mediabackends/vlcmediabackend.h"
#endif
#ifdef HAVE_GST
#include "mediabackends/gstmediabackend.h"
#endif
#ifdef HAVE_MPVQT
#include "mediabackends/mpvmediabackend.h"
#endif

class KMediaSessionPrivate
{
private:
    friend class KMediaSession;

    QHash<KMediaSession::MediaBackends, QString> m_availableBackends{
        {KMediaSession::MediaBackends::Qt, QStringLiteral("Qt Multimedia")},
#ifdef HAVE_LIBVLC
        {KMediaSession::MediaBackends::Vlc, QStringLiteral("VLC player")},
#endif
#ifdef HAVE_GST
        {KMediaSession::MediaBackends::Gst, QStringLiteral("GStreamer")},
#endif
#ifdef HAVE_MPVQT
        {KMediaSession::MediaBackends::Mpv, QStringLiteral("Mpv")},
#endif
    };

    AbstractMediaBackend *m_player = nullptr;
    PowerManagementInterface mPowerInterface;
    std::unique_ptr<Mpris2> m_mpris;
    MetaData *m_meta = nullptr;

    QString m_playerName;
    QString m_desktopEntryName;
    bool m_mpris2PauseInsteadOfStop = false;
    bool m_canGoNext = false;
    bool m_canGoPrevious = false;
};

KMediaSession::KMediaSession(const QString &playerName, const QString &desktopEntryName, QObject *parent)
    : QObject(parent)
    , d(std::make_unique<KMediaSessionPrivate>())
{
    qCDebug(KMediaSessionLog) << "KMediaSession::KMediaSesion begin";

    // set up metadata
    d->m_meta = new MetaData(this);
    connect(d->m_meta, &MetaData::metaDataChanged, this, &KMediaSession::metaDataChanged);

#ifdef HAVE_LIBVLC
    setCurrentBackend(KMediaSession::MediaBackends::Vlc);
#else
#ifdef HAVE_GST
    setCurrentBackend(KMediaSession::MediaBackends::Gst);
#else
    setCurrentBackend(KMediaSession::MediaBackends::Qt);
#endif
#endif

    // set up mpris2
    d->m_playerName = playerName.isEmpty()
        ? (KAboutData::applicationData().displayName().isEmpty() ? QStringLiteral("KMediaSession") : KAboutData::applicationData().displayName())
        : playerName;
    d->m_desktopEntryName = desktopEntryName.isEmpty()
        ? (KAboutData::applicationData().desktopFileName().isEmpty() ? QStringLiteral("org.kde.kmediasession")
                                                                     : KAboutData::applicationData().desktopFileName())
        : desktopEntryName;
    d->m_mpris = std::make_unique<Mpris2>(this);

    qCDebug(KMediaSessionLog) << "KMediaSession::KMediaSession end";
}

KMediaSession::~KMediaSession()
{
    qCDebug(KMediaSessionLog) << "KMediaSession::~KMediaSession";

    d->mPowerInterface.setPreventSleep(false);

    if (d->m_player) {
        delete d->m_player;
    }

    if (d->m_meta) {
        delete d->m_meta;
    }
}

QString KMediaSession::backendName(KMediaSession::MediaBackends backend) const
{
    qCDebug(KMediaSessionLog) << "KMediaSession::backendName()";
    if (d->m_availableBackends.contains(backend)) {
        return d->m_availableBackends[backend];
    }
    return QString();
}

KMediaSession::MediaBackends KMediaSession::currentBackend() const
{
    qCDebug(KMediaSessionLog) << "KMediaSession::currentBackend()";
    return d->m_player->backend();
}

QList<KMediaSession::MediaBackends> KMediaSession::availableBackends() const
{
    qCDebug(KMediaSessionLog) << "KMediaSession::availableBackends()";
    return d->m_availableBackends.keys();
}

QString KMediaSession::playerName() const
{
    qCDebug(KMediaSessionLog) << "KMediaSession::playerName()";
    return d->m_playerName;
}

QString KMediaSession::desktopEntryName() const
{
    qCDebug(KMediaSessionLog) << "KMediaSession::desktopEntryName()";
    return d->m_desktopEntryName;
}

bool KMediaSession::mpris2PauseInsteadOfStop() const
{
    qCDebug(KMediaSessionLog) << "KMediaSession::mpris2PauseInsteadOfStop()";
    return d->m_mpris2PauseInsteadOfStop;
}

bool KMediaSession::muted() const
{
    qCDebug(KMediaSessionLog) << "KMediaSession::muted()";
    if (d->m_player) {
        return d->m_player->muted();
    }
    return false;
}

qreal KMediaSession::volume() const
{
    qCDebug(KMediaSessionLog) << "KMediaSession::volume()";
    if (d->m_player) {
        return d->m_player->volume();
    }
    return 1.0;
}

QUrl KMediaSession::source() const
{
    qCDebug(KMediaSessionLog) << "KMediaSession::source()";
    if (d->m_player) {
        return d->m_player->source();
    }
    return QUrl();
}

KMediaSession::MediaStatus KMediaSession::mediaStatus() const
{
    qCDebug(KMediaSessionLog) << "KMediaSession::mediaStatus()";
    if (d->m_player) {
        return d->m_player->mediaStatus();
    }
    return KMediaSession::MediaStatus::UnknownMediaStatus;
}

KMediaSession::PlaybackState KMediaSession::playbackState() const
{
    qCDebug(KMediaSessionLog) << "KMediaSession::playbackState()";
    if (d->m_player) {
        return d->m_player->playbackState();
    }
    return KMediaSession::PlaybackState::StoppedState;
}

qreal KMediaSession::playbackRate() const
{
    qCDebug(KMediaSessionLog) << "KMediaSession::playBackRate()";
    if (d->m_player) {
        return d->m_player->playbackRate();
    }
    return 1.0;
}

qreal KMediaSession::minimumPlaybackRate() const
{
    qCDebug(KMediaSessionLog) << "KMediaSession::minimumPlayBackRate()";
    return MIN_RATE;
}

qreal KMediaSession::maximumPlaybackRate() const
{
    qCDebug(KMediaSessionLog) << "KMediaSession::maximumPlayBackRate()";
    return MAX_RATE;
}

KMediaSession::Error KMediaSession::error() const
{
    qCDebug(KMediaSessionLog) << "KMediaSession::error()";
    if (d->m_player) {
        return d->m_player->error();
    }
    return KMediaSession::Error::NoError;
}

qint64 KMediaSession::duration() const
{
    qCDebug(KMediaSessionLog) << "KMediaSession::duration()";
    if (d->m_player) {
        return d->m_player->duration();
    }
    return 0;
}

qint64 KMediaSession::position() const
{
    qCDebug(KMediaSessionLog) << "KMediaSession::position()";
    if (d->m_player) {
        return d->m_player->position();
    }
    return 0;
}

bool KMediaSession::seekable() const
{
    qCDebug(KMediaSessionLog) << "KMediaSession::seekable()";
    if (d->m_player) {
        return d->m_player->seekable();
    }
    return false;
}

MetaData *KMediaSession::metaData() const
{
    qCDebug(KMediaSessionLog) << "KMediaSession::metaData()";
    if (d->m_meta) {
        return d->m_meta;
    }
    return nullptr;
}

bool KMediaSession::canPlay() const
{
    qCDebug(KMediaSessionLog) << "KMediaSession::canPlay()";
    if (d->m_player) {
        return !d->m_player->source().isEmpty();
    } else {
        return false;
    }
}

bool KMediaSession::canPause() const
{
    qCDebug(KMediaSessionLog) << "KMediaSession::canPause()";
    if (d->m_player) {
        return !d->m_player->source().isEmpty();
    } else {
        return false;
    }
}

bool KMediaSession::canGoNext() const
{
    qCDebug(KMediaSessionLog) << "KMediaSession::canGoNext()";
    return d->m_canGoNext;
}

bool KMediaSession::canGoPrevious() const
{
    qCDebug(KMediaSessionLog) << "KMediaSession::canGoPrevious()";
    return d->m_canGoPrevious;
}

void KMediaSession::setCurrentBackend(KMediaSession::MediaBackends backend)
{
    qCDebug(KMediaSessionLog) << "KMediaSession::setCurrentBackend(" << backend << ")";

    if (!d->m_availableBackends.contains(backend)) {
        return;
    }

    if (d->m_player) {
        stop();
        setSource(QUrl());
        delete d->m_player;
    }

    switch (backend) {
    case KMediaSession::MediaBackends::Qt:
        d->m_player = new QtMediaBackend(this);
        break;
#ifdef HAVE_LIBVLC
    case KMediaSession::MediaBackends::Vlc:
        d->m_player = new VlcMediaBackend(this);
        break;
#endif
#ifdef HAVE_GST
    case KMediaSession::MediaBackends::Gst:
        d->m_player = new GstMediaBackend(this);
        break;
#endif
#ifdef HAVE_MPVQT
    case KMediaSession::MediaBackends::Mpv:
        d->m_player = new MpvMediaBackend(this);
        break;
#endif
    };

    connect(d->m_player, &AbstractMediaBackend::mutedChanged, this, &KMediaSession::mutedChanged);
    connect(d->m_player, &AbstractMediaBackend::volumeChanged, this, &KMediaSession::volumeChanged);
    connect(d->m_player, &AbstractMediaBackend::sourceChanged, this, &KMediaSession::sourceChanged);
    connect(d->m_player, &AbstractMediaBackend::mediaStatusChanged, this, &KMediaSession::mediaStatusChanged);
    connect(d->m_player, &AbstractMediaBackend::playbackStateChanged, this, [this](KMediaSession::PlaybackState state) {
        switch (state) {
        case KMediaSession::PlaybackState::StoppedState:
            d->mPowerInterface.setPreventSleep(false);
            break;
        case KMediaSession::PlaybackState::PlayingState:
            d->mPowerInterface.setPreventSleep(true);
            break;
        case KMediaSession::PlaybackState::PausedState:
            d->mPowerInterface.setPreventSleep(false);
            break;
        }
        QTimer::singleShot(0, this, [this, state]() {
            Q_EMIT playbackStateChanged(state);
        });
    });
    connect(d->m_player, &AbstractMediaBackend::playbackRateChanged, this, &KMediaSession::playbackRateChanged);
    connect(d->m_player, &AbstractMediaBackend::errorChanged, this, &KMediaSession::errorChanged);
    connect(d->m_player, &AbstractMediaBackend::durationChanged, this, &KMediaSession::durationChanged);
    connect(d->m_player, &AbstractMediaBackend::positionChanged, this, &KMediaSession::positionChanged);
    connect(d->m_player, &AbstractMediaBackend::seekableChanged, this, &KMediaSession::seekableChanged);

    QTimer::singleShot(0, this, [this, backend]() {
        Q_EMIT currentBackendChanged(backend);
        Q_EMIT playbackStateChanged(playbackState());
        Q_EMIT mediaStatusChanged(mediaStatus());
        Q_EMIT errorChanged(error());
        Q_EMIT seekableChanged(seekable());
        Q_EMIT durationChanged(duration());
        Q_EMIT positionChanged(position());
        Q_EMIT mutedChanged(muted());
        Q_EMIT volumeChanged(volume());
        Q_EMIT sourceChanged(source());
    });
}

void KMediaSession::setPlayerName(const QString &name)
{
    qCDebug(KMediaSessionLog) << "KMediaSession::setPlayerName(" << name << ")";
    if (name != d->m_playerName) {
        d->m_playerName = name;
        Q_EMIT playerNameChanged(name);
    }
}

void KMediaSession::setDesktopEntryName(const QString &name)
{
    qCDebug(KMediaSessionLog) << "KMediaSession::setDesktopEntryName(" << name << ")";
    if (name != d->m_desktopEntryName) {
        d->m_desktopEntryName = name;
        Q_EMIT desktopEntryNameChanged(name);
    }
}

void KMediaSession::setMpris2PauseInsteadOfStop(bool newState)
{
    qCDebug(KMediaSessionLog) << "KMediaSession::setMpris2PauseInsteadOfStop(" << newState << ")";
    if (newState != d->m_mpris2PauseInsteadOfStop) {
        d->m_mpris2PauseInsteadOfStop = newState;
        Q_EMIT mpris2PauseInsteadOfStopChanged(newState);
    }
}

void KMediaSession::setMuted(bool muted)
{
    qCDebug(KMediaSessionLog) << "KMediaSession::setMuted(" << muted << ")";
    if (d->m_player) {
        d->m_player->setMuted(muted);
    }
}

void KMediaSession::setVolume(qreal volume)
{
    qCDebug(KMediaSessionLog) << "KMediaSession::setVolume(" << volume << ")";
    if (d->m_player) {
        d->m_player->setVolume(volume);
    }
}

void KMediaSession::setSource(const QUrl &source)
{
    qCDebug(KMediaSessionLog) << "KMediaSession::setSource(" << source << ")";
    if (d->m_player) {
        metaData()->clear();
        d->m_player->setSource(source);
        QTimer::singleShot(0, this, [this]() {
            Q_EMIT canPlayChanged(true);
            Q_EMIT canPauseChanged(true);
        });
    }
}

void KMediaSession::setPosition(qint64 position)
{
    qCDebug(KMediaSessionLog) << "KMediaSession::setPosition(" << position << ")";
    qCDebug(KMediaSessionLog) << "Seeking: " << position;
    if (d->m_player) {
        d->m_player->setPosition(position);
        QTimer::singleShot(0, this, [this, position]() {
            Q_EMIT positionChanged(position);
            Q_EMIT positionJumped(position);
        });
    }
}

void KMediaSession::setPlaybackRate(qreal rate)
{
    qCDebug(KMediaSessionLog) << "KMediaSession::setPlaybackRate(" << rate << ")";
    if (d->m_player) {
        qreal clippedRate = rate > MAX_RATE ? MAX_RATE : (rate < MIN_RATE ? MIN_RATE : rate);
        d->m_player->setPlaybackRate(clippedRate);
        QTimer::singleShot(0, this, [this, clippedRate]() {
            Q_EMIT playbackRateChanged(clippedRate);
        });
    }
}

void KMediaSession::setMetaData(MetaData *metaData)
{
    qCDebug(KMediaSessionLog) << "KMediaSession::setMetaData(" << metaData << ")";
    if (metaData && (metaData != d->m_meta)) {
        delete d->m_meta;
        d->m_meta = metaData;
        connect(d->m_meta, &MetaData::metaDataChanged, this, &KMediaSession::metaDataChanged);
        Q_EMIT d->m_meta->metaDataChanged(d->m_meta);
    }
}

void KMediaSession::setCanGoNext(bool newState)
{
    qCDebug(KMediaSessionLog) << "KMediaSession::setCanGoNext(" << newState << ")";
    if (newState != d->m_canGoNext) {
        d->m_canGoNext = newState;
        Q_EMIT canGoNextChanged(d->m_canGoNext);
    }
}

void KMediaSession::setCanGoPrevious(bool newState)
{
    qCDebug(KMediaSessionLog) << "KMediaSession::setCanGoPrevious(" << newState << ")";
    if (newState != d->m_canGoPrevious) {
        d->m_canGoPrevious = newState;
        Q_EMIT canGoPreviousChanged(d->m_canGoPrevious);
    }
}

void KMediaSession::pause()
{
    qCDebug(KMediaSessionLog) << "KMediaSession::pause()";

    if (d->m_player && !source().isEmpty()) {
        d->m_player->pause();
        d->mPowerInterface.setPreventSleep(false);
    }
}

void KMediaSession::play()
{
    qCDebug(KMediaSessionLog) << "KMediaSession::play()";

    if (d->m_player && !source().isEmpty()) {
        d->m_player->play();
        d->mPowerInterface.setPreventSleep(true);
    }
}

void KMediaSession::stop()
{
    qCDebug(KMediaSessionLog) << "KMediaSession::stop()";

    if (d->m_player && !source().isEmpty()) {
        d->m_player->stop();
        d->mPowerInterface.setPreventSleep(false);
    }
}
