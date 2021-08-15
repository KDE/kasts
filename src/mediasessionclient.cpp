/*
 * SPDX-FileCopyrightText: 2021 Swapnil Tripathi <swapnil06.st@gmail.com>
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "mediasessionclient.h"
#include "audiomanager.h"

#include <QDebug>

#ifdef Q_OS_ANDROID
#include <QAndroidJniObject>
#include <QAndroidJniEnvironment>

static void play(JNIEnv *env, jobject thiz)
{
    Q_UNUSED(env)
    Q_UNUSED(thiz)
    qDebug() << "JAVA play() working.";
    // audio manager play
}
static void pause(JNIEnv *env, jobject thiz)
{
    Q_UNUSED(env)
    Q_UNUSED(thiz)
    qDebug() << "JAVA pause() working.";
    // audio manager pause
}
static void stop(JNIEnv *env, jobject thiz)
{
    Q_UNUSED(env)
    Q_UNUSED(thiz)
    qDebug() << "JAVA stop() working.";
    //audio manager previous
}
static void next(JNIEnv *env, jobject thiz)
{
    Q_UNUSED(env)
    Q_UNUSED(thiz)
    qDebug() << "JAVA next() working.";
    // audio manager next
}
static void seek(JNIEnv *env, jobject thiz, jlong position)
{
    Q_UNUSED(env)
    Q_UNUSED(thiz)
    Q_UNUSED(position)
    qDebug() << "JAVA seek() working.";
    // implement seek
}
static const JNINativeMethod methods[] {{"playerPlay", "()V", reinterpret_cast<void *>(play)},
    {"playerPause", "()V", reinterpret_cast<void *>(pause)},
    {"playerStop", "()V", reinterpret_cast<void *>(stop)},
    {"playerNext", "()V", reinterpret_cast<void *>(next)},
    {"playerSeek", "(J)V", reinterpret_cast<void *>(seek)}};

Q_DECL_EXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *)
{
    static bool initialized = false;
    if (initialized)
        return JNI_VERSION_1_6;
    initialized = true;

    JNIEnv *env = nullptr;
    if (vm->GetEnv((void **)&env, JNI_VERSION_1_4) != JNI_OK) {
        qWarning() << "Failed to get JNI environment.";
        return -1;
    }
    jclass theclass = env->FindClass("org/kde/kasts/KastsActivity");
    if (env->RegisterNatives(theclass, methods, sizeof(methods) / sizeof(JNINativeMethod)) < 0) {
        qWarning() << "Failed to register native functions.";
        return -1;
    }
    return JNI_VERSION_1_4;
}
#endif

MediaSessionClient* MediaSessionClient::s_instance = nullptr;

MediaSessionClient::MediaSessionClient(AudioManager *audioPlayer, QObject *parent)
    : QObject(parent)
    , m_audioPlayer(audioPlayer)
{
    s_instance = this;
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

MediaSessionClient* MediaSessionClient::instance()
{
    return s_instance;
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
#ifndef Q_OS_ANDROID
    Q_UNUSED(status)
#endif

#ifdef Q_OS_ANDROID
    QAndroidJniObject::callStaticMethod<void>("org/kde/kasts/KastsActivity", "setSessionState", "(I)V", status);
#endif
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
#ifdef Q_OS_ANDROID
    QAndroidJniObject title = QAndroidJniObject::fromString(entry->title());
    // Title string
    QAndroidJniObject author = QAndroidJniObject::fromString(authorString);
    // Author string
    QAndroidJniObject album = QAndroidJniObject::fromString(QStringLiteral("Album"));
    // Author string
    qint64 duration = qint64(m_audioPlayer->duration());
    // Playback duration
    qint64 position = qint64(m_audioPlayer->position());
    // Playback position
    int rate = m_audioPlayer->playbackRate();
    // Playback rate

    QAndroidJniObject::callStaticMethod<void>("org/kde/kasts/KastsActivity", "setMetadata","(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;JJF)V",title.object<jstring>(), author.object<jstring>(), album.object<jstring>(), (jlong)position, (jlong)duration, (jfloat)rate);
#endif
}

void MediaSessionClient::setPlaybackRate()
{
    /*
     * Sets the media session's rate metadata.
     */
    int rate = m_audioPlayer->playbackRate();
#ifndef Q_OS_ANDROID
    Q_UNUSED(rate)
#endif

#ifdef Q_OS_ANDROID
    QAndroidJniObject::callStaticMethod<void>("org/kde/kasts/KastsActivity", "setPlaybackSpeed", "(I)V", rate);
#endif
}

void MediaSessionClient::setDuration()
{
    /*
     * Sets the media session's playback duration.
     */
    qint64 duration = qint64(m_audioPlayer->duration());
#ifndef Q_OS_ANDROID
    Q_UNUSED(duration)
#endif

#ifdef Q_OS_ANDROID
    QAndroidJniObject::callStaticMethod<void>("org/kde/kasts/KastsActivity", "setDuration", "(J)V", (jlong)duration);
#endif
}

void MediaSessionClient::setPosition()
{
    /*
     * Sets the media session's current playback position.
     */
    qint64 position = qint64(m_audioPlayer->position());

#ifndef Q_OS_ANDROID
    Q_UNUSED(position)
#endif

#ifdef Q_OS_ANDROID
    QAndroidJniObject::callStaticMethod<void>("org/kde/kasts/KastsActivity", "setPosition", "(J)V", (jlong)position);
#endif
}
