/*
   SPDX-FileCopyrightText: 2017 (c) Matthieu Gallien <matthieu_gallien@yahoo.fr>

   SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once

#include <QObject>
#include <QUrl>
#include <QMediaPlayer>
#include <QString>

#include <memory>

#include "entry.h"

class AudioManagerPrivate;

class AudioManager : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool playerOpen
               READ playerOpen
               WRITE setPlayerOpen
               NOTIFY playerOpenChanged)

    Q_PROPERTY(Entry* entry
               READ entry
               WRITE setEntry
               NOTIFY entryChanged)

    Q_PROPERTY(bool muted
               READ muted
               WRITE setMuted
               NOTIFY mutedChanged)

    Q_PROPERTY(qreal volume
               READ volume
               WRITE setVolume
               NOTIFY volumeChanged)

    Q_PROPERTY(QUrl source
               READ source
               WRITE setSource
               NOTIFY sourceChanged)

    Q_PROPERTY(QMediaPlayer::MediaStatus status
               READ status
               NOTIFY statusChanged)

    Q_PROPERTY(QMediaPlayer::State playbackState
               READ playbackState
               NOTIFY playbackStateChanged)

    Q_PROPERTY(QMediaPlayer::Error error
               READ error
               NOTIFY errorChanged)

    Q_PROPERTY(qint64 duration
               READ duration
               NOTIFY durationChanged)

    Q_PROPERTY(qint64 position
               READ position
               WRITE setPosition
               NOTIFY positionChanged)

    Q_PROPERTY(bool seekable
               READ seekable
               NOTIFY seekableChanged)

public:

    explicit AudioManager(QObject *parent = nullptr);

    static AudioManager &instance()
    {
        static AudioManager _instance;
        return _instance;
    }

    ~AudioManager() override;

    [[nodiscard]] Entry* entry() const;

    [[nodiscard]] bool playerOpen() const;

    [[nodiscard]] bool muted() const;

    [[nodiscard]] qreal volume() const;

    [[nodiscard]] QUrl source() const;

    [[nodiscard]] QMediaPlayer::MediaStatus status() const;

    [[nodiscard]] QMediaPlayer::State playbackState() const;

    [[nodiscard]] QMediaPlayer::Error error() const;

    [[nodiscard]] qint64 duration() const;

    [[nodiscard]] qint64 position() const;

    [[nodiscard]] bool seekable() const;

Q_SIGNALS:

    void playerOpenChanged();

    void entryChanged();

    void mutedChanged(bool muted);

    void volumeChanged();

    void sourceChanged();

    void statusChanged(QMediaPlayer::MediaStatus status);

    void playbackStateChanged(QMediaPlayer::State state);

    void errorChanged(QMediaPlayer::Error error);

    void durationChanged(qint64 duration);

    void positionChanged(qint64 position);

    void seekableChanged(bool seekable);

    void playing();

    void paused();

    void stopped();

public Q_SLOTS:

    void setEntry(Entry* entry);

    void setPlayerOpen(bool state);

    void setMuted(bool muted);

    void setVolume(qreal volume);

    void setSource(const QUrl &source);

    void setPosition(qint64 position);

    void play();

    void pause();

    void stop();

    void seek(qint64 position);

private Q_SLOTS:

    void mediaStatusChanged();

    void playerStateChanged();

    void playerMutedChanged();

    void playerVolumeChanged();

private:

    friend class AudioManagerPrivate;

    std::unique_ptr<AudioManagerPrivate> d;

};
