/**
 * SPDX-FileCopyrightText: 2017 (c) Matthieu Gallien <matthieu_gallien@yahoo.fr>
 * SPDX-FileCopyrightText: 2021 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once

#include <QMediaPlayer>
#include <QObject>
#include <QString>
#include <QUrl>

#include <KFormat>

#include <memory>

#include "entry.h"
#include "error.h"

class AudioManagerPrivate;

class AudioManager : public QObject
{
    Q_OBJECT

    Q_PROPERTY(Entry *entry READ entry WRITE setEntry NOTIFY entryChanged)
    Q_PROPERTY(bool muted READ muted WRITE setMuted NOTIFY mutedChanged)
    Q_PROPERTY(qreal volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(QMediaPlayer::MediaStatus status READ status NOTIFY statusChanged)
    Q_PROPERTY(QMediaPlayer::State playbackState READ playbackState NOTIFY playbackStateChanged)
    Q_PROPERTY(qreal playbackRate READ playbackRate WRITE setPlaybackRate NOTIFY playbackRateChanged)
    Q_PROPERTY(QMediaPlayer::Error error READ error NOTIFY errorChanged)
    Q_PROPERTY(qint64 duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(qint64 position READ position WRITE setPosition NOTIFY positionChanged)
    Q_PROPERTY(bool seekable READ seekable NOTIFY seekableChanged)
    Q_PROPERTY(bool canPlay READ canPlay NOTIFY canPlayChanged)
    Q_PROPERTY(bool canSkipForward READ canSkipForward NOTIFY canSkipForwardChanged)
    Q_PROPERTY(bool canSkipBackward READ canSkipBackward NOTIFY canSkipBackwardChanged)
    Q_PROPERTY(bool canGoNext READ canGoNext NOTIFY canGoNextChanged)
    Q_PROPERTY(QString formattedLeftDuration READ formattedLeftDuration NOTIFY positionChanged)
    Q_PROPERTY(QString formattedDuration READ formattedDuration NOTIFY durationChanged)
    Q_PROPERTY(QString formattedPosition READ formattedPosition NOTIFY positionChanged)
    Q_PROPERTY(qint64 sleepTime READ sleepTime WRITE setSleepTimer RESET stopSleepTimer NOTIFY sleepTimerChanged)
    Q_PROPERTY(qint64 remainingSleepTime READ remainingSleepTime NOTIFY remainingSleepTimeChanged)
    Q_PROPERTY(QString formattedRemainingSleepTime READ formattedRemainingSleepTime NOTIFY remainingSleepTimeChanged)

public:
    const double MAX_RATE = 1.0;
    const double MIN_RATE = 2.5;
    const qint64 SKIP_STEP = 10000;
    const qint64 SKIP_TRACK_END = 15000;

    static AudioManager &instance()
    {
        static AudioManager _instance;
        return _instance;
    }

    ~AudioManager() override;

    [[nodiscard]] Entry *entry() const;
    [[nodiscard]] bool muted() const;
    [[nodiscard]] qreal volume() const;
    [[nodiscard]] QUrl source() const;
    [[nodiscard]] QMediaPlayer::MediaStatus status() const;
    [[nodiscard]] QMediaPlayer::State playbackState() const;
    [[nodiscard]] qreal playbackRate() const;
    [[nodiscard]] qreal minimumPlaybackRate() const;
    [[nodiscard]] qreal maximumPlaybackRate() const;
    [[nodiscard]] QMediaPlayer::Error error() const;
    [[nodiscard]] qint64 duration() const;
    [[nodiscard]] qint64 position() const;
    [[nodiscard]] bool seekable() const;
    [[nodiscard]] bool canPlay() const;
    [[nodiscard]] bool canPause() const;
    [[nodiscard]] bool canSkipForward() const;
    [[nodiscard]] bool canSkipBackward() const;
    [[nodiscard]] bool canGoNext() const;

    QString formattedDuration() const;
    QString formattedLeftDuration() const;
    QString formattedPosition() const;

    qint64 sleepTime() const; // returns originally set sleep time
    qint64 remainingSleepTime() const; // returns remaining sleep time
    QString formattedRemainingSleepTime() const;

Q_SIGNALS:

    void entryChanged(Entry *entry);
    void mutedChanged(bool muted);
    void volumeChanged();
    void sourceChanged();
    void statusChanged(QMediaPlayer::MediaStatus status);
    void playbackStateChanged(QMediaPlayer::State state);
    void playbackRateChanged(qreal rate);
    void errorChanged(QMediaPlayer::Error error);
    void durationChanged(qint64 duration);
    void positionChanged(qint64 position);
    void seekableChanged(bool seekable);
    void playing();
    void paused();
    void stopped();
    void canPlayChanged();
    void canPauseChanged();
    void canSkipForwardChanged();
    void canSkipBackwardChanged();
    void canGoNextChanged();

    void sleepTimerChanged(qint64 duration);
    void remainingSleepTimeChanged(qint64 duration);

    void logError(Error::Type type, const QString &url, const QString &id, const int errorId, const QString &errorString, const QString &title);

public Q_SLOTS:

    void setEntry(Entry *entry);
    void setMuted(bool muted);
    void setVolume(qreal volume);
    // void setSource(const QUrl &source);  //source should only be set by audiomanager itself
    void setPosition(qint64 position);
    void setPlaybackRate(qreal rate);
    void play();
    void pause();
    void playPause();
    void stop();
    void seek(qint64 position);
    void skipBackward();
    void skipForward();
    void next();

    void setSleepTimer(qint64 duration);
    void stopSleepTimer();

private Q_SLOTS:

    void mediaStatusChanged();
    void playerStateChanged();
    void playerDurationChanged(qint64 duration);
    void playerMutedChanged();
    void playerVolumeChanged();
    void savePlayPosition();
    void prepareAudio();
    void checkForPendingSeek();

private:
    explicit AudioManager(QObject *parent = nullptr);

    friend class AudioManagerPrivate;

    std::unique_ptr<AudioManagerPrivate> d;
    KFormat m_kformat;
};
