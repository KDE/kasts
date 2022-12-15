/**
 * SPDX-FileCopyrightText: 2022-2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#include "qtmediabackend.h"
#include "qtmediabackendlogging.h"

#include <memory>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QMediaPlayer>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTimer>

class QtMediaBackendPrivate
{
private:
    friend class QtMediaBackend;

    KMediaSession *m_KMediaSession = nullptr;
    QMediaPlayer m_player;

    std::unique_ptr<QTemporaryDir> imageCacheDir = nullptr;

    KMediaSession::Error translateErrorEnum(QMediaPlayer::Error errorEnum);
    KMediaSession::MediaStatus translateMediaStatusEnum(QMediaPlayer::MediaStatus mediaEnum);
    KMediaSession::PlaybackState translatePlaybackStateEnum(QMediaPlayer::State playbackStateEnum);
    void parseMetaData();
};

QtMediaBackend::QtMediaBackend(QObject *parent)
    : AbstractMediaBackend(parent)
    , d(std::make_unique<QtMediaBackendPrivate>())
{
    qCDebug(QtMediaBackendLog) << "QtMediaBackend::QtMediaBackend";

    d->m_KMediaSession = static_cast<KMediaSession *>(parent);

    // Parsing of metadata
    connect(&d->m_player, QOverload<>::of(&QMediaObject::metaDataChanged), this, [this]() {
        d->parseMetaData();
    });

    // connect to QMediaPlayer signals and dispatch to AbstractMediaBackend
    // signals and add debug output
    connect(&d->m_player, &QMediaPlayer::mutedChanged, this, [this](bool muted) {
        QTimer::singleShot(0, this, [this, muted]() {
            qCDebug(QtMediaBackendLog) << "QtMediaBackend::mutedChanged(" << muted << ")";
            Q_EMIT mutedChanged(muted);
        });
    });

    connect(&d->m_player, &QMediaPlayer::volumeChanged, this, [this](int volume) {
        qreal realVolume = static_cast<qreal>(volume) / 100.0;
        qreal userVolume = static_cast<qreal>(QAudio::convertVolume(realVolume, QAudio::LinearVolumeScale, QAudio::LogarithmicVolumeScale)) * 100.0;
        QTimer::singleShot(0, this, [this, userVolume]() {
            qCDebug(QtMediaBackendLog) << "QtMediaBackend::volumeChanged(" << userVolume << ")";
            Q_EMIT volumeChanged(userVolume);
        });
    });

    connect(&d->m_player, &QMediaPlayer::mediaChanged, this, [this](const QMediaContent &media) {
        QUrl source = media.request().url();
        QTimer::singleShot(0, this, [this, source]() {
            qCDebug(QtMediaBackendLog) << "QtMediaBackend::sourceChanged(" << source << ")";
            Q_EMIT sourceChanged(source);
        });
    });

    connect(&d->m_player, &QMediaPlayer::mediaStatusChanged, this, [this](const QMediaPlayer::MediaStatus &qtMediaStatus) {
        const KMediaSession::MediaStatus mediaStatus = d->translateMediaStatusEnum(qtMediaStatus);
        QTimer::singleShot(0, this, [this, mediaStatus]() {
            Q_EMIT mediaStatusChanged(mediaStatus);
        });
    });

    connect(&d->m_player, &QMediaPlayer::stateChanged, this, [this](const QMediaPlayer::State &qtPlaybackState) {
        const KMediaSession::PlaybackState playbackState = d->translatePlaybackStateEnum(qtPlaybackState);
        QTimer::singleShot(0, this, [this, playbackState]() {
            Q_EMIT playbackStateChanged(playbackState);
        });
    });

    connect(&d->m_player, &QMediaPlayer::playbackRateChanged, this, [this](const qreal &playbackRate) {
        QTimer::singleShot(0, this, [this, playbackRate]() {
            Q_EMIT playbackRateChanged(playbackRate);
        });
    });

    connect(&d->m_player, QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error), this, [this](const QMediaPlayer::Error &error) {
        QTimer::singleShot(0, this, [this, error]() {
            Q_EMIT errorChanged(d->translateErrorEnum(error));
        });
    });

    connect(&d->m_player, &QMediaPlayer::durationChanged, this, [this](qint64 duration) {
        QTimer::singleShot(0, this, [this, duration]() {
            qCDebug(QtMediaBackendLog) << "QtMediaBackend::durationChanged(" << duration << ")";
            Q_EMIT durationChanged(duration);
        });
    });

    connect(&d->m_player, &QMediaPlayer::positionChanged, this, [this](qint64 position) {
        QTimer::singleShot(0, this, [this, position]() {
            qCDebug(QtMediaBackendLog) << "QtMediaBackend::positionChanged(" << position << ")";
            Q_EMIT positionChanged(position);
        });
    });
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
    return d->m_player.isMuted();
}

qreal QtMediaBackend::volume() const
{
    qCDebug(QtMediaBackendLog) << "QtMediaBackend::volume()";
    qreal realVolume = static_cast<qreal>(d->m_player.volume() / 100.0);
    qreal userVolume = static_cast<qreal>(QAudio::convertVolume(realVolume, QAudio::LinearVolumeScale, QAudio::LogarithmicVolumeScale));

    return userVolume * 100.0;
}

QUrl QtMediaBackend::source() const
{
    qCDebug(QtMediaBackendLog) << "QtMediaBackend::source()";
    return d->m_player.media().request().url();
}

KMediaSession::MediaStatus QtMediaBackend::mediaStatus() const
{
    qCDebug(QtMediaBackendLog) << "QtMediaBackend::mediaStatus()";
    return d->translateMediaStatusEnum(d->m_player.mediaStatus());
}

KMediaSession::PlaybackState QtMediaBackend::playbackState() const
{
    qCDebug(QtMediaBackendLog) << "QtMediaBackend::playbackState()";
    return d->translatePlaybackStateEnum(d->m_player.state());
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
    d->m_player.setMuted(muted);
}

void QtMediaBackend::setVolume(qreal volume)
{
    qCDebug(QtMediaBackendLog) << "QtMediaBackend::setVolume(" << volume << ")";

    qreal realVolume = static_cast<qreal>(QAudio::convertVolume(volume / 100.0, QAudio::LogarithmicVolumeScale, QAudio::LinearVolumeScale));
    d->m_player.setVolume(qRound(realVolume * 100.0));
}

void QtMediaBackend::setSource(const QUrl &source)
{
    qCDebug(QtMediaBackendLog) << "QtMediaBackend::setSource(" << source << ")";
    d->m_player.setMedia(source);
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
    case QMediaPlayer::Error::ServiceMissingError:
        return KMediaSession::Error::ServiceMissingError;
    default:
        return KMediaSession::Error::NoError;
    }
}

KMediaSession::MediaStatus QtMediaBackendPrivate::translateMediaStatusEnum(QMediaPlayer::MediaStatus mediaEnum)
{
    qCDebug(QtMediaBackendLog) << "QtMediaBackendPrivate::translateMediaStatusEnum(" << mediaEnum << ")";
    switch (mediaEnum) {
    case QMediaPlayer::MediaStatus::UnknownMediaStatus:
        return KMediaSession::MediaStatus::UnknownMediaStatus;
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
    default:
        return KMediaSession::MediaStatus::UnknownMediaStatus;
    }
}

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

void QtMediaBackendPrivate::parseMetaData()
{
    qCDebug(QtMediaBackendLog) << "QtMediaBackendPrivate::parseMetaData()";
    if (m_KMediaSession->metaData()->title().isEmpty()) {
        m_KMediaSession->metaData()->setTitle(m_player.metaData(QStringLiteral("Title")).toString());
    }
    if (m_KMediaSession->metaData()->artist().isEmpty()) {
        m_KMediaSession->metaData()->setArtist(m_player.metaData(QStringLiteral("ContributingArtist")).toString());
    }
    if (m_KMediaSession->metaData()->album().isEmpty()) {
        m_KMediaSession->metaData()->setAlbum(m_player.metaData(QStringLiteral("AlbumTitle")).toString());
    }
    if (m_KMediaSession->metaData()->artworkUrl().isEmpty()) {
        if (m_player.metaData(QStringLiteral("CoverArtImage")).isValid()) {
            imageCacheDir = std::make_unique<QTemporaryDir>();
            if (imageCacheDir->isValid()) {
                QString filePath = imageCacheDir->path() + QStringLiteral("/coverimage");

                bool success = m_player.metaData(QStringLiteral("CoverArtImage")).value<QImage>().save(filePath, "PNG");

                if (success) {
                    QString localFilePath = QStringLiteral("file://") + filePath;
                    m_KMediaSession->metaData()->setArtworkUrl(QUrl(localFilePath));
                }
            }
        }
    }
}
