/**
 * SPDX-FileCopyrightText: 2021 Swapnil Tripathi <swapnil06.st@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "mediasessionclient.h"
#include "audiomanager.h"

#include <QDebug>
#include <QAndroidJniObject>
#include <QAndroidJniEnvironment>

static void play(JNIEnv *env, jobject thiz)
{
    Q_UNUSED(env)
    Q_UNUSED(thiz)
    // audio manager play
}
static void pause(JNIEnv *env, jobject thiz)
{
    Q_UNUSED(env)
    Q_UNUSED(thiz)
    // audio manager pause
}
static void stop(JNIEnv *env, jobject thiz)
{
    Q_UNUSED(env)
    Q_UNUSED(thiz)
    //audio manager previous
}
static void next(JNIEnv *env, jobject thiz)
{
    Q_UNUSED(env)
    Q_UNUSED(thiz)
    // audio manager next
}
static void seek(JNIEnv *env, jobject thiz, jlong position)
{
    Q_UNUSED(env)
    Q_UNUSED(thiz)
    // implement seek
}
void registerNativeMethods() {
    JNINativeMethod methods[] {{"play", "()V", reinterpret_cast<void *>(play)},
    {"pause", "()V", reinterpret_cast<void *>(pause)},
    {"stop", "()V", reinterpret_cast<void *>(stop)},
    {"next", "()V", reinterpret_cast<void *>(next)},
    {"seek", "(J)V", reinterpret_cast<void *>(seek)}};

    QAndroidJniObject javaClass("org/kde/kasts/KastsActivity");
    QAndroidJniEnvironment env;
    jclass objectClass = env->GetObjectClass(javaClass.object<jobject>());
    env->RegisterNatives(objectClass,
                         methods,
                         sizeof(methods) / sizeof(methods[0]));
    env->DeleteLocalRef(objectClass);
}

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
            authorString.append(author->name());
            if(entry->authors().count() > 1)
                authorString.append(QStringLiteral(", "));
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
