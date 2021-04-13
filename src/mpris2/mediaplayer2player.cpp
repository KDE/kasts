/**
 * SPDX-FileCopyrightText: 2014 (c) Sujith Haridasan <sujith.haridasan@kdemail.net>
 * SPDX-FileCopyrightText: 2014 (c) Ashish Madeti <ashishmadeti@gmail.com>
 * SPDX-FileCopyrightText: 2016 (c) Matthieu Gallien <matthieu_gallien@yahoo.fr>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "mediaplayer2player.h"
#include "mpris2.h"

#include "fetcher.h"
#include "datamanager.h"
#include "audiomanager.h"

#include <QCryptographicHash>
#include <QStringList>
#include <QDBusMessage>
#include <QDBusConnection>


static const double MAX_RATE = 1.0;
static const double MIN_RATE = 1.0;

MediaPlayer2Player::MediaPlayer2Player(AudioManager *audioPlayer, bool showProgressOnTaskBar, QObject* parent)
    : QDBusAbstractAdaptor(parent), m_audioPlayer(audioPlayer),
      mProgressIndicatorSignal(QDBusMessage::createSignal(QStringLiteral("/org/kde/alligator"),
                                                          QStringLiteral("com.canonical.Unity.LauncherEntry"),
                                                          QStringLiteral("Update"))),
      mShowProgressOnTaskBar(showProgressOnTaskBar)
{
    // Signals from AudioManager which are directly forwarded
    connect(m_audioPlayer, &AudioManager::sourceChanged,
            this, &MediaPlayer2Player::playerSourceChanged, Qt::QueuedConnection);

    connect(m_audioPlayer, &AudioManager::playbackRateChanged,
            this, &MediaPlayer2Player::rateChanged);
    // TODO: implement this in AudioManager, such that it can be forwarded
    //connect(m_audioPlayer, &AudioManager::minimumRateChanged,
    //        this, &MediaPlayer2Player::mimimumRateChanged);
    //connect(m_audioPlayer, &AudioManager::maximumRateChanged,
    //        this, &MediaPlayer2Player::maximumRateChanged);
    connect(m_audioPlayer, &AudioManager::canSkipForwardChanged,
            this, &MediaPlayer2Player::canGoNextChanged);
    connect(m_audioPlayer, &AudioManager::canSkipBackwardChanged,
            this, &MediaPlayer2Player::canGoPreviousChanged);
    connect(m_audioPlayer, &AudioManager::canPlayChanged,
            this, &MediaPlayer2Player::canPlayChanged);
    connect(m_audioPlayer, &AudioManager::canPauseChanged,
            this, &MediaPlayer2Player::canPauseChanged);
    connect(m_audioPlayer, &AudioManager::seekableChanged,
            this, &MediaPlayer2Player::canSeekChanged);

    // Signals which are semi-wrapped signals from AudioManager
    connect(m_audioPlayer, &AudioManager::playbackStateChanged,
            this, &MediaPlayer2Player::playerPlaybackStateChanged);
    connect(m_audioPlayer, &AudioManager::volumeChanged,
            this, &MediaPlayer2Player::playerVolumeChanged);
    connect(m_audioPlayer, &AudioManager::positionChanged,
            this, &MediaPlayer2Player::audioPositionChanged);

    // Custom signals not directly connected to AudioManager
    // 1. Metadata
    connect(m_audioPlayer, &AudioManager::durationChanged,
            this, &MediaPlayer2Player::audioDurationChanged);

    // old signal connections
    connect(m_audioPlayer, &AudioManager::positionChanged,
            this, &MediaPlayer2Player::playerSeeked);

    m_volume = m_audioPlayer->volume() / 100;
    signalPropertiesChange(QStringLiteral("Volume"), Volume());

    m_mediaPlayerPresent = 1;
    if (m_audioPlayer->entry()) {
        setCurrentTrack(DataManager::instance().getQueue().indexOf(m_audioPlayer->entry()->id()));
    }
}

MediaPlayer2Player::~MediaPlayer2Player()
= default;

QString MediaPlayer2Player::PlaybackStatus() const
{
    QString result;

    if (m_audioPlayer->playbackState() == QMediaPlayer::StoppedState) {
        result = QStringLiteral("Stopped");
    } else if (m_audioPlayer->playbackState() == QMediaPlayer::PlayingState) {
        result = QStringLiteral("Playing");
    } else {
        result = QStringLiteral("Paused");
    }

    return result;
}

bool MediaPlayer2Player::CanGoNext() const
{
    if (m_audioPlayer)
        return m_audioPlayer->canSkipForward();
    else
        return false;
}

void MediaPlayer2Player::Next()
{
    if (m_audioPlayer)
        m_audioPlayer->skipForward();
}

bool MediaPlayer2Player::CanGoPrevious() const
{
    if (m_audioPlayer)
        return m_audioPlayer->canSkipBackward();
    else
        return false;
}


void MediaPlayer2Player::Previous()
{
    if (m_audioPlayer)
        m_audioPlayer->skipBackward();
}

bool MediaPlayer2Player::CanPause() const
{
    if (m_audioPlayer)
        return m_audioPlayer->canPause();
    else
        return false;
}

void MediaPlayer2Player::Pause()
{
    if (m_audioPlayer)
        m_audioPlayer->pause();
}

void MediaPlayer2Player::PlayPause()
{
    if (m_audioPlayer)
        m_audioPlayer->playPause();
}

void MediaPlayer2Player::Stop()
{
    // we actually don't really want to stop, because that would reset the player
    // position, so we pause instead
    if (m_audioPlayer)
        m_audioPlayer->pause();
}

bool MediaPlayer2Player::CanPlay() const
{
    if (m_audioPlayer)
        return m_audioPlayer->canPlay();
    else
        return false;
}

void MediaPlayer2Player::Play()
{
    if (m_audioPlayer)
        m_audioPlayer->play();
}

double MediaPlayer2Player::Volume() const
{
    return m_volume;
}

void MediaPlayer2Player::setVolume(double volume)
{
    m_volume= qBound(0.0, volume, 1.0);
    emit volumeChanged(m_volume);

    m_audioPlayer->setVolume(100 * m_volume);

    signalPropertiesChange(QStringLiteral("Volume"), Volume());
}

QVariantMap MediaPlayer2Player::Metadata() const
{
    return m_metadata;
}

qlonglong MediaPlayer2Player::Position() const
{
    return qlonglong(m_audioPlayer->position()) * 1000;
}

double MediaPlayer2Player::Rate() const
{
    return m_audioPlayer->playbackRate();
}

void MediaPlayer2Player::setRate(double newRate)
{
    if (newRate <= 0.0001 && newRate >= -0.0001) {
        Pause();
    } else {
        m_audioPlayer->setPlaybackRate(qBound(MinimumRate(), newRate, MaximumRate()));
    }
}

double MediaPlayer2Player::MinimumRate() const
{
    return MIN_RATE;
}

double MediaPlayer2Player::MaximumRate() const
{
    return MAX_RATE;
}

bool MediaPlayer2Player::CanSeek() const
{
    return m_audioPlayer->seekable();
}

bool MediaPlayer2Player::CanControl() const
{
    return true;
}

void MediaPlayer2Player::Seek(qlonglong Offset)
{
    if (m_audioPlayer) {
        auto offset = (m_position + Offset) / 1000;
        m_audioPlayer->seek(int(offset));
    }
}

void MediaPlayer2Player::SetPosition(const QDBusObjectPath &trackId, qlonglong pos)
{
    if (m_audioPlayer) {
        if (m_audioPlayer->entry()) {
            if (trackId.path() == m_audioPlayer->entry()->id()) {
                m_audioPlayer->seek(int(pos / 1000));
            }
        }
    }
}

void MediaPlayer2Player::OpenUri(const QString &uri)
{
    Q_UNUSED(uri);
}

void MediaPlayer2Player::playerSourceChanged()
{
    // TODO: refactor this; too complicated for this player
    setCurrentTrack(DataManager::instance().getQueue().indexOf(m_audioPlayer->entry()->id()));
}


void MediaPlayer2Player::playerPlaybackStateChanged()
{
    signalPropertiesChange(QStringLiteral("PlaybackStatus"), PlaybackStatus());
    emit playbackStatusChanged();
}

void MediaPlayer2Player::audioPositionChanged()
{
    setPropertyPosition(static_cast<int>(m_audioPlayer->position()));
}

void MediaPlayer2Player::playerSeeked(qint64 position)
{
    Q_EMIT Seeked(position * 1000);
}

void MediaPlayer2Player::audioDurationChanged()
{
    m_metadata = getMetadataOfCurrentTrack();
    signalPropertiesChange(QStringLiteral("Metadata"), Metadata());

    playerPlaybackStateChanged();
    setPropertyPosition(static_cast<int>(m_audioPlayer->position()));
}

void MediaPlayer2Player::playerVolumeChanged()
{
    setVolume(m_audioPlayer->volume() / 100.0);
}

int MediaPlayer2Player::currentTrack() const
{
    return DataManager::instance().getQueue().indexOf(m_audioPlayer->entry()->id());
}

void MediaPlayer2Player::setCurrentTrack(int newTrackPosition)
{
    m_currentTrack = m_audioPlayer->entry()->title();
    m_currentTrackId = QDBusObjectPath(QLatin1String("/org/kde/alligator/playlist/") + QString::number(newTrackPosition)).path();

    emit currentTrackChanged();

    m_metadata = getMetadataOfCurrentTrack();
    signalPropertiesChange(QStringLiteral("Metadata"), Metadata());
}

QVariantMap MediaPlayer2Player::getMetadataOfCurrentTrack()
{
    auto result = QVariantMap();

    if (m_currentTrackId.isEmpty()) {
        return {};
    }

    Entry* entry = m_audioPlayer->entry();

    result[QStringLiteral("mpris:trackid")] = QVariant::fromValue<QDBusObjectPath>(QDBusObjectPath(m_currentTrackId));
    result[QStringLiteral("mpris:length")] = qlonglong(m_audioPlayer->duration()) * 1000;
    //convert milli-seconds into micro-seconds
    if (!entry->title().isEmpty()) {
        result[QStringLiteral("xesam:title")] = entry->title();
    }
    result[QStringLiteral("xesam:url")] = entry->enclosure()->path();
    if (!entry->feed()->name().isEmpty()) {
        result[QStringLiteral("xesam:album")] = entry->feed()->name();
    }
    if (entry->authors().count() > 0) {
        QStringList authors;
        for (auto &author : entry->authors()) authors.append(author->name());
        result[QStringLiteral("xesam:artist")] = authors;
    }
    if (!entry->image().isEmpty()) {
        result[QStringLiteral("mpris:artUrl")] = Fetcher::instance().imagePath(entry->image());
    }

    return result;
}

int MediaPlayer2Player::mediaPlayerPresent() const
{
    return m_mediaPlayerPresent;
}

void MediaPlayer2Player::setMediaPlayerPresent(int status)
{
    if (m_mediaPlayerPresent != status) {
        m_mediaPlayerPresent = status;
        emit mediaPlayerPresentChanged();

        signalPropertiesChange(QStringLiteral("CanGoNext"), CanGoNext());
        signalPropertiesChange(QStringLiteral("CanGoPrevious"), CanGoPrevious());
        signalPropertiesChange(QStringLiteral("CanPause"), CanPause());
        signalPropertiesChange(QStringLiteral("CanPlay"), CanPlay());
        emit canGoNextChanged();
        emit canGoPreviousChanged();
        emit canPauseChanged();
        emit canPlayChanged();
    }
}

void MediaPlayer2Player::setPropertyPosition(int newPositionInMs)
{
    m_position = qlonglong(newPositionInMs) * 1000;

    /* only send new progress when it has advanced more than 1 %
     * to limit DBus traffic
     */
    const auto incrementalProgress = static_cast<double>(newPositionInMs - mPreviousProgressPosition) / m_audioPlayer->duration();
    if (mShowProgressOnTaskBar && (incrementalProgress > 0.01 || incrementalProgress < 0))
    {
        mPreviousProgressPosition = newPositionInMs;
        QVariantMap parameters;
        parameters.insert(QStringLiteral("progress-visible"), true);
        parameters.insert(QStringLiteral("progress"), static_cast<double>(newPositionInMs) / m_audioPlayer->duration());

        mProgressIndicatorSignal.setArguments({QStringLiteral("application://org.kde.alligator.desktop"), parameters});

        QDBusConnection::sessionBus().send(mProgressIndicatorSignal);
    }
}

void MediaPlayer2Player::signalPropertiesChange(const QString &property, const QVariant &value)
{
    QVariantMap properties;
    properties[property] = value;
    const int ifaceIndex = metaObject()->indexOfClassInfo("D-Bus Interface");
    QDBusMessage msg = QDBusMessage::createSignal(QStringLiteral("/org/mpris/MediaPlayer2"),
                                                  QStringLiteral("org.freedesktop.DBus.Properties"), QStringLiteral("PropertiesChanged"));

    msg << QLatin1String(metaObject()->classInfo(ifaceIndex).value());
    msg << properties;
    msg << QStringList();

    QDBusConnection::sessionBus().send(msg);
}

bool MediaPlayer2Player::showProgressOnTaskBar() const
{
    return mShowProgressOnTaskBar;
}

void MediaPlayer2Player::setShowProgressOnTaskBar(bool value)
{
    mShowProgressOnTaskBar = value;

    QVariantMap parameters;

    if (!mShowProgressOnTaskBar || m_audioPlayer->playbackState() == QMediaPlayer::StoppedState || m_audioPlayer->duration() == 0) {
        parameters.insert(QStringLiteral("progress-visible"), false);
        parameters.insert(QStringLiteral("progress"), 0);
    } else {
        parameters.insert(QStringLiteral("progress-visible"), true);
        parameters.insert(QStringLiteral("progress"), qRound(static_cast<double>(m_position / m_audioPlayer->duration())) / 1000.0);
    }

    mProgressIndicatorSignal.setArguments({QStringLiteral("application://org.kde.alligator.desktop"), parameters});

    QDBusConnection::sessionBus().send(mProgressIndicatorSignal);
}
