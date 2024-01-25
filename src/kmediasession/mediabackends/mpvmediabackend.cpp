/**
 * SPDX-FileCopyrightText: 2024 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#include "mpvmediabackend.h"
#include "mpvmediabackend_p.h"
#include "mpvmediabackendlogging.h"
#include "mpvsignalslogging.h"

#include <MpvAbstractItem>
#include <MpvController>

#include <QMap>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVariant>

#define LOG100 4.60517018599

MpvMediaBackend::MpvMediaBackend(QObject *parent)
    : AbstractMediaBackend(parent)
    , d(std::make_unique<MpvMediaBackendPrivate>())
{
    qCDebug(MpvMediaBackendLog) << "MpvMediaBackend::MpvMediaBackend";
    d->m_kmediaSession = static_cast<KMediaSession *>(parent);
    d->m_parent = this;
}

MpvMediaBackend::~MpvMediaBackend()
{
    qCDebug(MpvMediaBackendLog) << "MpvMediaBackend::~MpvMediaBackend";
}

KMediaSession::MediaBackends MpvMediaBackend::backend() const
{
    qCDebug(MpvMediaBackendLog) << "MpvMediaBackend::backend()";
    return KMediaSession::MediaBackends::Mpv;
}

bool MpvMediaBackend::muted() const
{
    qCDebug(MpvMediaBackendLog) << "MpvMediaBackend::muted()";

    return d->m_isMuted;
}

qreal MpvMediaBackend::volume() const
{
    qCDebug(MpvMediaBackendLog) << "MpvMediaBackend::volume()";

    return d->m_volume;
}

QUrl MpvMediaBackend::source() const
{
    qCDebug(MpvMediaBackendLog) << "MpvMediaBackend::source()";

    return d->m_source;
}

KMediaSession::Error MpvMediaBackend::error() const
{
    qCDebug(MpvMediaBackendLog) << "MpvMediaBackend::error()";

    return d->m_error;
}

qint64 MpvMediaBackend::duration() const
{
    qCDebug(MpvMediaBackendLog) << "MpvMediaBackend::duration()";

    return d->m_duration;
}

qint64 MpvMediaBackend::position() const
{
    qCDebug(MpvMediaBackendLog) << "MpvMediaBackend::position()";

    d->m_position = d->getProperty(QStringLiteral("time-pos")).toDouble() * 1000;
    return d->m_position;
}

qreal MpvMediaBackend::playbackRate() const
{
    qCDebug(MpvMediaBackendLog) << "MpvMediaBackend::playbackRate()";

    return d->m_playbackRate;
}

bool MpvMediaBackend::seekable() const
{
    qCDebug(MpvMediaBackendLog) << "MpvMediaBackend::seekable()";

    return d->m_isSeekable;
}

KMediaSession::PlaybackState MpvMediaBackend::playbackState() const
{
    qCDebug(MpvMediaBackendLog) << "MpvMediaBackend::playbackState()";

    bool paused = d->getProperty(QStringLiteral("pause")).toBool();
    if (!d->m_source.isEmpty()) {
        if (paused) {
            if (d->m_playerState != KMediaSession::PlaybackState::StoppedState) {
                d->m_playerState = KMediaSession::PlaybackState::PausedState;
            }
            if (d->m_timer->isActive()) {
                d->m_timer->stop();
            }
        } else {
            d->m_playerState = KMediaSession::PlaybackState::PlayingState;
            if (!d->m_timer->isActive()) {
                d->m_timer->start(d->m_notifyInterval);
            }
        }
    } else {
        d->m_playerState = KMediaSession::PlaybackState::StoppedState;
        if (d->m_timer->isActive()) {
            d->m_timer->stop();
        }
    }
    return d->m_playerState;
}

KMediaSession::MediaStatus MpvMediaBackend::mediaStatus() const
{
    qCDebug(MpvMediaBackendLog) << "MpvMediaBackend::mediaStatus()";

    return d->m_mediaStatus;
}

void MpvMediaBackend::setMuted(bool muted)
{
    qCDebug(MpvMediaBackendLog) << "MpvMediaBackend::setMuted(" << muted << ")";

    Q_EMIT d->setProperty(QStringLiteral("ao-mute"), muted);
}

void MpvMediaBackend::setVolume(qreal volume)
{
    qCDebug(MpvMediaBackendLog) << "MpvMediaBackend::setVolume(" << volume << ")";

    if (volume >= 99.0) {
        Q_EMIT d->setProperty(QStringLiteral("ao-volume"), 100);
    } else {
        Q_EMIT d->setProperty(QStringLiteral("ao-volume"), (-std::log(1 - volume / 100.0) / LOG100) * 100.0);
    }
}

void MpvMediaBackend::setSource(const QUrl &source)
{
    if (source.isEmpty()) {
        return;
    }

    if (playbackState() != KMediaSession::PlaybackState::StoppedState) {
        stop();
    }

    d->m_duration = 0;
    d->m_isSeekable = false;
    d->m_playbackRate = 1.0;
    d->m_position = 0;
    d->m_playerState = KMediaSession::PlaybackState::StoppedState;
    d->m_mediaStatus = KMediaSession::MediaStatus::LoadingMedia;

    d->command(QStringList{QStringLiteral("loadfile"), source.toString()});
    Q_EMIT d->setProperty(QStringLiteral("pause"), true);

    d->m_source = source;

    qCDebug(MpvMediaBackendLog) << "MpvMediaBackend::sourceChanged(" << source << ")";
    QTimer::singleShot(0, this, [this, source]() {
        Q_EMIT sourceChanged(source);
    });
}

void MpvMediaBackend::setPosition(qint64 position)
{
    qCDebug(MpvMediaBackendLog) << "MpvMediaBackend::setPosition(" << position << ")";

    Q_EMIT d->setProperty(QStringLiteral("time-pos"), position / 1000.0);

    d->m_position = position;
    Q_EMIT positionChanged(d->m_position);
}

void MpvMediaBackend::setPlaybackRate(qreal rate)
{
    qCDebug(MpvMediaBackendLog) << "MpvMediaBackend::setPlaybackRate(" << rate << ")";

    Q_EMIT d->setProperty(QStringLiteral("speed"), rate);
}

void MpvMediaBackend::play()
{
    qCDebug(MpvMediaBackendLog) << "MpvMediaBackend::play()";

    if (d->m_playerState == KMediaSession::PlaybackState::PlayingState) {
        return;
    }

    d->m_playerState = KMediaSession::PlaybackState::PlayingState;
    Q_EMIT playbackStateChanged(d->m_playerState);

    Q_EMIT d->setProperty(QStringLiteral("pause"), false);
}

void MpvMediaBackend::pause()
{
    qCDebug(MpvMediaBackendLog) << "MpvMediaBackend::pause()";

    if (d->m_playerState == KMediaSession::PlaybackState::PausedState) {
        return;
    }

    d->m_playerState = KMediaSession::PlaybackState::PausedState;
    Q_EMIT playbackStateChanged(d->m_playerState);

    Q_EMIT d->setProperty(QStringLiteral("pause"), true);
}

void MpvMediaBackend::stop()
{
    qCDebug(MpvMediaBackendLog) << "MpvMediaBackend::stop()";

    if (d->m_playerState == KMediaSession::PlaybackState::StoppedState) {
        return;
    }

    Q_EMIT d->setProperty(QStringLiteral("pause"), true);

    d->m_playerState = KMediaSession::PlaybackState::StoppedState;
    Q_EMIT playbackStateChanged(d->m_playerState);

    setPosition(0);
    Q_EMIT positionChanged(d->m_position);
}

MpvMediaBackendPrivate::MpvMediaBackendPrivate(QQuickItem *parent)
    : MpvAbstractItem(parent)
{
    // enable console output
    Q_EMIT setProperty(QStringLiteral("terminal"), QStringLiteral("no"));

    // don't load user scripts or configs
    Q_EMIT setProperty(QStringLiteral("config"), QStringLiteral("no"));
    Q_EMIT setProperty(QStringLiteral("load-scripts"), QStringLiteral("no"));

    // force vo to libmpv (which it should be set to anyway) --> is this needed?
    // setProperty(QStringLiteral("vo"), QStringLiteral("libmpv"));
    Q_EMIT setProperty(QStringLiteral("video"), QStringLiteral("no"));
    Q_EMIT setProperty(QStringLiteral("audio-display"), QStringLiteral("no"));

    // use safe hardware acceleration
    Q_EMIT setProperty(QStringLiteral("hwdec"), QStringLiteral("auto-safe"));

    // disable OSD and fonts
    Q_EMIT setProperty(QStringLiteral("osd-level"), QStringLiteral("0"));
    Q_EMIT setProperty(QStringLiteral("embeddedfonts"), QStringLiteral("no"));

    // disable input
    Q_EMIT setProperty(QStringLiteral("input-builtin-bindings"), QStringLiteral("no"));
    Q_EMIT setProperty(QStringLiteral("input-default-bindings"), QStringLiteral("no"));
    Q_EMIT setProperty(QStringLiteral("input-vo-keyboard"), QStringLiteral("no"));

    Q_EMIT observeProperty(QStringLiteral("duration"), MPV_FORMAT_DOUBLE);
    // Q_EMIT observeProperty(QStringLiteral("time-pos"), MPV_FORMAT_DOUBLE); // fires too frequently; using dedicated timer
    Q_EMIT observeProperty(QStringLiteral("speed"), MPV_FORMAT_DOUBLE);
    Q_EMIT observeProperty(QStringLiteral("pause"), MPV_FORMAT_FLAG);
    Q_EMIT observeProperty(QStringLiteral("metadata"), MPV_FORMAT_NODE_MAP);
    Q_EMIT observeProperty(QStringLiteral("seekable"), MPV_FORMAT_FLAG);

    connect(mpvController(), &MpvController::propertyChanged, this, &MpvMediaBackendPrivate::onPropertyChanged, Qt::QueuedConnection);
    connect(mpvController(), &MpvController::fileLoaded, this, &MpvMediaBackendPrivate::onFileLoaded, Qt::QueuedConnection);
    connect(mpvController(), &MpvController::endFile, this, &MpvMediaBackendPrivate::onEndFile, Qt::QueuedConnection);

    // create timer to get player state updates
    // this is done because some properties don't send signals (e.g. ao-volume),
    // or fire them to rapidly (e.g. time-pos)
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &MpvMediaBackendPrivate::timerUpdate);
}

void MpvMediaBackendPrivate::onFileLoaded()
{
    qCDebug(MpvSignalsLog) << "MpvMediaBackendPrivate::onFileLoaded";

    m_isSeekable = true;
    m_playerState = KMediaSession::PlaybackState::PausedState;
    m_mediaStatus = KMediaSession::MediaStatus::BufferedMedia;

    Q_EMIT m_parent->mediaStatusChanged(m_mediaStatus);
    Q_EMIT m_parent->playbackStateChanged(m_playerState);
    Q_EMIT m_parent->seekableChanged(m_isSeekable);

    parseMetaData();
}

void MpvMediaBackendPrivate::onEndFile(const QString &reason)
{
    qCDebug(MpvSignalsLog) << "MpvMediaBackendPrivate::onEndFile";

    if (m_timer->isActive()) {
        m_timer->stop();
    }

    m_isSeekable = false;
    m_playerState = KMediaSession::PlaybackState::StoppedState;
    m_mediaStatus = KMediaSession::MediaStatus::EndOfMedia;

    if (reason == QStringLiteral("error")) {
        m_mediaStatus = KMediaSession::MediaStatus::InvalidMedia;
        m_error = KMediaSession::Error::ResourceError;
        Q_EMIT m_parent->errorChanged(m_error);
    }

    Q_EMIT m_parent->mediaStatusChanged(KMediaSession::MediaStatus::NoMedia);
    Q_EMIT m_parent->mediaStatusChanged(m_mediaStatus);
    Q_EMIT m_parent->playbackStateChanged(m_playerState);
    Q_EMIT m_parent->seekableChanged(m_isSeekable);
}

void MpvMediaBackendPrivate::parseMetaData()
{
    qCDebug(MpvSignalsLog) << "MpvMediaBackendPrivate::parseMetaData";
    qCDebug(MpvSignalsLog) << getProperty(QStringLiteral("metadata")).toMap();

    QMap metaData = getProperty(QStringLiteral("metadata")).toMap();
    if (metaData.contains(QStringLiteral("title"))) {
        m_kmediaSession->metaData()->setTitle(metaData[QStringLiteral("title")].toString());
    }
    if (metaData.contains(QStringLiteral("artist"))) {
        m_kmediaSession->metaData()->setArtist(metaData[QStringLiteral("artist")].toString());
    }
    if (metaData.contains(QStringLiteral("album"))) {
        m_kmediaSession->metaData()->setAlbum(metaData[QStringLiteral("album")].toString());
    }

    // TODO: get image; perhaps from track-list?
    // qDebug() << getProperty(QStringLiteral("track-list"));
}

void MpvMediaBackendPrivate::onPropertyChanged(const QString &property, const QVariant &value)
{
    qCDebug(MpvSignalsLog) << "MpvMediaBackendPrivate::onPropertyChanged triggered on property" << property << ", value" << value;

    // abort here if the signalled property value is invalid
    if (!value.isValid()) {
        return;
    }

    if (property == QStringLiteral("time-pos")) {
        qCDebug(MpvSignalsLog) << "MpvMediaBackendPrivate::onPropertyChanged"
                               << "time-pos" << value;
        m_position = value.toDouble() * 1000;
        Q_EMIT m_parent->positionChanged(m_position);
    } else if (property == QStringLiteral("duration")) {
        qCDebug(MpvSignalsLog) << "MpvMediaBackendPrivate::onPropertyChanged"
                               << "duration" << value;
        m_duration = value.toDouble() * 1000;
        Q_EMIT m_parent->durationChanged(m_duration);
    } else if (property == QStringLiteral("speed")) {
        qCDebug(MpvSignalsLog) << "MpvMediaBackendPrivate::onPropertyChanged"
                               << "speed" << value;
        m_playbackRate = value.toDouble();
        Q_EMIT m_parent->playbackRateChanged(m_playbackRate);
    } else if (property == QStringLiteral("seekable")) {
        qCDebug(MpvSignalsLog) << "MpvMediaBackendPrivate::onPropertyChanged"
                               << "seekable" << value;
        m_isSeekable = value.toBool();
        Q_EMIT m_parent->seekableChanged(m_isSeekable);
    } else if (property == QStringLiteral("metadata")) {
        qCDebug(MpvSignalsLog) << "MpvMediaBackendPrivate::onPropertyChanged"
                               << "metadata" << value;
        parseMetaData();
    } else if (property == QStringLiteral("pause")) {
        qCDebug(MpvSignalsLog) << "MpvMediaBackendPrivate::onPropertyChanged"
                               << "pause" << value;
        // pause is expected to be false if there's no currently loaded media, so skip it
        if (!m_source.isEmpty()) {
            bool paused = value.toBool();
            if (paused) {
                if (m_playerState != KMediaSession::PlaybackState::StoppedState) {
                    m_playerState = KMediaSession::PlaybackState::PausedState;
                }
                if (m_timer->isActive()) {
                    m_timer->stop();
                }
            } else {
                m_playerState = KMediaSession::PlaybackState::PlayingState;
                if (!m_timer->isActive()) {
                    m_timer->start(m_notifyInterval);
                }
            }
        } else {
            m_playerState = KMediaSession::PlaybackState::StoppedState;
            if (m_timer->isActive()) {
                m_timer->stop();
            }
        }
        Q_EMIT m_parent->playbackStateChanged(m_playerState);
    }
}

void MpvMediaBackendPrivate::timerUpdate()
{
    qCDebug(MpvSignalsLog) << "MpvMediaBackendPrivate::timerUpdate";
    QTimer::singleShot(0, this, [this]() {
        // Update position
        // Always update position; this timer only runs if we're playing so
        // the position should have changed
        m_position = getProperty(QStringLiteral("time-pos")).toDouble() * 1000;
        Q_EMIT m_parent->positionChanged(m_position);

        // Update volume
        m_newVolume = std::max(0.0, getProperty(QStringLiteral("ao-volume")).toDouble());
        m_newVolume = (1 - std::exp(-m_newVolume / 100.0 * LOG100)) * 100.0; // convert from linear to log scale
        m_newVolume = (m_newVolume) > 99 ? 100 : m_newVolume;
        if (abs(m_newVolume - m_volume) > 0.5) {
            m_volume = m_newVolume;
            Q_EMIT m_parent->volumeChanged(m_volume);
        }

        // Update mute state
        m_newIsMuted = getProperty(QStringLiteral("ao-mute")).toBool();
        if (m_newIsMuted != m_isMuted) {
            m_isMuted = m_newIsMuted;
            Q_EMIT m_parent->mutedChanged(m_isMuted);
        }
    });
}
