/*
 * SPDX-FileCopyrightText: 2021 Swapnil Tripathi <swapnil06.st@gmail.com>
 * SPDX-License-Identifier: LicenseRef-KDE-Accepted-LGPL
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
    Q_EMIT MediaSessionClient::instance().play();
}
static void pause(JNIEnv *env, jobject thiz)
{
    Q_UNUSED(env)
    Q_UNUSED(thiz)
    emit MediaSessionClient::instance().pause();
}
static void next(JNIEnv *env, jobject thiz)
{
    Q_UNUSED(env)
    Q_UNUSED(thiz)
    emit MediaSessionClient::instance().next();
}
static void seek(JNIEnv *env, jobject thiz, jlong position)
{
    Q_UNUSED(env)
    Q_UNUSED(thiz)
    Q_UNUSED(position)
}
static const JNINativeMethod methods[] {{"playerPlay", "()V", reinterpret_cast<void *>(play)},
    {"playerPause", "()V", reinterpret_cast<void *>(pause)},
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
    jclass theclass = env->FindClass("org/kde/kasts/Receiver");
    if (env->RegisterNatives(theclass, methods, sizeof(methods) / sizeof(JNINativeMethod)) < 0) {
        qWarning() << "Failed to register native functions.";
        return -1;
    }
    return JNI_VERSION_1_4;
}

MediaSessionClient::MediaSessionClient()
    : QObject()
    , m_audioPlayer(&AudioManager::instance())
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
    connect(this, &MediaSessionClient::play, m_audioPlayer, &AudioManager::play);
    // Connect android notification's play action to play the media.
    connect(this, &MediaSessionClient::pause, m_audioPlayer, &AudioManager::pause);
    // Connect android notification's play action to pause the media.
    connect(this, &MediaSessionClient::next, m_audioPlayer, &AudioManager::next);
    // Connect android notification's play action to skip to next media.
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
    Entry *entry = m_audioPlayer->entry();

    QString authorString = QStringLiteral("");
    for (int i = 0; i < entry->authors().size(); i++) {
        authorString += entry->authors()[i]->name();
        if (i < entry->authors().size() - 1) {
            authorString += QStringLiteral(", ");
        }
    }

    const auto title = QAndroidJniObject::fromString(entry->title());
    const auto author = QAndroidJniObject::fromString(authorString);
    const auto album = QAndroidJniObject::fromString(QStringLiteral("Album"));
    const auto duration = qint64(m_audioPlayer->duration());
    const auto position = qint64(m_audioPlayer->position());
    const auto rate = m_audioPlayer->playbackRate();

    QAndroidJniObject::callStaticMethod<void>("org/kde/kasts/KastsActivity", "setMetadata","(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;JJF)V",title.object<jstring>(), author.object<jstring>(), album.object<jstring>(), (jlong)position, (jlong)duration, (jfloat)rate);
}

void MediaSessionClient::setPlaybackRate()
{
    int rate = m_audioPlayer->playbackRate();
    QAndroidJniObject::callStaticMethod<void>("org/kde/kasts/KastsActivity", "setPlaybackSpeed", "(I)V", rate);
}

void MediaSessionClient::setDuration()
{
    qint64 duration = qint64(m_audioPlayer->duration());
    QAndroidJniObject::callStaticMethod<void>("org/kde/kasts/KastsActivity", "setDuration", "(J)V", (jlong)duration);
}

void MediaSessionClient::setPosition()
{
    qint64 position = qint64(m_audioPlayer->position());
    QAndroidJniObject::callStaticMethod<void>("org/kde/kasts/KastsActivity", "setPosition", "(J)V", (jlong)position);
}
