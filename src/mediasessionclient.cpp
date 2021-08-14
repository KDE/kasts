/**
 * SPDX-FileCopyrightText: 2021 Swapnil Tripathi <swapnil06.st@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "mediasessionclient.h"
#include "audiomanager.h"

#include <QDebug>

MediaSessionClient::MediaSessionClient(AudioManager *audioPlayer, QObject *parent)
    : QObject(parent)
    , m_audioPlayer(audioPlayer)
{
    connect(m_audioPlayer, &AudioManager::playbackStateChanged, this, &MediaSessionClient::setSessionPlaybackState);
    // Sets the current playback state.
    connect(m_audioPlayer, &AudioManager::entryChanged, this, &MediaSessionClient::setSessionMetadata);
    // Updates the android session's metadata.
    connect(m_audioPlayer, &AudioManager::playbackRateChanged, this, &MediaSessionClient::setPlaybackRate);
    // Sets the rate of the media playback.
    connect(m_audioPlayer, &AudioManager::durationChanged, this, &MediaSessionClient::setDuration);
    // Sets the playback duration metadata.
    connect(m_audioPlayer, &AudioManager::positionChanged, this, &MediaSessionClient::setPosition);
    // Sets the playback position metadata.
    connect(m_audioPlayer, &AudioManager::playing, this, &MediaSessionClient::setSessionPlaybackState);
    // Sets the playback to playing.
    connect(m_audioPlayer, &AudioManager::paused, this, &MediaSessionClient::setSessionPlaybackState);
    // Sets the playback to paused.
    connect(m_audioPlayer, &AudioManager::stopped, this, &MediaSessionClient::setSessionPlaybackState);
    // Sets the playback to stopped.
}

void MediaSessionClient::setSessionPlaybackState()
{
    qDebug() << "MediaSessionClient::setSessionPlaybackState called with state value = " << m_audioPlayer->playbackState();
    int status = -1;
    switch(m_audioPlayer->playbackState()) {
        case QMediaPlayer::PlayingState :
            status = 0;
            break;
        case QMediaPlayer::PausedState :
            status = 1;
            break;
        case QMediaPlayer::StoppedState :
            status = 2;
            break;
    }
    QAndroidJniObject::callStaticMethod<void>("org/kde/kasts/KastsActivity", "setSessionState", "(I)V", status);
}

void MediaSessionClient::setSessionMetadata()
{
    /*
     * Sets the media session's metadata. This will be triggered every time there is state change ie. next, previous, etc.
     */
    Entry *entry = m_audioPlayer->entry();

    QString authorString = QStringLiteral("");
    if (entry->authors().count() > 0) {
        for (auto &author : entry->authors()) {
            authorString.append(QStringLiteral(", "));
            authorString.append(author->name());
        }
    }
    QAndroidJniObject title = QAndroidJniObject::fromString(entry->title());
    // Title string
    QAndroidJniObject author = QAndroidJniObject::fromString(authorString);
    // Author string
    QAndroidJniObject album = QAndroidJniObject::fromString(QStringLiteral("Album"));
    // Author string
    qint64 duration = qint64(m_audioPlayer->duration()) * 1000;
    // Playback duration
    qint64 position = qint64(m_audioPlayer->position()) * 1000;
    // Playback position
    int rate = m_audioPlayer->playbackRate();
    // Playback rate

    QAndroidJniObject::callStaticMethod<void>("org/kde/kasts/KastsActivity", "setMetadata","(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;JJF)V",title.object<jstring>(), author.object<jstring>(), album.object<jstring>(), position, duration, rate);
}

void MediaSessionClient::setPlaybackRate()
{
    /*
     * Sets the media session's rate metadata.
     */
    int rate = m_audioPlayer->playbackRate();
    QAndroidJniObject::callStaticMethod<void>("org/kde/kasts/KastsActivity", "setPlaybackSpeed", "(I)V", rate);
}

void MediaSessionClient::setDuration()
{
    /*
     * Sets the media session's playback duration.
     */
    qint64 duration = qint64(m_audioPlayer->duration()) * 1000;
    QAndroidJniObject::callStaticMethod<void>("org/kde/kasts/KastsActivity", "setDuration", "(I)V", duration);
}

void MediaSessionClient::setPosition()
{
    /*
     * Sets the media session's current playback position.
     */
    qint64 position = qint64(m_audioPlayer->position()) * 1000;
    QAndroidJniObject::callStaticMethod<void>("org/kde/kasts/KastsActivity", "setPosition", "(I)V", position);
}
