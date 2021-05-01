/**
 * SPDX-FileCopyrightText: 2014 (c) Sujith Haridasan <sujith.haridasan@kdemail.net>
 * SPDX-FileCopyrightText: 2014 (c) Ashish Madeti <ashishmadeti@gmail.com>
 * SPDX-FileCopyrightText: 2016 (c) Matthieu Gallien <matthieu_gallien@yahoo.fr>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QObject>
#include <QSharedPointer>
#include <memory>

class MediaPlayer2Player;
class MediaPlayer2;
class AudioManager;

class Mpris2 : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString playerName READ playerName WRITE setPlayerName NOTIFY playerNameChanged)
    Q_PROPERTY(AudioManager *audioPlayer READ audioPlayer WRITE setAudioPlayer NOTIFY audioPlayerChanged)
    Q_PROPERTY(bool showProgressOnTaskBar READ showProgressOnTaskBar WRITE setShowProgressOnTaskBar NOTIFY showProgressOnTaskBarChanged)

public:
    explicit Mpris2(QObject *parent = nullptr);
    ~Mpris2() override;

    [[nodiscard]] QString playerName() const;

    [[nodiscard]] AudioManager *audioPlayer() const;

    [[nodiscard]] bool showProgressOnTaskBar() const;

public Q_SLOTS:

    void setPlayerName(const QString &playerName);

    void setAudioPlayer(AudioManager *audioPlayer);

    void setShowProgressOnTaskBar(bool value);

Q_SIGNALS:
    void raisePlayer();

    void playerNameChanged();

    void audioPlayerChanged();

    void showProgressOnTaskBarChanged();

private:
    void initDBusService();

    std::unique_ptr<MediaPlayer2> m_mp2;
    std::unique_ptr<MediaPlayer2Player> m_mp2p;
    QString m_playerName;
    AudioManager *m_audioPlayer = nullptr;
    bool mShowProgressOnTaskBar = true;
};
