/**
 * SPDX-FileCopyrightText: 2022-2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#pragma once

#include <gst/gst.h>
#include <memory>

#include <QObject>
#include <QUrl>

#include "abstractmediabackend.h"
#include "kmediasession.h"

class GstMediaBackendPrivate;

class GstMediaBackend : public AbstractMediaBackend
{
    Q_OBJECT

public:
    explicit GstMediaBackend(QObject *parent);
    ~GstMediaBackend();

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
    friend class GstMediaBackendPrivate;
    std::unique_ptr<GstMediaBackendPrivate> d;

    void handleMessage(GstMessage *message);
    void timerUpdate();
};
