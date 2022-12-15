/**
 * SPDX-FileCopyrightText: 2014 Sujith Haridasan <sujith.haridasan@kdemail.net>
 * SPDX-FileCopyrightText: 2014 Ashish Madeti <ashishmadeti@gmail.com>
 * SPDX-FileCopyrightText: 2016 Matthieu Gallien <matthieu_gallien@yahoo.fr>
 * SPDX-FileCopyrightText: 2022-2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "mediaplayer2.h"
#include "mpris2logging.h"

#include <QCoreApplication>
#include <QDebug>

#include <KAboutData>

MediaPlayer2::MediaPlayer2(KMediaSession *audioPlayer, QObject *parent)
    : QDBusAbstractAdaptor(parent)
    , m_audioPlayer(audioPlayer)
{
    qCDebug(Mpris2Log) << "MediaPlayer2::MediaPlayer2()";

    connect(this, &MediaPlayer2::raisePlayer, audioPlayer, &KMediaSession::raiseWindowRequested);
    connect(this, &MediaPlayer2::quitPlayer, audioPlayer, &KMediaSession::quitRequested);
}

MediaPlayer2::~MediaPlayer2()
{
    qCDebug(Mpris2Log) << "MediaPlayer2::~MediaPlayer2()";
}

bool MediaPlayer2::CanQuit() const
{
    qCDebug(Mpris2Log) << "MediaPlayer2::CanQuit()";
    return true;
}

bool MediaPlayer2::CanRaise() const
{
    qCDebug(Mpris2Log) << "MediaPlayer2::CanRaise()";
    return true;
}
bool MediaPlayer2::HasTrackList() const
{
    qCDebug(Mpris2Log) << "MediaPlayer2::HasTrackList()";
    return false;
}

void MediaPlayer2::Quit()
{
    qCDebug(Mpris2Log) << "MediaPlayer2::Quit()";
    Q_EMIT quitPlayer();
}

void MediaPlayer2::Raise()
{
    qCDebug(Mpris2Log) << "MediaPlayer2::Raise()";
    Q_EMIT raisePlayer();
}

QString MediaPlayer2::Identity() const
{
    qCDebug(Mpris2Log) << "MediaPlayer2::Identity()";
    if (m_audioPlayer) {
        return m_audioPlayer->playerName();
    }
    return QStringLiteral("kmediasession");
}

QString MediaPlayer2::DesktopEntry() const
{
    qCDebug(Mpris2Log) << "MediaPlayer2::DesktopEntry()";
    if (m_audioPlayer) {
        return m_audioPlayer->desktopEntryName();
    }
    return QLatin1String("");
}

QStringList MediaPlayer2::SupportedUriSchemes() const
{
    qCDebug(Mpris2Log) << "MediaPlayer2::SupportedUriSchemes()";
    return QStringList() << QStringLiteral("file");
}

QStringList MediaPlayer2::SupportedMimeTypes() const
{
    qCDebug(Mpris2Log) << "MediaPlayer2::SupportedMimeTypes()";
    return QStringList();
}
