/**
 * SPDX-FileCopyrightText: 2014 Sujith Haridasan <sujith.haridasan@kdemail.net>
 * SPDX-FileCopyrightText: 2014 Ashish Madeti <ashishmadeti@gmail.com>
 * SPDX-FileCopyrightText: 2016 Matthieu Gallien <matthieu_gallien@yahoo.fr>
 * SPDX-FileCopyrightText: 2022-2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "mediaplayer2player.h"
#include "mpris2.h"
#include "mpris2logging.h"

#include "kmediasession.h"
#include "metadata.h"

#include <QCryptographicHash>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDebug>
#include <QMetaClassInfo>
#include <QStringList>
#include <QTimer>

MediaPlayer2Player::MediaPlayer2Player(KMediaSession *audioPlayer, bool showProgressOnTaskBar, QObject *parent)
    : QDBusAbstractAdaptor(parent)
    , m_audioPlayer(audioPlayer)
    , mProgressIndicatorSignal(
          QDBusMessage::createSignal(QStringLiteral("/org/kde/kmediasession"), QStringLiteral("com.canonical.Unity.LauncherEntry"), QStringLiteral("Update")))
    , mShowProgressOnTaskBar(showProgressOnTaskBar)
{
    qCDebug(Mpris2Log) << "MediaPlayer2Player::MediaPlayer2Player()";
    // This will signal when the track is changed
    connect(m_audioPlayer, &KMediaSession::sourceChanged, this, &MediaPlayer2Player::setSource);

    // Signals from KMediaSession which are directly forwarded
    // TODO: implement this in KMediaSession, such that it can be forwarded
    // connect(m_audioPlayer, &KMediaSession::minimumRateChanged,
    //        this, &MediaPlayer2Player::mimimumRateChanged);
    // connect(m_audioPlayer, &KMediaSession::maximumRateChanged,
    //        this, &MediaPlayer2Player::maximumRateChanged);
    connect(m_audioPlayer, &KMediaSession::seekableChanged, this, &MediaPlayer2Player::canSeekChanged);

    // Signals which are semi-wrapped signals from KMediaSession
    connect(m_audioPlayer, &KMediaSession::playbackStateChanged, this, &MediaPlayer2Player::playerPlaybackStateChanged);
    connect(m_audioPlayer, &KMediaSession::playbackRateChanged, this, &MediaPlayer2Player::playerPlaybackRateChanged);
    connect(m_audioPlayer, &KMediaSession::volumeChanged, this, &MediaPlayer2Player::playerVolumeChanged);
    connect(m_audioPlayer, &KMediaSession::positionJumped, this, &MediaPlayer2Player::playerSeeked); // Implement Seeked signal

    connect(m_audioPlayer, &KMediaSession::canPlayChanged, this, &MediaPlayer2Player::playerCanPlayChanged);
    connect(m_audioPlayer, &KMediaSession::canPauseChanged, this, &MediaPlayer2Player::playerCanPauseChanged);
    connect(m_audioPlayer, &KMediaSession::canGoNextChanged, this, &MediaPlayer2Player::playerCanGoNextChanged);
    connect(m_audioPlayer, &KMediaSession::canGoPreviousChanged, this, &MediaPlayer2Player::playerCanGoPreviousChanged);

    connect(m_audioPlayer, &KMediaSession::seekableChanged, this, &MediaPlayer2Player::playerCanSeekChanged);
    connect(m_audioPlayer, &KMediaSession::metaDataChanged, this, &MediaPlayer2Player::playerMetaDataChanged);

    // signals needed for progress indicator on taskbar
    connect(m_audioPlayer, &KMediaSession::durationChanged, this, &MediaPlayer2Player::audioDurationChanged);
    connect(m_audioPlayer, &KMediaSession::positionChanged, this, &MediaPlayer2Player::audioPositionChanged);

    // Update DBUS object if desktopEntryName changes
    connect(m_audioPlayer, &KMediaSession::desktopEntryNameChanged, this, [this](const QString &desktopName) {
        QString newDesktopName = QStringLiteral("/") + desktopName;
        newDesktopName.replace(QStringLiteral("."), QStringLiteral("/"));
        mProgressIndicatorSignal = QDBusMessage::createSignal(newDesktopName, QStringLiteral("com.canonical.Unity.LauncherEntry"), QStringLiteral("Update"));
    });

    // singleShot used here to execute this code after constructor has finished
    QTimer::singleShot(0, this, [this]() {
        if (m_audioPlayer) {
            m_volume = m_audioPlayer->volume() / 100;
            signalPropertiesChange(QStringLiteral("Volume"), Volume());

            if (!m_audioPlayer->source().isEmpty()) {
                setSource(m_audioPlayer->source());
            }
        }
    });
}

QString MediaPlayer2Player::PlaybackStatus() const
{
    qCDebug(Mpris2Log) << "MediaPlayer2Player::PlaybackStatus()";
    QString result;

    if (m_audioPlayer->playbackState() == KMediaSession::StoppedState) {
        result = QStringLiteral("Stopped");
    } else if (m_audioPlayer->playbackState() == KMediaSession::PlayingState) {
        result = QStringLiteral("Playing");
    } else {
        result = QStringLiteral("Paused");
    }

    if (mShowProgressOnTaskBar) {
        QVariantMap parameters;

        if (m_audioPlayer->playbackState() == KMediaSession::StoppedState || m_audioPlayer->duration() == 0) {
            parameters.insert(QStringLiteral("progress-visible"), false);
            parameters.insert(QStringLiteral("progress"), 0);
        } else {
            parameters.insert(QStringLiteral("progress-visible"), true);
            parameters.insert(QStringLiteral("progress"),
                              qRound(static_cast<double>((m_audioPlayer->duration() > 0) ? m_position / m_audioPlayer->duration() : 0)) / 1000.0);
        }

        const QString fullDesktopPath = QStringLiteral("application://") + m_audioPlayer->desktopEntryName() + QStringLiteral(".desktop");
        mProgressIndicatorSignal.setArguments({fullDesktopPath, parameters});

        QDBusConnection::sessionBus().send(mProgressIndicatorSignal);
    }

    return result;
}

bool MediaPlayer2Player::CanGoNext() const
{
    qCDebug(Mpris2Log) << "MediaPlayer2Player::CanGoNext()";
    if (m_audioPlayer) {
        return m_audioPlayer->canGoNext();
    } else {
        return false;
    }
}

void MediaPlayer2Player::Next()
{
    qCDebug(Mpris2Log) << "MediaPlayer2Player::Next()";
    if (m_audioPlayer) {
        QTimer::singleShot(0, this, [this]() {
            Q_EMIT m_audioPlayer->nextRequested();
        });
    }
}

bool MediaPlayer2Player::CanGoPrevious() const
{
    qCDebug(Mpris2Log) << "MediaPlayer2Player::CanGoPrevious()";
    if (m_audioPlayer) {
        return m_audioPlayer->canGoPrevious();
    } else {
        return false;
    }
}

void MediaPlayer2Player::Previous()
{
    qCDebug(Mpris2Log) << "MediaPlayer2Player::Previous()";
    if (m_audioPlayer) {
        QTimer::singleShot(0, this, [this]() {
            Q_EMIT m_audioPlayer->previousRequested();
        });
    }
}

bool MediaPlayer2Player::CanPause() const
{
    qCDebug(Mpris2Log) << "MediaPlayer2Player::CanPause()";
    if (m_audioPlayer) {
        return m_audioPlayer->canPause();
    } else {
        return false;
    }
}

void MediaPlayer2Player::Pause()
{
    qCDebug(Mpris2Log) << "MediaPlayer2Player::Pause()";
    if (m_audioPlayer)
        m_audioPlayer->pause();
}

void MediaPlayer2Player::PlayPause()
{
    qCDebug(Mpris2Log) << "MediaPlayer2Player::PlayPause()";
    if (m_audioPlayer) {
        if (m_audioPlayer->playbackState() != KMediaSession::PlaybackState::PlayingState) {
            m_audioPlayer->play();
        } else if (m_audioPlayer->playbackState() == KMediaSession::PlaybackState::PlayingState) {
            m_audioPlayer->pause();
        }
    }
}

void MediaPlayer2Player::Stop()
{
    qCDebug(Mpris2Log) << "MediaPlayer2Player::Stop()";
    if (m_audioPlayer) {
        if (!m_audioPlayer->mpris2PauseInsteadOfStop()) {
            m_audioPlayer->stop();
        } else if (m_audioPlayer->playbackState() == KMediaSession::PlaybackState::PlayingState) {
            m_audioPlayer->pause();
        }
    }
}

bool MediaPlayer2Player::CanPlay() const
{
    qCDebug(Mpris2Log) << "MediaPlayer2Player::CanPlay()";
    if (m_audioPlayer) {
        return m_audioPlayer->canPlay();
    } else {
        return false;
    }
}

void MediaPlayer2Player::Play()
{
    qCDebug(Mpris2Log) << "MediaPlayer2Player::Play()";
    if (m_audioPlayer)
        m_audioPlayer->play();
}

double MediaPlayer2Player::Volume() const
{
    qCDebug(Mpris2Log) << "MediaPlayer2Player::Volume()";
    return m_volume;
}

void MediaPlayer2Player::setVolume(double volume)
{
    qCDebug(Mpris2Log) << "MediaPlayer2Player::setVolume(" << volume << ")";
    if (m_audioPlayer) {
        m_volume = qBound(0.0, volume, 1.0);
        Q_EMIT volumeChanged(m_volume);

        m_audioPlayer->setVolume(100 * m_volume);

        signalPropertiesChange(QStringLiteral("Volume"), Volume());
    }
}

QVariantMap MediaPlayer2Player::Metadata() const
{
    qCDebug(Mpris2Log) << "MediaPlayer2Player::Metadata()";
    return m_metadata;
}

qlonglong MediaPlayer2Player::Position() const
{
    qCDebug(Mpris2Log) << "MediaPlayer2Player::Position()";
    if (m_audioPlayer)
        return qlonglong(m_audioPlayer->position()) * 1000;
    else
        return 0;
}

double MediaPlayer2Player::Rate() const
{
    qCDebug(Mpris2Log) << "MediaPlayer2Player::Rate()";
    if (m_audioPlayer)
        return m_audioPlayer->playbackRate();
    else
        return 1.0;
}

double MediaPlayer2Player::MinimumRate() const
{
    qCDebug(Mpris2Log) << "MediaPlayer2Player::MinimumRate()";
    if (m_audioPlayer)
        return m_audioPlayer->minimumPlaybackRate();
    else
        return 1.0;
}

double MediaPlayer2Player::MaximumRate() const
{
    qCDebug(Mpris2Log) << "MediaPlayer2Player::MaximumRate()";
    if (m_audioPlayer)
        return m_audioPlayer->maximumPlaybackRate();
    else
        return 1.0;
}

void MediaPlayer2Player::setRate(double newRate)
{
    qCDebug(Mpris2Log) << "MediaPlayer2Player::setRate(" << newRate << ")";
    if (newRate <= 0.0001 && newRate >= -0.0001) {
        Pause();
    } else {
        m_audioPlayer->setPlaybackRate(newRate);
    }
}

bool MediaPlayer2Player::CanSeek() const
{
    qCDebug(Mpris2Log) << "MediaPlayer2Player::CanSeek()";
    if (m_audioPlayer)
        return m_audioPlayer->seekable();
    else
        return false;
}

bool MediaPlayer2Player::CanControl() const
{
    qCDebug(Mpris2Log) << "MediaPlayer2Player::CanControl()";
    return true;
}

void MediaPlayer2Player::Seek(qlonglong Offset)
{
    qCDebug(Mpris2Log) << "MediaPlayer2Player::Seek(" << Offset << ")";
    if (m_audioPlayer) {
        auto offset = (m_position + Offset) / 1000;
        m_audioPlayer->setPosition(int(offset));
    }
}

void MediaPlayer2Player::SetPosition(const QDBusObjectPath &trackId, qlonglong pos)
{
    qCDebug(Mpris2Log) << "MediaPlayer2Player::SetPosition(" << pos << ")";
    if (m_audioPlayer) {
        if (!m_audioPlayer->source().isEmpty()) {
            if (trackId.path() == m_currentTrackId) {
                m_audioPlayer->setPosition(int(pos / 1000));
            }
        }
    }
}

void MediaPlayer2Player::OpenUri(const QString &uri)
{
    qCDebug(Mpris2Log) << "MediaPlayer2Player::OpenUri(" << uri << ")";
    Q_UNUSED(uri);
}

void MediaPlayer2Player::playerPlaybackStateChanged()
{
    qCDebug(Mpris2Log) << "MediaPlayer2Player::playerPlaybackStateChanged()";
    signalPropertiesChange(QStringLiteral("PlaybackStatus"), PlaybackStatus());
    Q_EMIT playbackStatusChanged();
}

void MediaPlayer2Player::playerPlaybackRateChanged()
{
    qCDebug(Mpris2Log) << "MediaPlayer2Player::playerPlaybackRateChanged()";
    signalPropertiesChange(QStringLiteral("Rate"), Rate());
    // Q_EMIT rateChanged(Rate());
}

void MediaPlayer2Player::playerSeeked(qint64 position)
{
    qCDebug(Mpris2Log) << "MediaPlayer2Player::playerSeeked(" << position << ")";
    Q_EMIT Seeked(position * 1000);
}

void MediaPlayer2Player::audioPositionChanged()
{
    qCDebug(Mpris2Log) << "MediaPlayer2Player::audioPositionChanged()";
    // for progress indicator on taskbar
    if (m_audioPlayer)
        setPropertyPosition(static_cast<int>(m_audioPlayer->position()));

    // Occasionally send updated position through MPRIS to make sure that
    // audio position is still correct if playing without seeking for a long
    // time.  This will also guarantee correct playback position if the MPRIS
    // client does not support non-standard playback rates
    qlonglong position = Position();
    if (abs(position - m_lastSentPosition) > 10000000) { // every 10 seconds
        m_lastSentPosition = position;
        Q_EMIT Seeked(position);
    }
}

void MediaPlayer2Player::audioDurationChanged()
{
    qCDebug(Mpris2Log) << "MediaPlayer2Player::audioDurationChanged()";
    // We reset all metadata in case the audioDuration changed
    // This is done because duration is not yet available when setEntry is
    // called (this is before the QMediaPlayer has read the new track
    if (m_audioPlayer) {
        // qCDebug(Mpris2Log) << "Signal change of audio duration through MPRIS2" << m_audioPlayer->duration();
        if (!m_audioPlayer->source().isEmpty()) {
            m_metadata = getMetadataOfCurrentTrack();
            signalPropertiesChange(QStringLiteral("Metadata"), Metadata());
            signalPropertiesChange(QStringLiteral("CanPause"), CanPause());
            signalPropertiesChange(QStringLiteral("CanPlay"), CanPlay());
        }

        // for progress indicator on taskbar
        setPropertyPosition(static_cast<int>(m_audioPlayer->position()));
    }
}

void MediaPlayer2Player::playerVolumeChanged()
{
    qCDebug(Mpris2Log) << "MediaPlayer2Player::playerVolumeChanged()";
    if (m_audioPlayer)
        setVolume(m_audioPlayer->volume() / 100.0);
}

void MediaPlayer2Player::playerCanPlayChanged()
{
    qCDebug(Mpris2Log) << "MediaPlayer2Player::playerCanPlayChanged()";
    signalPropertiesChange(QStringLiteral("CanPlay"), CanPlay());
    // Q_EMIT canPlayChanged();
}

void MediaPlayer2Player::playerCanPauseChanged()
{
    qCDebug(Mpris2Log) << "MediaPlayer2Player::playerCanPauseChanged()";
    signalPropertiesChange(QStringLiteral("CanPause"), CanPause());
    // Q_EMIT canPauseChanged();
}

void MediaPlayer2Player::playerCanGoNextChanged()
{
    qCDebug(Mpris2Log) << "MediaPlayer2Player::playerCanGoNextChanged()";
    signalPropertiesChange(QStringLiteral("CanGoNext"), CanGoNext());
    // Q_EMIT canGoNextChanged();
}

void MediaPlayer2Player::playerCanGoPreviousChanged()
{
    qCDebug(Mpris2Log) << "MediaPlayer2Player::playerCanGoPreviousChanged()";
    signalPropertiesChange(QStringLiteral("CanGoPrevious"), CanGoNext());
    // Q_EMIT canGoPreviousChanged();
}

void MediaPlayer2Player::playerCanSeekChanged()
{
    qCDebug(Mpris2Log) << "MediaPlayer2Player::playerCanSeekChanged()";
    signalPropertiesChange(QStringLiteral("CanSeek"), CanSeek());
    // Q_EMIT canSeekChanged();
}

void MediaPlayer2Player::playerMetaDataChanged()
{
    qCDebug(Mpris2Log) << "MediaPlayer2Player::playerMetaDataChanged()";
    m_metadata = getMetadataOfCurrentTrack();
    signalPropertiesChange(QStringLiteral("Metadata"), Metadata());
}

void MediaPlayer2Player::setSource(const QUrl &source)
{
    qCDebug(Mpris2Log) << "MediaPlayer2Player::setSource(" << source << ")";
    if (source.isEmpty())
        return;

    if (m_audioPlayer) {
        if (!m_audioPlayer->source().isEmpty()) {
            if (m_audioPlayer->source() == source) {
                int queuenr = 0; // TODO: figure out smart way to handle this
                QString desktopName = QStringLiteral("/") + m_audioPlayer->desktopEntryName();
                desktopName.replace(QStringLiteral("."), QStringLiteral("/"));
                m_currentTrackId = QDBusObjectPath(desktopName + QLatin1String("/playlist/") + QString::number(queuenr)).path();

                m_metadata = getMetadataOfCurrentTrack();
                signalPropertiesChange(QStringLiteral("Metadata"), Metadata());
            }
        }
    }
}

QVariantMap MediaPlayer2Player::getMetadataOfCurrentTrack()
{
    qCDebug(Mpris2Log) << "MediaPlayer2Player::getMetadataOfCurrentTrack()";
    auto result = QVariantMap();

    if (m_currentTrackId.isEmpty()) {
        return {};
    }

    if (!m_audioPlayer) {
        return {};
    }

    if (m_audioPlayer->source().isEmpty()) {
        return {};
    }

    result[QStringLiteral("mpris:trackid")] = QVariant::fromValue<QDBusObjectPath>(QDBusObjectPath(m_currentTrackId));
    result[QStringLiteral("mpris:length")] = qlonglong(m_audioPlayer->duration()) * 1000; // convert milli-seconds into micro-seconds
    if (!m_audioPlayer->metaData()->title().isEmpty()) {
        result[QStringLiteral("xesam:title")] = m_audioPlayer->metaData()->title();
    }
    if (!m_audioPlayer->metaData()->album().isEmpty()) {
        result[QStringLiteral("xesam:album")] = m_audioPlayer->metaData()->album();
    }
    if (!m_audioPlayer->metaData()->artist().isEmpty()) {
        result[QStringLiteral("xesam:artist")] = QStringList(m_audioPlayer->metaData()->artist());
    }
    if (!m_audioPlayer->metaData()->artworkUrl().isEmpty()) {
        result[QStringLiteral("mpris:artUrl")] = m_audioPlayer->metaData()->artworkUrl().toString();
    }

    return result;
}

void MediaPlayer2Player::setPropertyPosition(int newPositionInMs)
{
    qCDebug(Mpris2Log) << "MediaPlayer2Player::setPropertyPosition(" << newPositionInMs << ")";
    // only needed for progressbar on taskbar (?)
    m_position = qlonglong(newPositionInMs) * 1000;

    /* only send new progress when it has advanced more than 1 %
     * to limit DBus traffic
     */
    const auto incrementalProgress = static_cast<double>(newPositionInMs - mPreviousProgressPosition) / m_audioPlayer->duration();
    if (mShowProgressOnTaskBar && (incrementalProgress > 0.01 || incrementalProgress < 0)) {
        mPreviousProgressPosition = newPositionInMs;
        QVariantMap parameters;
        parameters.insert(QStringLiteral("progress-visible"), true);
        parameters.insert(QStringLiteral("progress"), static_cast<double>(newPositionInMs) / m_audioPlayer->duration());

        const QString fullDesktopPath = QStringLiteral("application://") + m_audioPlayer->desktopEntryName() + QStringLiteral(".desktop");
        mProgressIndicatorSignal.setArguments({fullDesktopPath, parameters});

        QDBusConnection::sessionBus().send(mProgressIndicatorSignal);
    }
}

void MediaPlayer2Player::signalPropertiesChange(const QString &property, const QVariant &value)
{
    qCDebug(Mpris2Log) << "MediaPlayer2Player::signalPropertiesChange(" << property << value << ")";
    // qDebug() << "mpris signal property change" << property << value;
    QVariantMap properties;
    properties[property] = value;
    const int ifaceIndex = metaObject()->indexOfClassInfo("D-Bus Interface");
    QDBusMessage msg = QDBusMessage::createSignal(QStringLiteral("/org/mpris/MediaPlayer2"),
                                                  QStringLiteral("org.freedesktop.DBus.Properties"),
                                                  QStringLiteral("PropertiesChanged"));

    msg << QLatin1String(metaObject()->classInfo(ifaceIndex).value());
    msg << properties;
    msg << QStringList();

    QDBusConnection::sessionBus().send(msg);
}

bool MediaPlayer2Player::showProgressOnTaskBar() const
{
    qCDebug(Mpris2Log) << "MediaPlayer2Player::showProgressOnTaskBar()";
    return mShowProgressOnTaskBar;
}

void MediaPlayer2Player::setShowProgressOnTaskBar(bool value)
{
    qCDebug(Mpris2Log) << "MediaPlayer2Player::setShowProgressOnTaskBar(" << value << ")";
    mShowProgressOnTaskBar = value;

    QVariantMap parameters;

    if (!mShowProgressOnTaskBar || m_audioPlayer->playbackState() == KMediaSession::StoppedState || m_audioPlayer->duration() == 0) {
        parameters.insert(QStringLiteral("progress-visible"), false);
        parameters.insert(QStringLiteral("progress"), 0);
    } else {
        parameters.insert(QStringLiteral("progress-visible"), true);
        parameters.insert(QStringLiteral("progress"),
                          qRound(static_cast<double>((m_audioPlayer->duration() > 0) ? m_position / m_audioPlayer->duration() : 0)) / 1000.0);
    }

    const QString fullDesktopPath = QStringLiteral("application://") + m_audioPlayer->desktopEntryName() + QStringLiteral(".desktop");
    mProgressIndicatorSignal.setArguments({fullDesktopPath, parameters});

    QDBusConnection::sessionBus().send(mProgressIndicatorSignal);
}
