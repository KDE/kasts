/**
 * SPDX-FileCopyrightText: 2014 (c) Sujith Haridasan <sujith.haridasan@kdemail.net>
 * SPDX-FileCopyrightText: 2014 (c) Ashish Madeti <ashishmadeti@gmail.com>
 * SPDX-FileCopyrightText: 2016 (c) Matthieu Gallien <matthieu_gallien@yahoo.fr>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "mpris2.h"
#include "audiomanager.h"
#if !defined Q_OS_ANDROID
#include "mediaplayer2.h"
#include "mediaplayer2player.h"
#endif

#if !defined Q_OS_ANDROID
#include <QDBusConnection>
#endif

#if defined Q_OS_WIN
#include <Windows.h>
#else
#include <unistd.h>
#endif

Mpris2::Mpris2(QObject *parent)
    : QObject(parent)
{
}

void Mpris2::initDBusService()
{
#if !defined Q_OS_ANDROID
    QString mspris2Name(QStringLiteral("org.mpris.MediaPlayer2.") + m_playerName);

    bool success = QDBusConnection::sessionBus().registerService(mspris2Name);

    // If the above failed, it's likely because we're not the first instance
    // or the name is already taken. In that event the MPRIS2 spec wants the
    // following:
    if (!success) {
#if defined Q_OS_WIN
        success = QDBusConnection::sessionBus().registerService(mspris2Name + QLatin1String(".instance") + QString::number(GetCurrentProcessId()));
#else
        success = QDBusConnection::sessionBus().registerService(mspris2Name + QLatin1String(".instance") + QString::number(getpid()));
#endif
    }

    if (success) {
        m_mp2 = std::make_unique<MediaPlayer2>(this);
        m_mp2p = std::make_unique<MediaPlayer2Player>(m_audioPlayer, mShowProgressOnTaskBar, this);

        QDBusConnection::sessionBus().registerObject(QStringLiteral("/org/mpris/MediaPlayer2"), this, QDBusConnection::ExportAdaptors);

        connect(m_mp2.get(), &MediaPlayer2::raisePlayer, this, &Mpris2::raisePlayer);
    }
#endif
}

Mpris2::~Mpris2() = default;

QString Mpris2::playerName() const
{
    return m_playerName;
}

AudioManager *Mpris2::audioPlayer() const
{
    return m_audioPlayer;
}

bool Mpris2::showProgressOnTaskBar() const
{
    return mShowProgressOnTaskBar;
}

void Mpris2::setPlayerName(const QString &playerName)
{
    if (m_playerName == playerName) {
        return;
    }

    m_playerName = playerName;

#if !defined Q_OS_ANDROID
    if (m_audioPlayer && !m_playerName.isEmpty()) {
        if (!m_mp2) {
            initDBusService();
        }
    }
#endif

    emit playerNameChanged();
}

void Mpris2::setAudioPlayer(AudioManager *audioPlayer)
{
    if (m_audioPlayer == audioPlayer)
        return;

    m_audioPlayer = audioPlayer;

#if !defined Q_OS_ANDROID
    if (m_audioPlayer && !m_playerName.isEmpty()) {
        if (!m_mp2) {
            initDBusService();
        }
    }
#endif

    emit audioPlayerChanged();
}

void Mpris2::setShowProgressOnTaskBar(bool value)
{
#if !defined Q_OS_ANDROID
    m_mp2p->setShowProgressOnTaskBar(value);
#endif
    mShowProgressOnTaskBar = value;
    Q_EMIT showProgressOnTaskBarChanged();
}
