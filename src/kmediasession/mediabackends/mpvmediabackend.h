/**
 * SPDX-FileCopyrightText: 2024 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#pragma once

#include <QObject>
#include <QUrl>

#include "abstractmediabackend.h"
#include "kmediasession.h"

class MpvMediaBackendPrivate;

class MpvMediaBackend : public AbstractMediaBackend
{
    Q_OBJECT

public:
    explicit MpvMediaBackend(QObject *parent);
    ~MpvMediaBackend();

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
    friend class MpvMediaBackendPrivate;
    std::unique_ptr<MpvMediaBackendPrivate> d;
};
