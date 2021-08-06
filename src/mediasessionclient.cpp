/**
 * SPDX-FileCopyrightText: 2021 Swapnil Tripathi <swapnil06.st@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "mediasessionclient.h"
#include "audiomanager.h"

#include <QtAndroid>
#include <QDebug>

MediaSessionClient::MediaSessionClient(AudioManager *audioPlayer, QObject *parent)
    : QObject(parent)
    , m_audioPlayer(audioPlayer)
{
    connect(m_audioPlayer, &AudioManager::playbackStateChanged, this, &MediaSessionClient::setState);
}

void MediaSessionClient::setState()
{
    qDebug() << m_audioPlayer->playbackState();
    switch(m_audioPlayer->playbackState()) {
        case QMediaPlayer::StoppedState :
            QAndroidJniObject::callStaticMethod<jint>
                                ("org/kde/kasts/MediaService"
                                , "setSessionState"
                                , "(I)I"
                                , 2);
        case QMediaPlayer::PausedState :
            QAndroidJniObject::callStaticMethod<jint>
                                ("org/kde/kasts/MediaService"
                                , "setSessionState"
                                , "(I)I"
                                , 1);
        case QMediaPlayer::PlayingState :
            QAndroidJniObject::callStaticMethod<jint>
                                ("org/kde/kasts/MediaService"
                                , "setSessionState"
                                , "(I)I"
                                , 0);
    }
}
