/*
 * SPDX-FileCopyrightText: 2021 Swapnil Tripathi <swapnil06.st@gmail.com>
 * SPDX-License-Identifier: LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include <QObject>

#ifdef Q_OS_ANDROID
#include <QtAndroid>
#endif

class AudioManager;
class Entry;

class MediaSessionClient : public QObject
{
    Q_OBJECT
public:
    explicit MediaSessionClient(AudioManager *audioPlayer, QObject *parent = nullptr);
    static MediaSessionClient* instance();

Q_SIGNALS:
    void play();
    void pause();
    void next();

private Q_SLOTS:
    void setSessionPlaybackState();
    void setSessionMetadata();
    void setPlaybackRate();
    void setDuration();
    void setPosition();

private:
    AudioManager *m_audioPlayer = nullptr;
    static MediaSessionClient *s_instance;
};
