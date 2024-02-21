/**
 * SPDX-FileCopyrightText: 2014 Sujith Haridasan <sujith.haridasan@kdemail.net>
 * SPDX-FileCopyrightText: 2014 Ashish Madeti <ashishmadeti@gmail.com>
 * SPDX-FileCopyrightText: 2016 Matthieu Gallien <matthieu_gallien@yahoo.fr>
 * SPDX-FileCopyrightText: 2022-2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "mpris2.h"
#include "mpris2logging.h"

#include <QDebug>
#if !defined Q_OS_ANDROID && !defined Q_OS_WIN
#include <QDBusConnection>
#endif

#if defined Q_OS_WIN
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include "kmediasession.h"
#if !defined Q_OS_ANDROID && !defined Q_OS_WIN
#include "mediaplayer2.h"
#include "mediaplayer2player.h"
#endif

Mpris2::Mpris2(QObject *parent)
    : QObject(parent)
    , m_audioPlayer(static_cast<KMediaSession *>(parent))
{
    qCDebug(Mpris2Log) << "Mpris2::Mpris2()";

#if !defined Q_OS_ANDROID && !defined Q_OS_WIN
    connect(m_audioPlayer, &KMediaSession::playerNameChanged, this, [this]() {
        bool success = true;
        if (m_mp2) {
            success = unregisterDBusService(m_playerName);
        }

        if (success) {
            initDBusService(m_audioPlayer->playerName());
        }
    });

    initDBusService(m_audioPlayer->playerName());
#endif
}

void Mpris2::initDBusService(const QString &playerName)
{
    qCDebug(Mpris2Log) << "Mpris2::initDBusService(" << playerName << ")";
#if !defined Q_OS_ANDROID && !defined Q_OS_WIN

    QString tryPlayerName = playerName;
    QString mpris2Name(QStringLiteral("org.mpris.MediaPlayer2.") + tryPlayerName);

    bool success = QDBusConnection::sessionBus().registerService(mpris2Name);

    // If the above failed, it's likely because we're not the first instance
    // or the name is already taken. In that event the MPRIS2 spec wants the
    // following:
    if (!success) {
        tryPlayerName = tryPlayerName + QLatin1String(".instance") + QString::number(getpid());

        success = QDBusConnection::sessionBus().registerService(QStringLiteral("org.mpris.MediaPlayer2.") + tryPlayerName);
    }

    if (success) {
        m_playerName = tryPlayerName;
        if (!m_mp2) {
            m_mp2 = std::make_unique<MediaPlayer2>(m_audioPlayer, this);
            m_mp2p = std::make_unique<MediaPlayer2Player>(m_audioPlayer, m_ShowProgressOnTaskBar, this);
        }

        QDBusConnection::sessionBus().registerObject(QStringLiteral("/org/mpris/MediaPlayer2"), this, QDBusConnection::ExportAdaptors);
    }
#endif
}

bool Mpris2::unregisterDBusService(const QString &playerName)
{
    bool success = true;
#if !defined Q_OS_ANDROID && !defined Q_OS_WIN
    QString oldMpris2Name(QStringLiteral("org.mpris.MediaPlayer2.") + playerName);
    success = QDBusConnection::sessionBus().unregisterService(oldMpris2Name);

    if (success) {
        m_playerName = QLatin1String("");
    }
#endif
    return success;
}

Mpris2::~Mpris2() = default;

bool Mpris2::showProgressOnTaskBar() const
{
    qCDebug(Mpris2Log) << "Mpris2::showProgressOnTaskBar()";
    return m_ShowProgressOnTaskBar;
}

void Mpris2::setShowProgressOnTaskBar(bool value)
{
    qCDebug(Mpris2Log) << "Mpris2::setShowProgressOnTaskBar" << value << ")";
#if !defined Q_OS_ANDROID && !defined Q_OS_WIN
    m_mp2p->setShowProgressOnTaskBar(value);
#endif
    m_ShowProgressOnTaskBar = value;
    Q_EMIT showProgressOnTaskBarChanged();
}
