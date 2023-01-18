/**
 * SPDX-FileCopyrightText: 2022-2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#include "qtmediabackend.h"
#include "qtmediabackendlogging.h"

#include <memory>

#include <QAudio>
#if QT_VERSION > QT_VERSION_CHECK(6, 0, 0)
#include <QAudioOutput>
#endif
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#if QT_VERSION > QT_VERSION_CHECK(6, 0, 0)
#include <QMediaMetaData>
#endif
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTimer>

class QtMediaBackendPrivate
{
private:
    friend class QtMediaBackend;

    KMediaSession *m_KMediaSession = nullptr;

    QMediaPlayer m_player;
#if QT_VERSION > QT_VERSION_CHECK(6, 0, 0)
    QAudioOutput m_output;
#else
#define m_output m_player
#define QAudioOutput QMediaPlayer
#endif

    std::unique_ptr<QTemporaryDir> imageCacheDir = nullptr;

    KMediaSession::Error translateErrorEnum(QMediaPlayer::Error errorEnum);
    KMediaSession::MediaStatus translateMediaStatusEnum(QMediaPlayer::MediaStatus mediaEnum);
#if QT_VERSION > QT_VERSION_CHECK(6, 0, 0)
    KMediaSession::PlaybackState translatePlaybackStateEnum(QMediaPlayer::PlaybackState playbackStateEnum);
#else
    KMediaSession::PlaybackState translatePlaybackStateEnum(QMediaPlayer::State playbackStateEnum);
#endif
    void parseMetaData();
};

QtMediaBackend::QtMediaBackend(QObject *parent)
    : AbstractMediaBackend(parent)
    , d(std::make_unique<QtMediaBackendPrivate>())
{
    qCDebug(QtMediaBackendLog) << "QtMediaBackend::QtMediaBackend";

    d->m_KMediaSession = static_cast<KMediaSession *>(parent);

#if QT_VERSION > QT_VERSION_CHECK(6, 0, 0)
    d->m_player.setAudioOutput(&d->m_output);
#endif

    // connect to QMediaPlayer signals and dispatch to AbstractMediaBackend
    // signals and add debug output
    connect(&d->m_output, &QAudioOutput::mutedChanged, this, &QtMediaBackend::playerMutedSignalChanges);
    connect(&d->m_output, &QAudioOutput::volumeChanged, this, &QtMediaBackend::playerVolumeSignalChanges);
#if QT_VERSION > QT_VERSION_CHECK(6, 0, 0)
    connect(&d->m_player, &QMediaPlayer::sourceChanged, this, &QtMediaBackend::playerSourceSignalChanges);
    connect(&d->m_player, &QMediaPlayer::playbackStateChanged, this, &QtMediaBackend::playerStateSignalChanges);
    connect(&d->m_player, QOverload<QMediaPlayer::Error, const QString &>::of(&QMediaPlayer::errorOccurred), this, &QtMediaBackend::playerErrorSignalChanges);
    connect(&d->m_player, &QMediaPlayer::metaDataChanged, this, &QtMediaBackend::playerMetaDataSignalChanges);
#else
    connect(&d->m_player, &QMediaPlayer::mediaChanged, this, &QtMediaBackend::playerSourceSignalChanges);
    connect(&d->m_player, &QMediaPlayer::stateChanged, this, &QtMediaBackend::playerStateSignalChanges);
    connect(&d->m_player, QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error), this, &QtMediaBackend::playerErrorSignalChanges);
    connect(&d->m_player, QOverload<>::of(&QMediaObject::metaDataChanged), this, &QtMediaBackend::playerMetaDataSignalChanges);
#endif
    connect(&d->m_player, &QMediaPlayer::mediaStatusChanged, this, &QtMediaBackend::mediaStatusSignalChanges);
    connect(&d->m_player, &QMediaPlayer::playbackRateChanged, this, &QtMediaBackend::playerPlaybackRateSignalChanges);
    connect(&d->m_player, &QMediaPlayer::durationChanged, this, &QtMediaBackend::playerDurationSignalChanges);
    connect(&d->m_player, &QMediaPlayer::positionChanged, this, &QtMediaBackend::playerPositionSignalChanges);
    connect(&d->m_player, &QMediaPlayer::seekableChanged, this, &QtMediaBackend::playerSeekableSignalChanges);
}

QtMediaBackend::~QtMediaBackend()
{
    qCDebug(QtMediaBackendLog) << "QtMediaBackend::~QtMediaBackend";
    d->m_player.stop();
}

KMediaSession::MediaBackends QtMediaBackend::backend() const
{
    qCDebug(QtMediaBackendLog) << "QtMediaBackend::backend()";
    return KMediaSession::MediaBackends::Qt;
}

bool QtMediaBackend::muted() const
{
    qCDebug(QtMediaBackendLog) << "QtMediaBackend::muted()";
    return d->m_output.isMuted();
}

qreal QtMediaBackend::volume() const
{
    qCDebug(QtMediaBackendLog) << "QtMediaBackend::volume()";
#if QT_VERSION > QT_VERSION_CHECK(6, 0, 0)
    qreal realVolume = static_cast<qreal>(d->m_output.volume());
#else
    qreal realVolume = static_cast<qreal>(d->m_output.volume() / 100.0);
#endif
    qreal userVolume = static_cast<qreal>(QAudio::convertVolume(realVolume, QAudio::LinearVolumeScale, QAudio::LogarithmicVolumeScale));

    return userVolume * 100.0;
}

QUrl QtMediaBackend::source() const
{
    qCDebug(QtMediaBackendLog) << "QtMediaBackend::source()";
#if QT_VERSION > QT_VERSION_CHECK(6, 0, 0)
    return d->m_player.source();
#else
    return d->m_player.media().request().url();
#endif
}

KMediaSession::MediaStatus QtMediaBackend::mediaStatus() const
{
    qCDebug(QtMediaBackendLog) << "QtMediaBackend::mediaStatus()";
    return d->translateMediaStatusEnum(d->m_player.mediaStatus());
}

KMediaSession::PlaybackState QtMediaBackend::playbackState() const
{
    qCDebug(QtMediaBackendLog) << "QtMediaBackend::playbackState()";
#if QT_VERSION > QT_VERSION_CHECK(6, 0, 0)
    return d->translatePlaybackStateEnum(d->m_player.playbackState());
#else
    return d->translatePlaybackStateEnum(d->m_player.state());
#endif
}

qreal QtMediaBackend::playbackRate() const
{
    qCDebug(QtMediaBackendLog) << "QtMediaBackend::playbackRate()";
    return d->m_player.playbackRate();
}

KMediaSession::Error QtMediaBackend::error() const
{
    qCDebug(QtMediaBackendLog) << "QtMediaBackend::error()";
    return d->translateErrorEnum(d->m_player.error());
}

qint64 QtMediaBackend::duration() const
{
    qCDebug(QtMediaBackendLog) << "QtMediaBackend::duration()";
    return d->m_player.duration();
}

qint64 QtMediaBackend::position() const
{
    qCDebug(QtMediaBackendLog) << "QtMediaBackend::position()";
    return d->m_player.position();
}

bool QtMediaBackend::seekable() const
{
    qCDebug(QtMediaBackendLog) << "QtMediaBackend::seekable()";
    return d->m_player.isSeekable();
}

void QtMediaBackend::setMuted(bool muted)
{
    qCDebug(QtMediaBackendLog) << "QtMediaBackend::setMuted(" << muted << ")";
    d->m_output.setMuted(muted);
}

void QtMediaBackend::setVolume(qreal volume)
{
    qCDebug(QtMediaBackendLog) << "QtMediaBackend::setVolume(" << volume << ")";

    qreal realVolume = static_cast<qreal>(QAudio::convertVolume(volume / 100.0, QAudio::LogarithmicVolumeScale, QAudio::LinearVolumeScale));
#if QT_VERSION > QT_VERSION_CHECK(6, 0, 0)
    d->m_output.setVolume(realVolume);
#else
    d->m_output.setVolume(qRound(realVolume * 100.0));
#endif
}

void QtMediaBackend::setSource(const QUrl &source)
{
    qCDebug(QtMediaBackendLog) << "QtMediaBackend::setSource(" << source << ")";
#if QT_VERSION > QT_VERSION_CHECK(6, 0, 0)
    d->m_player.setSource(source);
#else
    d->m_player.setMedia(source);
#endif
}

void QtMediaBackend::setPosition(qint64 position)
{
    qCDebug(QtMediaBackendLog) << "QtMediaBackend::setPosition(" << position << ")";
    d->m_player.setPosition(position);
}

void QtMediaBackend::setPlaybackRate(qreal rate)
{
    qCDebug(QtMediaBackendLog) << "QtMediaBackend::setPlaybackRate(" << rate << ")";
    d->m_player.setPlaybackRate(rate);
}

void QtMediaBackend::pause()
{
    qCDebug(QtMediaBackendLog) << "QtMediaBackend::pause()";
    d->m_player.pause();
}

void QtMediaBackend::play()
{
    qCDebug(QtMediaBackendLog) << "QtMediaBackend::play()";
    d->m_player.play();
}

void QtMediaBackend::stop()
{
    qCDebug(QtMediaBackendLog) << "QtMediaBackend::stop()";
    d->m_player.stop();
}

void QtMediaBackend::playerMutedSignalChanges(bool muted)
{
    QTimer::singleShot(0, this, [this, muted]() {
        qCDebug(QtMediaBackendLog) << "QtMediaBackend::mutedChanged(" << muted << ")";
        Q_EMIT mutedChanged(muted);
    });
}

#if QT_VERSION > QT_VERSION_CHECK(6, 0, 0)
void QtMediaBackend::playerVolumeSignalChanges(float volume)
{
    qreal realVolume = static_cast<qreal>(volume);
#else
void QtMediaBackend::playerVolumeSignalChanges(qint64 volume)
{
    qreal realVolume = static_cast<qreal>(volume) / 100.0;
#endif
    qreal userVolume = static_cast<qreal>(QAudio::convertVolume(realVolume, QAudio::LinearVolumeScale, QAudio::LogarithmicVolumeScale)) * 100.0;
    QTimer::singleShot(0, this, [this, userVolume]() {
        qCDebug(QtMediaBackendLog) << "QtMediaBackend::volumeChanged(" << userVolume << ")";
        Q_EMIT volumeChanged(userVolume);
    });
}

#if QT_VERSION > QT_VERSION_CHECK(6, 0, 0)
void QtMediaBackend::playerSourceSignalChanges(const QUrl &media)
{
    QUrl source = media;

#else
void QtMediaBackend::playerSourceSignalChanges(const QMediaContent &media)
{
    QUrl source = media.request().url();
#endif
    QTimer::singleShot(0, this, [this, source]() {
        qCDebug(QtMediaBackendLog) << "QtMediaBackend::sourceChanged(" << source << ")";
        Q_EMIT sourceChanged(source);
    });
}

void QtMediaBackend::mediaStatusSignalChanges(const QMediaPlayer::MediaStatus &qtMediaStatus)
{
    const KMediaSession::MediaStatus mediaStatus = d->translateMediaStatusEnum(qtMediaStatus);
    QTimer::singleShot(0, this, [this, mediaStatus]() {
        Q_EMIT mediaStatusChanged(mediaStatus);
    });
}

#if QT_VERSION > QT_VERSION_CHECK(6, 0, 0)
void QtMediaBackend::playerStateSignalChanges(const QMediaPlayer::PlaybackState &qtPlaybackState)
#else
void QtMediaBackend::playerStateSignalChanges(const QMediaPlayer::State &qtPlaybackState)
#endif
{
    const KMediaSession::PlaybackState playbackState = d->translatePlaybackStateEnum(qtPlaybackState);
    QTimer::singleShot(0, this, [this, playbackState]() {
        Q_EMIT playbackStateChanged(playbackState);
    });
}

void QtMediaBackend::playerPlaybackRateSignalChanges(const qreal &playbackRate)
{
    QTimer::singleShot(0, this, [this, playbackRate]() {
        Q_EMIT playbackRateChanged(playbackRate);
    });
}

void QtMediaBackend::playerErrorSignalChanges(const QMediaPlayer::Error &error)
{
    QTimer::singleShot(0, this, [this, error]() {
        Q_EMIT errorChanged(d->translateErrorEnum(error));
    });
}

void QtMediaBackend::playerDurationSignalChanges(qint64 newDuration)
{
    QTimer::singleShot(0, this, [this, newDuration]() {
        qCDebug(QtMediaBackendLog) << "QtMediaBackend::durationChanged(" << newDuration << ")";
        Q_EMIT durationChanged(newDuration);
    });
}

void QtMediaBackend::playerPositionSignalChanges(qint64 newPosition)
{
    QTimer::singleShot(0, this, [this, newPosition]() {
        qCDebug(QtMediaBackendLog) << "QtMediaBackend::positionChanged(" << newPosition << ")";
        Q_EMIT positionChanged(newPosition);
    });
}

void QtMediaBackend::playerSeekableSignalChanges(bool seekable)
{
    QTimer::singleShot(0, this, [this, seekable]() {
        Q_EMIT seekableChanged(seekable);
    });
}

void QtMediaBackend::playerMetaDataSignalChanges()
{
    d->parseMetaData();
}

KMediaSession::Error QtMediaBackendPrivate::translateErrorEnum(QMediaPlayer::Error errorEnum)
{
    qCDebug(QtMediaBackendLog) << "QtMediaBackendPrivate::translateErrorEnum(" << errorEnum << ")";
    switch (errorEnum) {
    case QMediaPlayer::Error::NoError:
        return KMediaSession::Error::NoError;
    case QMediaPlayer::Error::ResourceError:
        return KMediaSession::Error::ResourceError;
    case QMediaPlayer::Error::FormatError:
        return KMediaSession::Error::FormatError;
    case QMediaPlayer::Error::NetworkError:
        return KMediaSession::Error::NetworkError;
    case QMediaPlayer::Error::AccessDeniedError:
        return KMediaSession::Error::AccessDeniedError;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    case QMediaPlayer::Error::ServiceMissingError:
        return KMediaSession::Error::ServiceMissingError;
#endif
    default:
        return KMediaSession::Error::NoError;
    }
}

KMediaSession::MediaStatus QtMediaBackendPrivate::translateMediaStatusEnum(QMediaPlayer::MediaStatus mediaEnum)
{
    qCDebug(QtMediaBackendLog) << "QtMediaBackendPrivate::translateMediaStatusEnum(" << mediaEnum << ")";
    switch (mediaEnum) {
    case QMediaPlayer::MediaStatus::NoMedia:
        return KMediaSession::MediaStatus::NoMedia;
    case QMediaPlayer::MediaStatus::LoadingMedia:
        return KMediaSession::MediaStatus::LoadingMedia;
    case QMediaPlayer::MediaStatus::LoadedMedia:
        return KMediaSession::MediaStatus::LoadedMedia;
    case QMediaPlayer::MediaStatus::StalledMedia:
        return KMediaSession::MediaStatus::StalledMedia;
    case QMediaPlayer::MediaStatus::BufferingMedia:
        return KMediaSession::MediaStatus::BufferingMedia;
    case QMediaPlayer::MediaStatus::BufferedMedia:
        return KMediaSession::MediaStatus::BufferedMedia;
    case QMediaPlayer::MediaStatus::EndOfMedia:
        return KMediaSession::MediaStatus::EndOfMedia;
    case QMediaPlayer::MediaStatus::InvalidMedia:
        return KMediaSession::MediaStatus::InvalidMedia;
#if QT_VERSION > QT_VERSION_CHECK(6, 0, 0)
    default:
        return KMediaSession::MediaStatus::NoMedia;
#else
    case QMediaPlayer::MediaStatus::UnknownMediaStatus:
    default:
        return KMediaSession::MediaStatus::UnknownMediaStatus;
#endif
    }
}

#if QT_VERSION > QT_VERSION_CHECK(6, 0, 0)
KMediaSession::PlaybackState QtMediaBackendPrivate::translatePlaybackStateEnum(QMediaPlayer::PlaybackState playbackStateEnum)
{
    qCDebug(QtMediaBackendLog) << "QtMediaBackendPrivate::translateMediaStatusEnum(" << playbackStateEnum << ")";

    switch (playbackStateEnum) {
    case QMediaPlayer::PlaybackState::StoppedState:
        return KMediaSession::PlaybackState::StoppedState;
    case QMediaPlayer::PlaybackState::PlayingState:
        return KMediaSession::PlaybackState::PlayingState;
    case QMediaPlayer::PlaybackState::PausedState:
        return KMediaSession::PlaybackState::PausedState;
    default:
        return KMediaSession::PlaybackState::StoppedState;
    }
}
#else
KMediaSession::PlaybackState QtMediaBackendPrivate::translatePlaybackStateEnum(QMediaPlayer::State playbackStateEnum)
{
    qCDebug(QtMediaBackendLog) << "QtMediaBackendPrivate::translateMediaStatusEnum(" << playbackStateEnum << ")";

    switch (playbackStateEnum) {
    case QMediaPlayer::State::StoppedState:
        return KMediaSession::PlaybackState::StoppedState;
    case QMediaPlayer::State::PlayingState:
        return KMediaSession::PlaybackState::PlayingState;
    case QMediaPlayer::State::PausedState:
        return KMediaSession::PlaybackState::PausedState;
    default:
        return KMediaSession::PlaybackState::StoppedState;
    }
}
#endif

void QtMediaBackendPrivate::parseMetaData()
{
    qCDebug(QtMediaBackendLog) << "QtMediaBackendPrivate::parseMetaData()";

    if (m_KMediaSession->metaData()->title().isEmpty()) {
#if QT_VERSION > QT_VERSION_CHECK(6, 0, 0)
        m_KMediaSession->metaData()->setTitle(m_player.metaData().stringValue(QMediaMetaData::Title));
#else
        m_KMediaSession->metaData()->setTitle(m_player.metaData(QStringLiteral("Title")).toString());
#endif
    }

    if (m_KMediaSession->metaData()->artist().isEmpty()) {
#if QT_VERSION > QT_VERSION_CHECK(6, 0, 0)
        m_KMediaSession->metaData()->setArtist(m_player.metaData().stringValue(QMediaMetaData::ContributingArtist));
#else
        m_KMediaSession->metaData()->setArtist(m_player.metaData(QStringLiteral("ContributingArtist")).toString());
#endif
    }

    if (m_KMediaSession->metaData()->album().isEmpty()) {
#if QT_VERSION > QT_VERSION_CHECK(6, 0, 0)
        m_KMediaSession->metaData()->setAlbum(m_player.metaData().stringValue(QMediaMetaData::AlbumTitle));
#else
        m_KMediaSession->metaData()->setAlbum(m_player.metaData(QStringLiteral("AlbumTitle")).toString());
#endif
    }
    if (m_KMediaSession->metaData()->artworkUrl().isEmpty()) {
#if QT_VERSION > QT_VERSION_CHECK(6, 0, 0)
        if (m_player.metaData().value(QMediaMetaData::CoverArtImage).isValid()) {
#else
        if (m_player.metaData(QStringLiteral("CoverArtImage")).isValid()) {
#endif
            imageCacheDir = std::make_unique<QTemporaryDir>();
            if (imageCacheDir->isValid()) {
                QString filePath = imageCacheDir->path() + QStringLiteral("/coverimage");

#if QT_VERSION > QT_VERSION_CHECK(6, 0, 0)
                bool success = m_player.metaData().value(QMediaMetaData::CoverArtImage).value<QImage>().save(filePath, "PNG");
#else
                bool success = m_player.metaData(QStringLiteral("CoverArtImage")).value<QImage>().save(filePath, "PNG");
#endif

                if (success) {
                    QString localFilePath = QStringLiteral("file://") + filePath;
                    m_KMediaSession->metaData()->setArtworkUrl(QUrl(localFilePath));
                }
            }
        }
    }
}
