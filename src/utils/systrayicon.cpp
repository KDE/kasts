/**
 * SPDX-FileCopyrightText: 2023 Bart De Vries <bart@mogwai.be>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "systrayicon.h"

#ifndef Q_OS_ANDROID
#include <QCoreApplication>
#include <QFile>
#include <QMenu>

#include <KLocalizedString>

#include "audiomanager.h"
#include "kmediasession.h"
#include "settingsmanager.h"

SystrayIcon::SystrayIcon(QObject *parent)
    : QSystemTrayIcon(parent)
{
    setIconColor(intToIconColorEnum(SettingsManager::self()->trayIconType()));
    setToolTip(i18nc("@info:tooltip",
                     "Kasts\n"
                     "Middle-click to play/pause"));

    QMenu *menu = new QMenu();

    connect(this, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger) {
            Q_EMIT raiseWindow();
        } else if (reason == QSystemTrayIcon::MiddleClick) {
            AudioManager::instance().playPause();
        }
    });

    connect(SettingsManager::self(), &SettingsManager::showTrayIconChanged, this, [this]() {
        if (SettingsManager::self()->showTrayIcon()) {
            show();
        } else {
            hide();
        }
    });

    connect(SettingsManager::self(), &SettingsManager::trayIconTypeChanged, this, [this]() {
        setIconColor(intToIconColorEnum(SettingsManager::self()->trayIconType()));
    });

    // Seek backward
    QAction *skipBackwardAction = new QAction(i18nc("@action:inmenu", "Seek Backward"), this);
    skipBackwardAction->setIcon(QIcon::fromTheme(QStringLiteral("media-seek-backward")));
    skipBackwardAction->setEnabled(AudioManager::instance().canSkipBackward());

    connect(skipBackwardAction, &QAction::triggered, &AudioManager::instance(), &AudioManager::skipBackward);
    menu->addAction(skipBackwardAction);

    connect(&AudioManager::instance(), &AudioManager::canSkipBackwardChanged, this, [skipBackwardAction]() {
        skipBackwardAction->setEnabled(AudioManager::instance().canSkipBackward());
    });

    // Play / Pause
    QAction *playAction = new QAction(i18nc("@action:inmenu", "Play"), this);
    playAction->setIcon(QIcon::fromTheme(QStringLiteral("media-playback-start")));
    playAction->setVisible(AudioManager::instance().playbackState() != KMediaSession::PlaybackState::PlayingState);
    playAction->setEnabled(AudioManager::instance().canPlay());

    connect(playAction, &QAction::triggered, &AudioManager::instance(), &AudioManager::play);
    menu->addAction(playAction);

    QAction *pauseAction = new QAction(i18nc("@action:inmenu", "Pause"), this);
    pauseAction->setIcon(QIcon::fromTheme(QStringLiteral("media-playback-pause")));
    pauseAction->setVisible(AudioManager::instance().playbackState() == KMediaSession::PlaybackState::PlayingState);
    pauseAction->setEnabled(AudioManager::instance().canPlay());

    connect(pauseAction, &QAction::triggered, &AudioManager::instance(), &AudioManager::pause);
    menu->addAction(pauseAction);

    connect(&AudioManager::instance(), &AudioManager::playbackStateChanged, this, [playAction, pauseAction](KMediaSession::PlaybackState state) {
        playAction->setVisible(state != KMediaSession::PlaybackState::PlayingState);
        pauseAction->setVisible(state == KMediaSession::PlaybackState::PlayingState);
    });

    connect(&AudioManager::instance(), &AudioManager::canPlayChanged, this, [playAction, pauseAction]() {
        playAction->setEnabled(AudioManager::instance().canPlay());
        pauseAction->setEnabled(AudioManager::instance().canPlay());
    });

    // Seek forward
    QAction *skipForwardAction = new QAction(i18nc("@action:inmenu", "Seek Forward"), this);
    skipForwardAction->setIcon(QIcon::fromTheme(QStringLiteral("media-seek-forward")));
    skipForwardAction->setEnabled(AudioManager::instance().canSkipForward());

    connect(skipForwardAction, &QAction::triggered, &AudioManager::instance(), &AudioManager::skipForward);
    menu->addAction(skipForwardAction);

    connect(&AudioManager::instance(), &AudioManager::canSkipForwardChanged, this, [skipForwardAction]() {
        skipForwardAction->setEnabled(AudioManager::instance().canSkipForward());
    });

    // Skip forward
    QAction *nextAction = new QAction(i18nc("@action:inmenu", "Skip Forward"), this);
    nextAction->setIcon(QIcon::fromTheme(QStringLiteral("media-skip-forward")));
    nextAction->setEnabled(AudioManager::instance().canGoNext());

    connect(nextAction, &QAction::triggered, &AudioManager::instance(), &AudioManager::next);
    menu->addAction(nextAction);

    connect(&AudioManager::instance(), &AudioManager::canGoNextChanged, this, [nextAction]() {
        nextAction->setEnabled(AudioManager::instance().canGoNext());
    });

    // Separator
    menu->addSeparator();

    // Quit
    QAction *quitAction = new QAction(i18nc("@action:inmenu", "Quit"), this);
    quitAction->setIcon(QIcon::fromTheme(QStringLiteral("application-exit")));

    connect(quitAction, &QAction::triggered, QCoreApplication::instance(), QCoreApplication::quit);
    menu->addAction(quitAction);

    setContextMenu(menu);

    if (SettingsManager::self()->showTrayIcon()) {
        show();
    }
}
#else
SystrayIcon::SystrayIcon(QObject *parent)
    : QObject(parent)
{
}
#endif

SystrayIcon::~SystrayIcon()
{
}

bool SystrayIcon::available() const
{
#ifndef Q_OS_ANDROID
    return QSystemTrayIcon::isSystemTrayAvailable();
#else
    return false;
#endif
}

void SystrayIcon::setIconColor(SystrayIcon::IconColor iconColor)
{
#ifndef Q_OS_ANDROID
    // do not specify svg-extension; icon will not be visible due to [QTBUG-53550]
    switch (iconColor) {
    case SystrayIcon::IconColor::Colorful:
        setIcon(QIcon(QStringLiteral(":/logo")));
        break;
    case SystrayIcon::IconColor::Light:
        setIcon(QIcon(QStringLiteral(":/kasts-tray-light")));
        break;
    case SystrayIcon::IconColor::Dark:
        setIcon(QIcon(QStringLiteral(":/kasts-tray-dark")));
        break;
    }
#endif
}

int SystrayIcon::iconColorEnumToInt(SystrayIcon::IconColor iconColor)
{
    switch (iconColor) {
    case SystrayIcon::IconColor::Light:
        return 1;
    case SystrayIcon::IconColor::Dark:
        return 2;
    case SystrayIcon::IconColor::Colorful:
    default:
        return 0;
    }
}

SystrayIcon::IconColor SystrayIcon::intToIconColorEnum(int iconColorCode)
{
    switch (iconColorCode) {
    case 1:
        return SystrayIcon::IconColor::Light;
    case 2:
        return SystrayIcon::IconColor::Dark;
    case 0:
    default:
        return SystrayIcon::IconColor::Colorful;
    }
}
