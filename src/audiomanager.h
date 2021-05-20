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

#include <memory>

#include "entry.h"

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

public:
    static AudioManager &instance()
    {
        static AudioManager _instance;
        return _instance;
    }

    ~AudioManager() override;

    Q_INVOKABLE static QString timeString(qint64 timeInMicroSeconds);

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

private Q_SLOTS:

    void mediaStatusChanged();

    void playerStateChanged();

    void playerMutedChanged();

    void playerVolumeChanged();

    void savePlayPosition(qint64 position);

private:
    explicit AudioManager(QObject *parent = nullptr);

    friend class AudioManagerPrivate;

    std::unique_ptr<AudioManagerPrivate> d;
};
