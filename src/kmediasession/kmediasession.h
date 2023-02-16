/**
 * SPDX-FileCopyrightText: 2022-2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#pragma once

#include "kmediasession_export.h"

#include <memory>

#include <QList>
#include <QObject>
#include <QString>
#include <QUrl>

#include "metadata.h"

class KMediaSessionPrivate;

class KMEDIASESSION_EXPORT KMediaSession : public QObject
{
    Q_OBJECT

    Q_PROPERTY(KMediaSession::MediaBackends currentBackend READ currentBackend WRITE setCurrentBackend NOTIFY currentBackendChanged)
    Q_PROPERTY(QList<KMediaSession::MediaBackends> availableBackends READ availableBackends CONSTANT)

    Q_PROPERTY(QString playerName READ playerName WRITE setPlayerName NOTIFY playerNameChanged)
    Q_PROPERTY(QString desktopEntryName READ desktopEntryName WRITE setDesktopEntryName NOTIFY desktopEntryNameChanged)
    Q_PROPERTY(bool mpris2PauseInsteadOfStop READ mpris2PauseInsteadOfStop WRITE setMpris2PauseInsteadOfStop NOTIFY mpris2PauseInsteadOfStopChanged)

    Q_PROPERTY(bool muted READ muted WRITE setMuted NOTIFY mutedChanged)
    Q_PROPERTY(qreal volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(MediaStatus mediaStatus READ mediaStatus NOTIFY mediaStatusChanged)
    Q_PROPERTY(PlaybackState playbackState READ playbackState NOTIFY playbackStateChanged)
    Q_PROPERTY(qreal playbackRate READ playbackRate WRITE setPlaybackRate NOTIFY playbackRateChanged)
    Q_PROPERTY(Error error READ error NOTIFY errorChanged)
    Q_PROPERTY(qint64 position READ position WRITE setPosition NOTIFY positionChanged)
    Q_PROPERTY(qint64 duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(bool seekable READ seekable NOTIFY seekableChanged)

    Q_PROPERTY(MetaData *metaData READ metaData WRITE setMetaData NOTIFY metaDataChanged)

    Q_PROPERTY(bool canPlay READ canPlay NOTIFY canPlayChanged)
    Q_PROPERTY(bool canPause READ canPause NOTIFY canPauseChanged)
    Q_PROPERTY(bool canGoNext READ canGoNext WRITE setCanGoNext NOTIFY canGoNextChanged)
    Q_PROPERTY(bool canGoPrevious READ canGoPrevious WRITE setCanGoPrevious NOTIFY canGoPreviousChanged)

public:
    enum MediaBackends {
        Qt = 0,
        Vlc = 1,
        Gst = 2,
    };
    Q_ENUM(MediaBackends)

    enum MediaStatus {
        UnknownMediaStatus = 0,
        NoMedia,
        LoadingMedia,
        LoadedMedia,
        StalledMedia,
        BufferingMedia,
        BufferedMedia,
        EndOfMedia,
        InvalidMedia,
    };
    Q_ENUM(MediaStatus)

    enum PlaybackState {
        StoppedState = 0,
        PlayingState,
        PausedState,
    };
    Q_ENUM(PlaybackState)

    enum Error {
        NoError = 0,
        ResourceError,
        FormatError,
        NetworkError,
        AccessDeniedError,
        ServiceMissingError,
    };
    Q_ENUM(Error)

    explicit KMediaSession(const QString &playerName = QStringLiteral(""), const QString &desktopEntryName = QStringLiteral(""), QObject *parent = nullptr);
    ~KMediaSession();

    [[nodiscard]] Q_INVOKABLE QString backendName(KMediaSession::MediaBackends backend) const;
    [[nodiscard]] KMediaSession::MediaBackends currentBackend() const;
    [[nodiscard]] QList<KMediaSession::MediaBackends> availableBackends() const;

    [[nodiscard]] QString playerName() const;
    [[nodiscard]] QString desktopEntryName() const;
    [[nodiscard]] bool mpris2PauseInsteadOfStop() const;

    [[nodiscard]] bool muted() const;
    [[nodiscard]] qreal volume() const;
    [[nodiscard]] QUrl source() const;
    [[nodiscard]] KMediaSession::MediaStatus mediaStatus() const;
    [[nodiscard]] KMediaSession::PlaybackState playbackState() const;
    [[nodiscard]] qreal playbackRate() const;
    [[nodiscard]] KMediaSession::Error error() const;
    [[nodiscard]] qint64 duration() const;
    [[nodiscard]] qint64 position() const;
    [[nodiscard]] bool seekable() const;

    [[nodiscard]] MetaData *metaData() const;

    [[nodiscard]] bool canPlay() const;
    [[nodiscard]] bool canPause() const;
    [[nodiscard]] bool canGoNext() const;
    [[nodiscard]] bool canGoPrevious() const;

Q_SIGNALS:
    void currentBackendChanged(KMediaSession::MediaBackends backend);

    void playerNameChanged(const QString &name);
    void desktopEntryNameChanged(const QString &name);
    void mpris2PauseInsteadOfStopChanged(bool newState);

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

    void metaDataChanged(MetaData *metadata);

    void canPlayChanged(bool newState);
    void canPauseChanged(bool newState);
    void canGoNextChanged(bool newState);
    void canGoPreviousChanged(bool newState);

    // applications should connect to these signals and implement respective actions
    void nextRequested();
    void previousRequested();
    void raiseWindowRequested();
    void quitRequested();

public Q_SLOTS:
    void setCurrentBackend(KMediaSession::MediaBackends backend);

    void setPlayerName(const QString &name);
    void setDesktopEntryName(const QString &name);
    void setMpris2PauseInsteadOfStop(bool newState);

    void setMuted(bool muted);
    void setVolume(qreal volume);
    void setSource(const QUrl &source);
    void setPosition(qint64 position);
    void setPlaybackRate(qreal rate);

    void setMetaData(MetaData *metaData);

    void setCanGoNext(bool newState);
    void setCanGoPrevious(bool newState);

    Q_INVOKABLE void play();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void stop();

private:
    friend class KMediaSessionPrivate;
    std::unique_ptr<KMediaSessionPrivate> d;
};
