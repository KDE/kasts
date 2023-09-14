/**
 * SPDX-FileCopyrightText: 2022-2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#pragma once

#include <memory>

#include <QMediaPlayer>
#include <QObject>
#include <QString>
#include <QUrl>

#include "abstractmediabackend.h"
#include "kmediasession.h"

class QtMediaBackendPrivate;

class QtMediaBackend : public AbstractMediaBackend
{
    Q_OBJECT

public:
    QtMediaBackend(QObject *parent);
    ~QtMediaBackend();

    KMediaSession::MediaBackends backend() const override;

    bool muted() const override;
    qreal volume() const override;
    QUrl source() const override;
    KMediaSession::MediaStatus mediaStatus() const override;
    KMediaSession::PlaybackState playbackState() const override;
    qreal playbackRate() const override;
    KMediaSession::Error error() const override;
    qint64 duration() const override;
    qint64 position() const override;
    bool seekable() const override;

public Q_SLOTS:
    void setMuted(bool muted) override;
    void setVolume(qreal volume) override;
    void setSource(const QUrl &source) override;
    void setPosition(qint64 position) override;
    void setPlaybackRate(qreal rate) override;

    void play() override;
    void pause() override;
    void stop() override;

private:
    friend class QtMediaBackendPrivate;
    std::unique_ptr<QtMediaBackendPrivate> d;

    void playerMutedSignalChanges(bool muted);
    void mediaStatusSignalChanges(const QMediaPlayer::MediaStatus &qtMediaStatus);
    void playerVolumeSignalChanges(float volume);
    void playerSourceSignalChanges(const QUrl &media);
    void playerStateSignalChanges(const QMediaPlayer::PlaybackState &qtPlaybackState);
    void playerPlaybackRateSignalChanges(const qreal &playbackRate);
    void playerErrorSignalChanges(const QMediaPlayer::Error &error);
    void playerDurationSignalChanges(qint64 newDuration);
    void playerPositionSignalChanges(qint64 newPosition);
    void playerSeekableSignalChanges(bool isSeekable);
    void playerMetaDataSignalChanges();
};
