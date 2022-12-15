/**
 * SPDX-FileCopyrightText: 2022-2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#pragma once

#include <QObject>
#include <QUrl>

#include "kmediasession.h"

class AbstractMediaBackend : public QObject
{
    Q_OBJECT

public:
    inline AbstractMediaBackend(QObject *parent)
        : QObject(parent){};
    inline ~AbstractMediaBackend(){};

    virtual KMediaSession::MediaBackends backend() const = 0;

    virtual bool muted() const = 0;
    virtual qreal volume() const = 0;
    virtual QUrl source() const = 0;
    virtual KMediaSession::MediaStatus mediaStatus() const = 0;
    virtual KMediaSession::PlaybackState playbackState() const = 0;
    virtual qreal playbackRate() const = 0;
    virtual KMediaSession::Error error() const = 0;
    virtual qint64 duration() const = 0;
    virtual qint64 position() const = 0;
    virtual bool seekable() const = 0;

Q_SIGNALS:
    /* In the derived class, make sure to only emit the signals from this
     * abstract class. It is not needed to re-define (nor emit) these signals
     * in the derived class.
     */
    void mutedChanged(bool muted);
    void volumeChanged(qreal volume);
    void sourceChanged(const QUrl &source);
    void mediaStatusChanged(KMediaSession::MediaStatus status);
    void playbackStateChanged(KMediaSession::PlaybackState state);
    void playbackRateChanged(qreal rate);
    void errorChanged(KMediaSession::Error error);
    void durationChanged(qint64 duration);
    void positionChanged(qint64 position);
    void seekableChanged(bool seekable);

public Q_SLOTS:
    virtual void setMuted(bool muted) = 0;
    virtual void setVolume(qreal volume) = 0;
    virtual void setSource(const QUrl &source) = 0;
    virtual void setPosition(qint64 position) = 0;
    virtual void setPlaybackRate(qreal rate) = 0;

    virtual void play() = 0;
    virtual void pause() = 0;
    virtual void stop() = 0;
};
