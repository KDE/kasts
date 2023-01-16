/**
 * SPDX-FileCopyrightText: 2014 Sujith Haridasan <sujith.haridasan@kdemail.net>
 * SPDX-FileCopyrightText: 2014 Ashish Madeti <ashishmadeti@gmail.com>
 * SPDX-FileCopyrightText: 2016 Matthieu Gallien <matthieu_gallien@yahoo.fr>
 * SPDX-FileCopyrightText: 2022-2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QDBusAbstractAdaptor>
#include <QStringList>

#include "kmediasession.h"

class MediaPlayer2 : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2") // Docs: https://specifications.freedesktop.org/mpris-spec/latest/Media_Player.html

    Q_PROPERTY(bool CanQuit READ CanQuit CONSTANT)
    Q_PROPERTY(bool CanRaise READ CanRaise CONSTANT)
    Q_PROPERTY(bool HasTrackList READ HasTrackList CONSTANT)
    Q_PROPERTY(QString Identity READ Identity CONSTANT)
    Q_PROPERTY(QString DesktopEntry READ DesktopEntry CONSTANT)
    Q_PROPERTY(QStringList SupportedUriSchemes READ SupportedUriSchemes CONSTANT)
    Q_PROPERTY(QStringList SupportedMimeTypes READ SupportedMimeTypes CONSTANT)

public:
    explicit MediaPlayer2(KMediaSession *audioPlayer, QObject *parent = nullptr);
    ~MediaPlayer2() override;

    [[nodiscard]] bool CanQuit() const;
    [[nodiscard]] bool CanRaise() const;
    [[nodiscard]] bool HasTrackList() const;

    [[nodiscard]] QString Identity() const;
    [[nodiscard]] QString DesktopEntry() const;

    [[nodiscard]] QStringList SupportedUriSchemes() const;
    [[nodiscard]] QStringList SupportedMimeTypes() const;

public Q_SLOTS:
    void Quit();
    void Raise();

Q_SIGNALS:
    void raisePlayer();
    void quitPlayer();

private:
    KMediaSession *m_audioPlayer = nullptr;
};
