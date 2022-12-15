/**
 * SPDX-FileCopyrightText: 2022-2023 Bart De Vries <bart@mogwai.be>
 * SPDX-FileCopyrightText: 2017 Matthieu Gallien <matthieu_gallien@yahoo.fr>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once

#include <memory>

#include <QObject>
#include <QString>
#include <QUrl>

#include "abstractmediabackend.h"
#include "kmediasession.h"

class VlcMediaBackendPrivate;

class VlcMediaBackend : public AbstractMediaBackend
{
    Q_OBJECT

public:
    VlcMediaBackend(QObject *parent);
    ~VlcMediaBackend();

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
    friend class VlcMediaBackendPrivate;
    std::unique_ptr<VlcMediaBackendPrivate> d;

    void playerStateSignalChanges(KMediaSession::PlaybackState newState);
    void mediaStatusSignalChanges(KMediaSession::MediaStatus newStatus);
    void playerErrorSignalChanges(KMediaSession::Error error);
    void playerDurationSignalChanges(qint64 newDuration);
    void playerPositionSignalChanges(qint64 newPosition);
    void playerVolumeSignalChanges(qreal volume);
    void playerMutedSignalChanges(bool isMuted);
    void playerSeekableSignalChanges(bool isSeekable);

    void setPlayerName(const QString &name);
    void setDesktopEntryName(const QString &name);
};
