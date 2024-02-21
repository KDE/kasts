/**
 * SPDX-FileCopyrightText: 2014 Sujith Haridasan <sujith.haridasan@kdemail.net>
 * SPDX-FileCopyrightText: 2014 Ashish Madeti <ashishmadeti@gmail.com>
 * SPDX-FileCopyrightText: 2016 Matthieu Gallien <matthieu_gallien@yahoo.fr>
 * SPDX-FileCopyrightText: 2022-2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <memory>

#include <QObject>
#include <QSharedPointer>

#if !defined Q_OS_ANDROID && !defined Q_OS_WIN
class MediaPlayer2Player;
class MediaPlayer2;
#endif
class KMediaSession;

class Mpris2 : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool showProgressOnTaskBar READ showProgressOnTaskBar WRITE setShowProgressOnTaskBar NOTIFY showProgressOnTaskBarChanged)

public:
    explicit Mpris2(QObject *parent = nullptr);
    ~Mpris2() override;

    [[nodiscard]] bool showProgressOnTaskBar() const;

public Q_SLOTS:
    void setShowProgressOnTaskBar(bool value);

Q_SIGNALS:
    void showProgressOnTaskBarChanged();

private:
    void initDBusService(const QString &playerName);
    bool unregisterDBusService(const QString &playerName);

#if !defined Q_OS_ANDROID && !defined Q_OS_WIN
    std::unique_ptr<MediaPlayer2> m_mp2;
    std::unique_ptr<MediaPlayer2Player> m_mp2p;
#endif

    KMediaSession *m_audioPlayer = nullptr;
    bool m_ShowProgressOnTaskBar = true;
    QString m_playerName;
};
