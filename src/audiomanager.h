/**
 * SPDX-FileCopyrightText: 2017 (c) Matthieu Gallien <matthieu_gallien@yahoo.fr>
 * SPDX-FileCopyrightText: 2021-2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once

#include <kmediasession/kmediasession.h>
#include <memory>

#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QUrl>

#include <KFormat>

#include "entry.h"
#include "error.h"

class AudioManagerPrivate;

class AudioManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(KMediaSession::MediaBackends currentBackend READ currentBackend WRITE setCurrentBackend NOTIFY currentBackendChanged)
    Q_PROPERTY(QList<KMediaSession::MediaBackends> availableBackends READ availableBackends CONSTANT)

    Q_PROPERTY(Entry *entry READ entry WRITE setEntry NOTIFY entryChanged)
    Q_PROPERTY(bool muted READ muted WRITE setMuted NOTIFY mutedChanged)
    Q_PROPERTY(qreal volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(KMediaSession::MediaStatus status READ status NOTIFY statusChanged)
    Q_PROPERTY(KMediaSession::PlaybackState playbackState READ playbackState NOTIFY playbackStateChanged)
    Q_PROPERTY(qreal playbackRate READ playbackRate WRITE setPlaybackRate NOTIFY playbackRateChanged)
    Q_PROPERTY(KMediaSession::Error error READ error NOTIFY errorChanged)
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
    Q_PROPERTY(bool isStreaming READ isStreaming NOTIFY isStreamingChanged)

public:
    static AudioManager &instance()
    {
        static AudioManager _instance;
        return _instance;
    }
    static AudioManager *create(QQmlEngine *engine, QJSEngine *)
    {
        engine->setObjectOwnership(&instance(), QQmlEngine::CppOwnership);
        return &instance();
    }

    ~AudioManager() override;

    [[nodiscard]] Q_INVOKABLE QString backendName(KMediaSession::MediaBackends backend) const;
    [[nodiscard]] KMediaSession::MediaBackends currentBackend() const;
    [[nodiscard]] QList<KMediaSession::MediaBackends> availableBackends() const;

    [[nodiscard]] Entry *entry() const;
    [[nodiscard]] bool muted() const;
    [[nodiscard]] qreal volume() const;
    [[nodiscard]] QUrl source() const;
    [[nodiscard]] KMediaSession::MediaStatus status() const;
    [[nodiscard]] KMediaSession::PlaybackState playbackState() const;
    [[nodiscard]] qreal playbackRate() const;
    [[nodiscard]] qreal minimumPlaybackRate() const;
    [[nodiscard]] qreal maximumPlaybackRate() const;
    [[nodiscard]] KMediaSession::Error error() const;
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

    bool isStreaming() const;

Q_SIGNALS:
    void currentBackendChanged(KMediaSession::MediaBackends backend);

    void entryChanged(Entry *entry);
    void mutedChanged(bool muted);
    void volumeChanged();
    void sourceChanged();
    void statusChanged(KMediaSession::MediaStatus status);
    void playbackStateChanged(KMediaSession::PlaybackState state);
    void playbackRateChanged(qreal rate);
    void errorChanged(KMediaSession::Error error);
    void durationChanged(qint64 duration);
    void positionChanged(qint64 position);
    void seekableChanged(bool seekable);
    void canPlayChanged();
    void canPauseChanged();
    void canSkipForwardChanged();
    void canSkipBackwardChanged();
    void canGoNextChanged();

    void sleepTimerChanged(qint64 duration);
    void remainingSleepTimeChanged(qint64 duration);

    void isStreamingChanged();

    void logError(Error::Type type, const QString &url, const QString &id, const int errorId, const QString &errorString, const QString &title);

    // mpris2 signals
    void raiseWindowRequested();
    void quitRequested();

public Q_SLOTS:
    void setCurrentBackend(KMediaSession::MediaBackends backend);

    void setEntry(Entry *entry);
    void setMuted(bool muted);
    void setVolume(qreal volume);
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
    void playerDurationChanged(qint64 duration);
    void playerMutedChanged();
    void playerVolumeChanged();
    void savePlayPosition();
    void setEntryInfo(Entry *entry);
    void prepareAudio(const QUrl &loadUrl);
    void checkForPendingSeek();
    void updateMetaData();

private:
    explicit AudioManager(QObject *parent = nullptr);

    friend class AudioManagerPrivate;

    std::unique_ptr<AudioManagerPrivate> d;
    KFormat m_kformat;
};
