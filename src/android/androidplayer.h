/*
   SPDX-FileCopyrightText: 2025 (c) Pedro Nishiyama <nishiyama.v3@gmail.com>

   SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "androidplayerjni.h"

#include <QObject>

#include "audiomanager.h"

class AndroidPlayer : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(AudioManager *audioManager READ audioManager WRITE setAudioManager NOTIFY audioManagerChanged)

public:
    explicit AndroidPlayer(QObject *parent = nullptr);
    ~AndroidPlayer() override;

    [[nodiscard]] AudioManager *audioManager() const;

public Q_SLOTS:

    void setAudioManager(AudioManager *audioManager);

    void initialize();

Q_SIGNALS:

    void audioManagerChanged();

private Q_SLOTS:

    void playerStatusChanged();

    void playerPlaybackStateChanged();

    void playerPositionChanged(qint64 position);

    void next();

    void previous();

    void playPause();

    void stop();

    void seek(qlonglong offset);

private:
    AndroidPlayerJni *m_androidplayerjni = &AndroidPlayerJni::instance();

    AudioManager *m_audiomanager = nullptr;
};
