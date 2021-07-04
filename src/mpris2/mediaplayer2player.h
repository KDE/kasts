/**
 * SPDX-FileCopyrightText: 2014 (c) Sujith Haridasan <sujith.haridasan@kdemail.net>
 * SPDX-FileCopyrightText: 2014 (c) Ashish Madeti <ashishmadeti@gmail.com>
 * SPDX-FileCopyrightText: 2016 (c) Matthieu Gallien <matthieu_gallien@yahoo.fr>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QDBusAbstractAdaptor>
#include <QDBusMessage>
#include <QDBusObjectPath>

class AudioManager;
class Entry;
class Feed;

class MediaPlayer2Player : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2.Player") // Docs: https://specifications.freedesktop.org/mpris-spec/latest/Player_Interface.html

    Q_PROPERTY(QString PlaybackStatus READ PlaybackStatus NOTIFY playbackStatusChanged)
    Q_PROPERTY(double Rate READ Rate WRITE setRate NOTIFY rateChanged)
    Q_PROPERTY(QVariantMap Metadata READ Metadata NOTIFY playbackStatusChanged)
    Q_PROPERTY(double Volume READ Volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(qlonglong Position READ Position WRITE setPropertyPosition NOTIFY playbackStatusChanged)
    Q_PROPERTY(double MinimumRate READ MinimumRate CONSTANT)
    Q_PROPERTY(double MaximumRate READ MaximumRate CONSTANT)
    Q_PROPERTY(bool CanGoNext READ CanGoNext NOTIFY canGoNextChanged)
    Q_PROPERTY(bool CanGoPrevious READ CanGoPrevious NOTIFY canGoPreviousChanged)
    Q_PROPERTY(bool CanPlay READ CanPlay NOTIFY canPlayChanged)
    Q_PROPERTY(bool CanPause READ CanPause NOTIFY canPauseChanged)
    Q_PROPERTY(bool CanControl READ CanControl NOTIFY canControlChanged)
    Q_PROPERTY(bool CanSeek READ CanSeek NOTIFY canSeekChanged)

public:
    explicit MediaPlayer2Player(AudioManager *audioPlayer, bool showProgressOnTaskBar, QObject *parent = nullptr);
    ~MediaPlayer2Player() override;

    QString PlaybackStatus() const;
    double Rate() const;
    QVariantMap Metadata() const;
    double Volume() const;
    qlonglong Position() const;
    double MinimumRate() const;
    double MaximumRate() const;
    bool CanGoNext() const;
    bool CanGoPrevious() const;
    bool CanPlay() const;
    bool CanPause() const;
    bool CanSeek() const;
    bool CanControl() const;
    bool showProgressOnTaskBar() const;
    void setShowProgressOnTaskBar(bool value);

Q_SIGNALS:
    void Seeked(qlonglong Position);

    void rateChanged(double newRate);
    void volumeChanged(double newVol);
    void playbackStatusChanged();
    void canGoNextChanged();
    void canGoPreviousChanged();
    void canPlayChanged();
    void canPauseChanged();
    void canControlChanged();
    void canSeekChanged();
    void next();
    void previous();
    void playPause();
    void stop();

public Q_SLOTS:

    void setRate(double newRate);
    void setVolume(double volume);
    void Next();
    void Previous();
    void Pause();
    void PlayPause();
    void Stop();
    void Play();
    void Seek(qlonglong Offset);
    void SetPosition(const QDBusObjectPath &trackId, qlonglong pos);
    void OpenUri(const QString &uri);

private Q_SLOTS:

    void playerPlaybackStateChanged();
    void playerSeeked(qint64 position);
    void playerVolumeChanged();
    void playerCanGoNextChanged();
    void playerCanGoPreviousChanged();
    void playerCanPlayChanged();
    void playerCanPauseChanged();
    void playerCanSeekChanged();

    // progress on taskbar
    void audioPositionChanged();
    void audioDurationChanged();

private:
    void signalPropertiesChange(const QString &property, const QVariant &value);

    void setEntry(Entry *entry);

    QVariantMap getMetadataOfCurrentTrack();

    AudioManager *m_audioPlayer = nullptr;
    QVariantMap m_metadata;
    QString m_currentTrackId;
    double m_volume = 0.0;

    // progress on taskbar
    void setPropertyPosition(int newPositionInMs);

    mutable QDBusMessage mProgressIndicatorSignal;
    int mPreviousProgressPosition = 0;
    bool mShowProgressOnTaskBar = true;
    qlonglong m_position = 0;
};
