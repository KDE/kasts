/**
 * SPDX-FileCopyrightText: 2021 Swapnil Tripathi <swapnil06.st@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QObject>
#include <QtAndroid>

class AudioManager;
class Entry;

class MediaSessionClient : public QObject
{
public:
    explicit MediaSessionClient(AudioManager *audioPlayer, QObject *parent = nullptr);

private Q_SLOTS:
    void setSessionPlaybackState();
    void setSessionMetadata();
    void setPlaybackRate();
    void setDuration();
    void setPosition();

private:
    AudioManager *m_audioPlayer = nullptr;
};
